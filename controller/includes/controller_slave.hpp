#pragma once

#include <stdint.h>
#include "i2c_slave.hpp"

// ============================================================================
// ControllerSlave — implementacion GENERICA del protocolo de registros/mailbox
// (contraparte STM32 del ControllerLink del ESP32). No conoce ningun comando
// concreto: la semantica la provee un RequestHandler inyectado.
//
// Reparto de responsabilidades:
//   - Esta clase (ISR O(1)): puntero de registro, registros inmediatos, buffer
//     de request (inbox) / response (outbox), FSM de status. Solo mueve bytes.
//   - RequestHandler (main loop): dado (cmd, payload) produce la respuesta. Aca
//     viven los comandos de la aplicacion (login, mediciones, etc.).
//
// Protocolo (compatible con controller_link):
//   0x10 (W): request  [cmd][len][payload]
//   0x11 (R): response [status][len][data][filler]   status: 0 Idle,1 Proc,2 Ready,3 Err
//   resto de direcciones (R): registros inmediatos pre-cargados por la app.
//
// Ver docs/protocol-design.md.
// ============================================================================
class ControllerSlave : public I2cSlave
{
    public:
        // Interfaz de la semantica de comandos. La implementa la aplicacion.
        // Corre siempre en el main loop (via process()), nunca en la ISR.
        class RequestHandler
        {
            public:
                virtual ~RequestHandler() = default;

                // Procesa un request. Escribe hasta maxLen bytes de respuesta en `resp`
                // y devuelve cuantos escribio. Setea isError=true para reportar error
                // (por convencion `resp` lleva un mensaje ASCII).
                virtual uint8_t handle(uint8_t cmd,
                                       const uint8_t* payload, uint8_t payloadLen,
                                       uint8_t* resp, uint8_t maxLen,
                                       bool& isError) = 0;
        };

        static constexpr uint8_t MAX_PAYLOAD = 64;   // request/response payload maximo
        static constexpr uint8_t REG_COUNT   = 64;   // espacio de registros inmediatos (0x00..0x3F)

        // Registros reservados por el protocolo (no usar para datos inmediatos).
        static constexpr uint8_t REG_REQUEST  = 0x10;
        static constexpr uint8_t REG_RESPONSE = 0x11;

        explicit ControllerSlave(RequestHandler& handler) : handler(handler) {}

        // ================= ISR-side (llamados desde la interrupcion) =================

        void onAddressMatch(Direction direction) override
        {
            byteIndex = 0;
            readingResponse = false;
            if(direction == Direction::RX)
                pointerSet = false;   // fase de escritura: primero llega el puntero de registro
        }

        void onWriteByte(const uint8_t data) override
        {
            if(!pointerSet)
            {
                regPointer = data;
                pointerSet = true;
                byteIndex = 0;
                writingRequest = (regPointer == REG_REQUEST);
                if(writingRequest)
                    inboxLen = 0;
                return;
            }

            if(writingRequest && inboxLen < sizeof(inbox))
                inbox[inboxLen++] = data;
            // escrituras a registros de solo-lectura: se ignoran
        }

        uint8_t onReadByte() override
        {
            const uint16_t i = byteIndex++;

            if(regPointer == REG_RESPONSE)               // 0x11: [status][len][data][filler]
            {
                readingResponse = true;
                if(i == 0) return cmdStatus;
                if(i == 1) return respLen;
                if(i >= 2 && (i - 2) < respLen) return respData[i - 2];
                return 0x00;                             // filler
            }

            const uint16_t idx = regPointer + i;         // registros inmediatos
            return (idx < REG_COUNT) ? registers[idx] : 0x00;
        }

        void onEndTransaction() override
        {
            if(writingRequest)
            {
                writingRequest = false;
                // Request completo -> pedir procesamiento, salvo que ya haya uno en curso (F5).
                if(inboxLen >= 2 && cmdStatus != Processing)
                {
                    requestPending = true;
                    cmdStatus = Processing;
                }
            }
            else if(readingResponse && (cmdStatus == Ready || cmdStatus == Error))
            {
                // El master leyo la respuesta -> la lectura es el ack: volver a Idle.
                cmdStatus = Idle;
            }

            pointerSet = false;
            writingRequest = false;
            readingResponse = false;
            byteIndex = 0;
        }

        void onError() override
        {
            if(writingRequest && cmdStatus != Processing)
                inboxLen = 0;   // descartar request parcial
            pointerSet = false;
            writingRequest = false;
            readingResponse = false;
            byteIndex = 0;
        }

        // ===================== Main-loop side =====================

        // Procesa el request pendiente delegando en el RequestHandler. Llamar en el loop.
        void process()
        {
            if(!requestPending)
                return;
            requestPending = false;

            const uint8_t cmd         = inbox[0];
            const uint8_t declaredLen = inbox[1];
            const uint8_t availLen    = (inboxLen >= 2) ? (uint8_t)(inboxLen - 2) : 0;
            const uint8_t payloadLen  = (declaredLen <= availLen) ? declaredLen : availLen;

            bool isError = false;
            uint8_t n = handler.handle(cmd, &inbox[2], payloadLen,
                                       respData, MAX_PAYLOAD, isError);
            if(n > MAX_PAYLOAD)
                n = MAX_PAYLOAD;

            publishBarrier();           // respData/respLen visibles ANTES de publicar el status
            respLen   = n;
            cmdStatus = isError ? Error : Ready;
        }

        // --- API de registros inmediatos (la app los pre-carga desde el main loop) ---
        void setRegister(uint8_t addr, uint8_t value)
        {
            if(addr < REG_COUNT)
                registers[addr] = value;
        }

        void writeRegisters(uint8_t addr, const uint8_t* src, uint8_t len)
        {
            for(uint8_t i = 0; i < len && (addr + i) < REG_COUNT; i++)
                registers[addr + i] = src[i];
        }

    protected:
        enum Status : uint8_t { Idle = 0, Processing = 1, Ready = 2, Error = 3 };

        RequestHandler& handler;

        // --- Mailbox (compartido ISR <-> main loop) ---
        volatile uint8_t  cmdStatus = Idle;              // byte 0 de la respuesta 0x11
        volatile uint8_t  respLen   = 0;
        uint8_t  respData[MAX_PAYLOAD] = {};
        uint8_t  inbox[2 + MAX_PAYLOAD] = {};             // [cmd][len][payload]
        volatile uint16_t inboxLen = 0;
        volatile bool     requestPending = false;
        uint8_t  registers[REG_COUNT] = {};               // registros inmediatos

        // --- Estado por-transaccion (solo ISR) ---
        uint8_t  regPointer = 0;
        bool     pointerSet = false;
        bool     writingRequest = false;
        bool     readingResponse = false;
        uint16_t byteIndex = 0;

        // Barrera de compilador: en Cortex-M monocore alcanza para ordenar la publicacion
        // del outbox antes del status.
        static inline void publishBarrier() { asm volatile("" ::: "memory"); }
};
