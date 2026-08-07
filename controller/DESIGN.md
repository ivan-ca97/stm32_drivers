# Controller — design & rationale

What the controller does and, above all, **why** each design decision was made this way. How it
is integrated into a concrete system (buses, who uses it) lives in that system's documentation,
not here.

---

## 1. Purpose & guiding principle

`ControllerSlave` is the **slave endpoint** of a register + request/response protocol over I2C.
It turns the raw byte callbacks of the `I2cSlave` driver into two abstractions: **immediate
registers** and **deferred commands**. The command semantics are supplied by an injected
`RequestHandler`.

**Guiding principle:** the I2C ISR is **O(1) and deterministic**; all the heavy work (lookups,
validation, application logic) lives in the **main loop**. This principle is the root of almost
every decision below.

---

## 2. The two communication modes

| Mode | Path | For | Why |
|------|------|-----|-----|
| **Immediate (sync)** | the master reads a register and gets a value **pre-loaded** by the app | fast-changing state that must be readable at any time | always-fresh data, **zero** processing in the ISR |
| **Deferred (async)** | the master writes `[cmd][len][payload]`; the ISR **only buffers**; the main loop processes; the master **polls** the response | operations that need lookups / validation / work that **cannot** run in the ISR | keeps the ISR O(1) |

**Why two modes:** some state must be read instantly and with no latency; routing it through the
deferred cycle would be slow and pointless. Commands, on the other hand, cannot be resolved inside
the ISR without violating the guiding principle. Separating them gives both properties at once.

---

## 3. Shared register bank

A **single register bank** (the immediate registers) is the **current snapshot of the state**. It
is **written only by the main loop**; readers only read. Deferred commands that change state
**mutate the bank** via the main loop.

**Why a single source of truth:** it avoids duplicating/synchronizing state. Everyone who reads
sees the same thing. And since the **only writer is the main loop**, the concurrency discipline
becomes simple (§9).

### Windows: who may write where

The bank is one flat space, but its writers are independent. Each one takes a **window** — a
bounded view `[base, base+length)` — and publishes at window-local offsets; a write outside it is
dropped, so a writer cannot reach another's registers. Readers still see the whole bank: the
window scopes writing, not reading.

Bounding each writer to its own span only means something if the spans cannot overlap, so a
window is **claimed from the bank**, never constructed directly. The bank keeps the reservations
and refuses one that collides with a span already handed out — the same treatment opcode ranges
get from the router (§5). A bad base is then a start-up failure instead of two writers silently
corrupting each other.

---

## 4. Instances & read-only mode

The controller can be **instantiated once per I2C bus**, and several instances can **share the
same bank + router + sections**. An instance can be configured **read-only**: it still lets a
master select the register pointer to read, but it **ignores writes** to the request register and
to writable registers.

**Why read-only is a first-class mode:** if every writing master goes through a single
full-access instance, the **deferred mailbox is left with a single writer**, which removes
multi-master request/response correlation (no request id needed). *How* instances/read-only map
onto a product's buses is an integration concern.

---

## 5. Multi-section dispatch (handler registry)

A single slave receives **all** the deferred commands, but the functional areas are
**independent sections**, each its own `RequestHandler`. A **router** — which itself implements
`RequestHandler`, so `ControllerSlave` still sees "just one" — routes each `cmd` to the owning
section:

```
ControllerSlave.handle(cmd, ...) → Router
        ├─ section A claims its opcodes → HandlerA.handle(...)
        ├─ section B claims its opcodes → HandlerB.handle(...)
        └─ nobody claims it             → ErrUnknownCommand
```

**Mechanics:** each section **declares which opcodes it serves**; the router keeps an
`[opcodes → handler]` table and delegates. Overlapping ranges are rejected at registration.

**Why a registry (and not a fixed range or a section byte):**
- vs a **range hardcoded in the router**: it does not couple the router to each section's range;
  adding a section does not touch the router.
- vs an **explicit section byte** (`[section][cmd]...`): it does not change the wire format; the
  `cmd` stays a single byte.

Each section is **independent and testable** on its own.

---

## 6. Async handshake: status polling (single writer)

The master learns its result is ready via the **status byte** of the response mailbox:

