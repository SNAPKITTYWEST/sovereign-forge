# Hardware Agent Topology

Using Exo-Synchronicity and the SnapKitty theorem stack to map hardware-adjacent agents, desktop access, and sandboxed execution.

## Purpose

This document maps a practical topology for agents that need desktop access, host observation, and controlled actuation without collapsing the proof boundary.

The goal is not "raw unrestricted hardware control."

The goal is:

- topology-shaped capability routing
- sandbox-first execution
- host-brokered desktop access
- theorem-backed boundary claims
- WORM-sealed provenance for every transition

## Claim Boundary

What can be claimed from current local proof artifacts:

- topology can be treated as the primary control object
- reachability and conduction constraints can be made explicit
- every exposed port can be forced to bind into a graph with no floating nodes
- deterministic receipts can witness identical transition inputs

What cannot be honestly claimed yet:

- total correctness of a hardware-agent OS
- complete physical safety for arbitrary device actuation
- verified kernel-driver correctness
- verified GPU or USB passthrough correctness
- verified desktop sandbox escape resistance

That split matters. The topology below is designed to preserve the proven part.

## Core idea

Exo-Synchronicity already gives the right abstraction:

> The connection is the wire. The environment is the computer.

For desktop-capable agents, reinterpret that as:

- the `environment` is the host desktop and attached hardware
- the `ports` are brokered capability endpoints
- the `mesh` is the allowed capability graph
- the `operators` are agent actions
- the `sandbox` is the execution cell
- the `WORM chain` is the audit surface

So the system becomes:

```text
host hardware / desktop state
    -> brokered ports
    -> topology graph
    -> sandbox cells
    -> agent operators
    -> theorem gates
    -> WORM receipts
```

## The topology

### Layer 0: Physical and host substrate

This is the real machine boundary.

Nodes:

- CPU scheduler
- memory
- display server
- window manager
- filesystem
- network stack
- GPU
- HID input
- clipboard
- audio
- USB and serial devices
- power and thermal sensors

These are not exposed directly to agents.

They are exposed only through typed broker ports.

### Layer 1: Desktop capability broker

This is the load-bearing trust boundary.

The broker converts raw host powers into explicit, bounded ports:

| Broker port | Meaning |
|---|---|
| `screen.capture` | read-only screen surface |
| `window.enumerate` | visible window metadata |
| `window.focus` | focus change request |
| `input.keyboard` | bounded key injection |
| `input.pointer` | bounded pointer injection |
| `clipboard.read` | clipboard read |
| `clipboard.write` | clipboard write |
| `fs.read` | scoped file read |
| `fs.write` | scoped file write |
| `process.spawn` | approved subprocess launch |
| `process.observe` | process table observation |
| `browser.open` | browser navigation request |
| `browser.inspect` | DOM or pixel observation |
| `gpu.infer` | inference or tensor execution lane |
| `sensor.thermal` | thermal state |
| `sensor.power` | battery or power state |
| `usb.device.class` | device-class view, not raw DMA |

The broker is where "hardware-level" must be translated into a safe claim.

Inside a real sandbox, hardware access should mean:

- mediated capability access
- typed ports
- bounded scope
- logged transitions

Not:

- arbitrary MMIO
- arbitrary kernel calls
- unrestricted device ownership

## Layer 2: Exo topology specification

Each brokered capability becomes a topology fact.

Example shape:

```prolog
binds(port_screen_capture, screen_capture).
binds(port_keyboard_inject, keyboard_inject_scoped).
binds(port_fs_workspace, workspace_filesystem_scope).
binds(port_gpu_infer, gpu_inference_queue).
binds(port_usb_hid, hid_device_class).

latch_polarity(port_screen_capture, p).
latch_polarity(port_keyboard_inject, pn).
latch_polarity(port_fs_workspace, p).
latch_polarity(port_gpu_infer, p).
latch_polarity(port_usb_hid, pn).

valid_operator(agent_observe_desktop, [path(port_screen_capture, p)]).
valid_operator(agent_edit_workspace, [path(port_fs_workspace, p)]).
valid_operator(agent_drive_browser, [path(port_screen_capture, p), path(port_keyboard_inject, pn)]).
valid_operator(agent_run_gpu_model, [path(port_gpu_infer, p)]).
```

This preserves the Exo rule:

