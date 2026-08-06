# Authentication for embedded systems

User and session management with permission levels, for resource-constrained targets:
fixed-size storage (no dynamic allocation), errors returned as values (`Result<T>`), and
every hardware dependency injected as an interface.

## Subcomponents

- **`authentication/`** — the core: `User`, `UserManager`, `Session`, `SessionManager`
  and the `Authentication` facade. Token-based sessions. Injected dependencies:
  - `Clock` — time source for session expiry (the app provides the physical clock).
  - `TokenGenerator` — session-token entropy (e.g. chip UID + counter).
  - `PersistentStorage` — optional; when supplied, `UserManager` auto-persists on every
    mutation and reloads the user set on construction.
- **`auth_command_handler/`** — adapts the core to a wire protocol: decodes commands into
  typed calls and encodes the results. Plugs into a `ControllerSlave` as a
  `RequestHandler`, so it runs in the main loop and never in an ISR. The wire contract
  (opcodes, field sizes, permission encoding, error codes) lives in `auth_commands.hpp`.