```
master writes request (0x7E) → status = Processing
master polls status (0x7F)   ← Processing...
main loop finishes           → status = Ready | Error
master reads response        ← [status][len][data]
the read = ACK               → status back to Idle
```

The master correlates **by sequence** (it knows which command it sent); with a single writing
master there is no ambiguity about the origin.

**Why polling and not multi-master with async push + id-token:** push would require a
**request id-token** so the initiator could tell responses apart. It is only worth it if there is
a **latency-critical async flow**; when there isn't, polling is simpler and needs no id. If one
appears, a **"data-ready" GPIO** can be added as a targeted optimization, without reworking the
model. *(Whether a product has such a flow is an integration concern.)*

---

## 7. In-flight requests: single slot + Busy

With a single writing master doing *request → poll → read* there is **at most one pending
request**. A **single request slot** is used; if a second request arrives before the first's
response is read, it is **dropped** — the in-flight one is left intact (it is not silently
overwritten; the status simply stays `Processing`).

**Why not a FIFO:** a deep queue would only help with **pipelining** (several requests in flight),
and that would require the id-tokens discarded in §6. The door is left open to a small FIFO if
pipelining ever appears.

---

## 8. Bank consistency & freshness: the application's responsibility

The bank is a **byte store**. Keeping a **multi-byte** read consistent (so the reader does not mix
old/new if the main loop updates in the middle) and signalling **freshness** (whether there is new
data) **is not the controller's job**: it belongs to the protocol's **application**, which decides
where the data comes from and with what policy it is published.

The recommended pattern is a **seqlock**, implementable **on top of the current primitives**
without touching the base class: the app writes a sequence register and the data in order (via
`setRegister`/`writeRegisters`), and the master reads `seq → data → seq` and retries if it changed.
That same counter gives **consistency and freshness** in a single piece (a snapshot in the ISR, by
contrast, neither respects the layering nor distinguishes "I read the same data twice"). See how
the reactor does it in the integration doc.

---

## 9. Concurrency (mailbox)

Actors: **one ISR per bus instance** and the **main loop**. On a single-core Cortex-M
"concurrency" is **preemption by interrupt**.

- **Bank:** only the main loop writes; the ISRs only **read**. An aligned word store (16/32 bits)
  is atomic on the M4, so a single value never tears. Multi-byte read consistency is app-layer
  (§8).
- **Deferred mailbox:** touched only by the writing instance + the main loop, with the status FSM
  and a compiler barrier (`publishBarrier`) that orders the publication of the outbox **before**
  the status.

---

## 10. Robustness

- **Malformed request** (short payload / missing terminator) → status `Error`, never hangs.
- **Unknown command** → `ErrUnknownCommand`.
- **Bus error** mid-transaction → the driver auto-recovers; the controller **resets its
  per-transaction state** in `onError`.
- **Response longer than the buffer** → it is truncated.

---

## 11. Out of scope

The controller is the **slave endpoint**. The **master side** (this device initiating transfers to
others) is outside the controller and goes through the driver's master path.

---

## 12. Decisions summary

| Decision | Choice | Why | Rejected alternative |
|----------|--------|-----|----------------------|
| Work in the ISR | O(1), defer the heavy work | deterministic ISR, do not block the bus | process in the ISR |
| Modes | immediate + deferred | instant reads vs commands with processing | a single path for everything |
| State | single shared bank, single writer | one source of truth; simple concurrency | state per section/instance |
| Read-only mode | first-class read-only instance | lets the writers collapse to one (removes multi-master correlation) | full-access instances only |
| Multi-section dispatch | handler registry | does not couple the router; does not change the wire | fixed range / section byte |
| Async result ready | polling + status FSM | simple when there is no latency-critical async flow | multi-master push + id-token |
| In-flight requests | single slot + Busy | one writer ⇒ at most one pending | FIFO (would need pipelining + id) |
| Bank consistency/freshness | app-layer (seqlock over the primitives) | keeps the base a dumb transport; the same counter gives freshness | snapshot in the ISR (puts app policy in the base and gives no freshness) |

---

## 13. Pending

The **concrete** specification is done per product on top of this base: the **register-bank map**,
the **mailbox layout**, and the **opcode/section partition**.