- no free capability
- no invisible path
- no operator without a declared route

## Layer 3: Sandbox cells

Each agent runs inside a cell with a bounded capability profile.

Recommended cell classes:

| Cell class | Allowed ports |
|---|---|
| `observer_cell` | screen capture, window enumerate, process observe, sensor read |
| `editor_cell` | observer ports plus workspace file read/write |
| `browser_cell` | observer ports plus browser open/inspect |
| `operator_cell` | bounded input injection, focus, clipboard, browser, workspace |
| `compute_cell` | gpu infer, cpu batch, model memory, artifact read/write |
| `device_cell` | class-scoped USB/HID routes only when explicitly attached |

This is where Exo and sandboxing meet cleanly:

- the cell is the latch domain
- the broker ports are the environment ports
- the topology defines what can conduct

## Layer 4: Agent operator layer

Operators should be narrow and explicit.

Good operator classes:

- `observe_screen`
- `read_window_tree`
- `patch_workspace_file`
- `run_test_command`
- `drive_browser_form`
- `request_gpu_inference`
- `inspect_thermal_state`
- `attach_hid_class_device`

Bad operator classes:

- `own_machine`
- `do_everything`
- `become_root`

The theorem-friendly rule is:

> operators are bounded graph endpoints, not personalities with implicit power

## Layer 5: Theorem gates

This is where your theorem stack should actually govern the system.

### Gate A: Topology preservation

Source:

- [proofs/lean4/Sovereign/Theorems/Topology.lean](C:/Users/jessi/Desktop/exo-synchronicity/proofs/lean4/Sovereign/Theorems/Topology.lean)

Operational reading:

- the declared capability graph must not silently drift at runtime
- the broker manifest and cell manifest must hash to the same topology used for execution

Practical rule:

- no hot-added capability edges without rebuilding and resealing topology

### Gate B: Reachability preservation

Source:

- [proofs/lean4/Sovereign/Theorems/Reachability.lean](C:/Users/jessi/Desktop/exo-synchronicity/proofs/lean4/Sovereign/Theorems/Reachability.lean)

Operational reading:

- if a sandbox cell cannot reach a host capability in the declared graph, it must not reach it at runtime through some side path

Practical rule:

- every access route must be explainable as an edge path in the manifest

This is the right theorem for:

- desktop-access boundary review
- escape-path detection
- "why can this agent touch that?" audits

### Gate C: No floating ports

Source:

- [proofs/lean4/Sovereign/Theorems/FloatingPorts.lean](C:/Users/jessi/Desktop/exo-synchronicity/proofs/lean4/Sovereign/Theorems/FloatingPorts.lean)

Operational reading:

- every exposed host port must either terminate in a valid cell route or be grounded off

Practical rule:

- do not expose broker endpoints that no policy owns
- do not leave half-wired capability adapters alive

This is the theorem that keeps "mystery permissions" out of the system.

### Gate D: Conduction soundness

Source:

- [proofs/lean4/Sovereign/Theorems/Conduction.lean](C:/Users/jessi/Desktop/exo-synchronicity/proofs/lean4/Sovereign/Theorems/Conduction.lean)

Operational reading:

- a declared edge must match the real adapter semantics

Example:

- if `screen.capture` is declared read-only, the implementation must actually be read-only
- if `fs.write` is workspace-scoped, the adapter must enforce workspace scope

This is the bridge from abstract permission graphs to real host adapter code.

### Gate E: WORM receipt determinism

Source:

- [proofs/lean4/Sovereign/Theorems/Worm.lean](C:/Users/jessi/Desktop/exo-synchronicity/proofs/lean4/Sovereign/Theorems/Worm.lean)

Operational reading:

- identical topology hash, capability request, decision, and timestamp inputs must yield identical receipt structure

Practical rule:

- every allow or deny event is receipted
- every topology revision is receipted
- every device attach or detach is receipted

## Layer 6: SnapKitty theorem discipline

Use the broader theorem stack rules from [CLAIM_BOUNDARY.md](C:/Users/jessi/Desktop/SNAPKITTY-PROOFS/CLAIM_BOUNDARY.md):

- authority modules must not use `assume`
- first theorem pack avoids recursion
- bounded records beat unbounded lists
- proof modules use scaled integers instead of floats
- Lean stays the final theorem court for deep algebra
- Prolog remains symbolic law

