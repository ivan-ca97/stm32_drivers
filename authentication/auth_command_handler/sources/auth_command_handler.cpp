#include "auth_command_handler.hpp"

#include <string_view>

// ============================================================================
// All decoding/mapping helpers are file-local. Each command function decodes from
// a Reader, calls Authentication, and fills the Response (payload on success, or an
// error code via fail()). Wire layouts are documented in auth_commands.hpp.
// ============================================================================
namespace
{
    using Response = ControllerSlave::Response;

    // ---- request payload cursor ------------------------------------------------
    // Every read is bounds-checked; the first failure latches ok()=false so the
    // caller can bail out with a single ErrMalformedRequest.
    class Reader
    {
        public:
            explicit Reader(std::span<const uint8_t> payload)
                : p(payload.data()), len(payload.size()) {}

            bool ok() const { return valid; }

            bool u8(uint8_t& out)
            {
                if(!valid || off + 1 > len) return fail();
                out = p[off++];
                return true;
            }

            bool u16(uint16_t& out)   // little-endian
            {
                if(!valid || off + 2 > len) return fail();
                out = uint16_t(p[off]) | (uint16_t(p[off + 1]) << 8);
                off += 2;
                return true;
            }

            bool u64(uint64_t& out)   // little-endian
            {
                if(!valid || off + 8 > len) return fail();
                out = 0;
                for(unsigned i = 0; i < 8; i++)
                    out |= uint64_t(p[off + i]) << (8 * i);
                off += 8;
                return true;
            }

            // NUL-terminated string; the view points into the payload (no copy).
            bool str(std::string_view& out)
            {
                if(!valid) return false;
                unsigned start = off;
                while(off < len && p[off] != 0) off++;
                if(off >= len) return fail();   // missing terminator -> malformed
                out = std::string_view(reinterpret_cast<const char*>(p + start), off - start);
                off++;                          // consume the NUL
                return true;
            }

        private:
            bool fail() { valid = false; return false; }

            const uint8_t* p;
            unsigned       len;
            unsigned       off = 0;
            bool           valid = true;
    };

    // ---- mappers ---------------------------------------------------------------
    bool wirePermToEnum(uint8_t wire, Permission& out)
    {
        switch(wire)
        {
            case auth::PermNone:        out = Permission::None;        return true;
            case auth::PermObserver:    out = Permission::Observer;    return true;
            case auth::PermMaintenance: out = Permission::Maintenance; return true;
            case auth::PermSuperuser:   out = Permission::Superuser;   return true;
            default:                    return false;
        }
    }

    uint8_t errToWire(const base::Error& error)
    {
        if(!error.belongsTo(authentication::errorCategory))
            return auth::ErrInternal;

        switch(static_cast<authentication::Code>(error.code()))
        {
            case authentication::Code::UserIdNotFound:          return auth::ErrUserIdNotFound;
            case authentication::Code::UsernameNotFound:        return auth::ErrUsernameNotFound;
            case authentication::Code::UsernameAlreadyExists:   return auth::ErrUsernameExists;
            case authentication::Code::InvalidToken:            return auth::ErrInvalidToken;
            case authentication::Code::InsufficientPermissions: return auth::ErrNoPermission;
            case authentication::Code::IncorrectPassword:       return auth::ErrIncorrectPassword;
            case authentication::Code::UsersBufferFull:         return auth::ErrUsersBufferFull;
            case authentication::Code::SessionBufferFull:       return auth::ErrSessionBufferFull;
            case authentication::Code::Overflow:                return auth::ErrOverflow;
            case authentication::Code::UsernameBufferOverflow:  return auth::ErrUsernameOverflow;
            case authentication::Code::NameBufferOverflow:      return auth::ErrNameOverflow;
            case authentication::Code::PasswordBufferOverflow:  return auth::ErrPasswordOverflow;
            case authentication::Code::EmptyMandatoryField:     return auth::ErrEmptyField;
            case authentication::Code::IntegrityFailure:        return auth::ErrIntegrityFailure;
        }
        return auth::ErrInternal;
    }

    // Success leaves the response empty; failure writes one error-code byte.
    void finishVoid(const ResultVoid& r, Response& out)
    {
        if(!r) out.fail(errToWire(r.error()));
    }

    // ---- command handlers ------------------------------------------------------
    void doLogIn(Authentication& a, Reader& r, Response& out)
    {
        std::string_view user, pass;
        if(!r.str(user) || !r.str(pass)) return out.fail(auth::ErrMalformedRequest);

        auto session = a.authenticate(user, pass);
        if(!session)            return out.fail(errToWire(session.error()));
        if(*session == nullptr) return out.fail(auth::ErrInternal);

        out.putU64((*session)->getToken());
    }

    void doLogOut(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        if(!r.u64(token)) return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.logOut(token), out);
    }

    void doCreateUser(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        std::string_view user, pass;
        uint8_t permByte;
        if(!r.u64(token) || !r.str(user) || !r.str(pass) || !r.u8(permByte))
            return out.fail(auth::ErrMalformedRequest);

        Permission perm;
        if(!wirePermToEnum(permByte, perm)) return out.fail(auth::ErrMalformedRequest);

        auto id = a.createUser(token, perm, user, pass, "");
        if(!id) return out.fail(errToWire(id.error()));
        out.putU16(*id);
    }

    void doDeleteUser(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        uint16_t id;
        if(!r.u64(token) || !r.u16(id)) return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.deleteUser(token, id), out);
    }

    void doModifyOwnUsername(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        std::string_view user;
        if(!r.u64(token) || !r.str(user)) return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.modifyOwnUsername(token, user), out);
    }

    void doModifyOwnPassword(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        std::string_view oldPass, newPass;
        if(!r.u64(token) || !r.str(oldPass) || !r.str(newPass))
            return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.modifyOwnPassword(token, oldPass, newPass), out);
    }

    void doModifyOwnName(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        std::string_view name;
        if(!r.u64(token) || !r.str(name)) return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.modifyOwnName(token, name), out);
    }

    void doModifyUsername(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        uint16_t id;
        std::string_view user;
        if(!r.u64(token) || !r.u16(id) || !r.str(user))
            return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.modifyUsername(token, id, user), out);
    }

    void doModifyPassword(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        uint16_t id;
        std::string_view pass;
        if(!r.u64(token) || !r.u16(id) || !r.str(pass))
            return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.modifyPassword(token, id, pass), out);
    }

    void doModifyName(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        uint16_t id;
        std::string_view name;
        if(!r.u64(token) || !r.u16(id) || !r.str(name))
            return out.fail(auth::ErrMalformedRequest);
        finishVoid(a.modifyName(token, id, name), out);
    }

    void doModifyPermission(Authentication& a, Reader& r, Response& out)
    {
        uint64_t token;
        uint16_t id;
        uint8_t permByte;
        if(!r.u64(token) || !r.u16(id) || !r.u8(permByte))
            return out.fail(auth::ErrMalformedRequest);

        Permission perm;
        if(!wirePermToEnum(permByte, perm)) return out.fail(auth::ErrMalformedRequest);

        finishVoid(a.modifyPermission(token, id, perm), out);
    }
}

AuthCommandHandler::Progress AuthCommandHandler::handle(ControllerSlave::Request request)
{
    Reader r(request.payload());
    Response& response = request.response();

    switch(request.cmd())
    {
        case auth::CmdLogIn:             doLogIn(authentication, r, response); break;
        case auth::CmdLogOut:            doLogOut(authentication, r, response); break;

        case auth::CmdCreateUser:        doCreateUser(authentication, r, response); break;
        case auth::CmdDeleteUser:        doDeleteUser(authentication, r, response); break;

        case auth::CmdModifyOwnUsername: doModifyOwnUsername(authentication, r, response); break;
        case auth::CmdModifyOwnPassword: doModifyOwnPassword(authentication, r, response); break;
        case auth::CmdModifyOwnName:     doModifyOwnName(authentication, r, response); break;

        case auth::CmdModifyUsername:    doModifyUsername(authentication, r, response); break;
        case auth::CmdModifyPassword:    doModifyPassword(authentication, r, response); break;
        case auth::CmdModifyName:        doModifyName(authentication, r, response); break;
        case auth::CmdModifyPermission:  doModifyPermission(authentication, r, response); break;

        default:                         response.fail(auth::ErrUnknownCommand); break;
    }

    return Progress::Done;   // every command above resolves in this one call
}