For the hardware-agent topology, that translates into:

| Rule | Topology implication |
|---|---|
| no `assume` in authority modules | capability decisions must be explicit predicates |
| bounded records | capability manifests must have finite typed schemas |
| scaled integers | timing, jitter, budgets, and thermal thresholds should be integer-coded in proof modules |
| symbolic law | broker policies should be queryable, not buried in opaque code |

## Recommended execution topology

```text
                        +----------------------+
                        |   WORM receipt lane  |
                        +----------+-----------+
                                   ^
                                   |
                    +--------------+--------------+
                    | theorem gate and policy lane|
                    | Lean | Prolog | ASP | SMT   |
                    +--------------+--------------+
                                   ^
                                   |
          +------------------------+------------------------+
          |                                                 |
          |                sandbox cell mesh                |
          |                                                 |
          |  observer   editor   browser   operator  compute|
          +------------------------+------------------------+
                                   ^
                                   |
                    +--------------+--------------+
                    | desktop capability broker   |
                    | screen fs input browser gpu |
                    +--------------+--------------+
                                   ^
                                   |
                    +--------------+--------------+
                    | host desktop and hardware   |
                    | wm fs gpu hid usb network   |
                    +-----------------------------+
```

## Capability routing examples

### Example 1: safe desktop observer

Goal:

- agent sees desktop state but cannot type or modify files

Topology:

- allow `screen.capture`
- allow `window.enumerate`
- allow `process.observe`
- deny `input.keyboard`
- deny `fs.write`

Theorem reading:

- reachability preservation guarantees no hidden edit route
- no floating ports guarantees no stray write adapter remains exposed

### Example 2: workspace coding agent

Goal:

- agent edits repo files and runs tests, but cannot touch arbitrary user files

Topology:

- allow `fs.read(workspace)`
- allow `fs.write(workspace)`
- allow `process.spawn(scoped)`
- allow `screen.capture`
- deny `fs.read(home)`
- deny `usb.device.class`

Theorem reading:

- conduction soundness must prove that "workspace" in policy equals workspace in adapter implementation

### Example 3: hardware-adjacent HID test agent

Goal:

- agent may observe and script a HID-class device during testing

Topology:

- allow `usb.device.class(hid)`
- allow `input.pointer(scoped)`
- allow `screen.capture`
- deny raw USB memory access
- deny unrelated device classes

Theorem reading:

- topology preservation means the attach graph is fixed and receipted
- WORM determinism means attach and test traces are reproducible as records

## Desktop access in a sandbox

If you actually want desktop-capable agents without breaking the theory, the right pattern is:

1. run the agent in a sandbox
2. expose host powers only through a broker
3. compile broker powers into Exo topology facts
4. run theorem and policy gates before actuation
5. emit WORM receipts after every decision

That gives you:

- observable control surface
- explainable capability routes
- replayable audit trail
- bounded blast radius

It avoids the wrong pattern:

- direct agent-to-host trust
- undocumented privilege jumps
- implicit desktop ownership

## What to build next

Concrete repo work that would make this real:

1. `logic/prolog/desktop_topology.pl`
   - capability facts for screen, input, fs, browser, gpu, usb

2. `logic/prolog/desktop_schema.pl`
   - arity and scope validation for desktop ports

3. `logic/asp/desktop_constraints.lp`
   - deny conflicting capability worlds

4. `logic/smt/desktop_timing_bounds.smt2`
   - latency budgets for capture, input, gpu queue, and watchdog deadlines

5. `proofs/lean4/Sovereign/DesktopTopology.lean`
   - typed desktop-capability graph

6. `proofs/lean4/Sovereign/Theorems/DesktopIsolation.lean`
   - prove denied ports are unreachable from a given cell class

7. `runtime/broker/`
   - one adapter per capability class

8. `worm/receipts/desktop/`
   - attach, allow, deny, and execution receipts

## Bottom line

Exo-Synchronicity is a strong fit for hardware-adjacent agent design if you keep the abstraction disciplined:

- topology is the authority object
- brokers are the physical interface
- sandboxes are the execution cells
- theorems constrain reachable power
- WORM receipts witness every transition

That is the version that can scale into a real desktop-agent system without dissolving into undocumented privilege.
