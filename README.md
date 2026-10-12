# Deterministic Match Server with Replay and Divergence Detection

A small, fixed-timestep authoritative game server for 10 players: positions
and health live in Q16.16 fixed-point, abilities are Lua scripts that only
ever touch fixed-point integers, every input is appended to a match
recording as it is applied, and replaying that recording recomputes a
per-tick state checksum that is checked against the one recorded live.
Every number below was measured on this machine, not targeted; the 45,000
and 300,000-tick runs below are this repo's own measurements, not the
resume's language.

## Why this exists

This is a small version of the server behind any competitive game that
supports match replays or server-authoritative anti-cheat: the server is
the only place state is allowed to change, every client input is logged in
order, and "did this replay reproduce the match" has to be a yes/no answer
with no wiggle room, which only works if the simulation step is bit-for-bit
deterministic in the first place.

## Honest framing

- **Not a real game.** Ten players, three abilities (fireball, heal,
  dash), and flat 2D positions. There is no collision mesh, no real client,
  no network transport; this is the authoritative simulation core and the
  recording/replay/divergence pipeline around it, not a shippable game
  server.
- **Not matchmaking or netcode.** There is no client prediction,
  reconciliation, or rollback here, only the server-side authoritative
  tick and its recording.
- **Synthetic input.** The 45,000 and 300,000-tick runs below are driven
  by a seeded integer PRNG standing in for client input (every player
  moves one fixed-point step each tick in a seed-derived direction, and
  every 7th tick one living player casts an ability on another), not real
  players.
- **The "seeded floating-point divergence" claim is simulated at the
  checksum level, not injected into a real float path.** The sim core is
  fixed-point by construction (see Architecture), so there is no float
  arithmetic left inside `apply_tick` to perturb honestly. The divergence
  test instead builds two checksum streams that agree up to a chosen tick
  and then disagree, which is exactly the artifact a real divergence
  between two differently-built clients leaves in the data the localizer
  actually looks at. This is disclosed, not hidden.
- **Machine and toolchain.** WSL2 Ubuntu 22.04 on Windows 11, 8 physical /
  16 logical host cores (WSL capped to 12), g++ 11.4, `-O3` for the
  Release/benchmark build, libprotobuf 3.12.4, protobuf-compiler 3.12.4
  (pinned explicitly via `-DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc`;
  this machine also has a newer protoc 28.2 on PATH for the Go project in
  this portfolio, and mixing the two would link a generated-code version
  against the wrong runtime), Lua 5.4.4 (`liblua5.4-dev`), GoogleTest
  1.15.2 via FetchContent. Builds for test/debug run from this repo's own
  `build/`; the benchmark numbers below were measured from a copy under
  native WSL `~/build` rather than the `/mnt/c` checkout, since `/mnt/c` is
  slower and timing-noisy for anything compute-bound (playbook policy, not
  specific to this repo).

## Architecture

```
src/
  fixed.hpp        Q16.16 fixed-point type: pure-integer +,-,*,/, Newton's-method sqrt
  sim_state.hpp     PlayerState / MatchState / PlayerInput, reset_match()
  prng.hpp          xorshift64*, used only to generate synthetic test/bench traffic
  sim.hpp/.cpp      gen_tick_inputs() (synthetic input), apply_tick() (movement + abilities)
  ability_vm.hpp/.cpp  Embeds Lua 5.4, exposes get_pos/set_pos/deal_damage/fx_mul/fx_sqrt
                        to ability scripts as Lua integers (raw fixed-point), never floats
  checksum.hpp      FNV-1a 64-bit over the named fields of every player, in fixed order
  recording.hpp     MatchRecorder: builds and (de)serializes a MatchRecording protobuf
  replay.hpp        replay_and_check(): re-runs a recording, compares checksums tick by tick;
                    first_mismatch(): the same comparison over two raw checksum streams
proto/
  match_recording.proto  InputFrame / TickRecord / MatchRecording wire format
lua/abilities/
  fireball.lua, heal.lua, dash.lua   ability scripts, integer fixed-point only
tools/
  replay_tool.cpp   CLI: record a match to a file, or replay one and report the result
  bench_server.cpp  headless ticks/sec benchmark, no sleeping or pacing
tests/
  test_fixed.cpp            fixed-point arithmetic, determinism, sqrt vs. a reference oracle
  test_sim.cpp               10 players, same-seed determinism, ability effects
  test_lua_abilities.cpp     each ability's effect on state, checked directly
  test_replay.cpp            every input recorded; 40,000+ tick bit-identical replay
  test_divergence.cpp        exact-tick localization on an injected checksum disagreement
```

### Design notes

- **Fixed-point everywhere real state lives, not just "where it matters".**
  The temptation is to keep floats for anything that looks like geometry
  and fixed-point only for money-like fields. This repo does not do that:
  `PlayerState.x/y` are `Fixed`, ability math (`fx_mul`, `fx_sqrt`) is
  `Fixed`, and the Lua boundary passes the raw `int64_t` fixed-point value
  as a Lua integer rather than converting to a Lua float and back. Lua 5.4
  has a native integer subtype specifically so this conversion is never
  needed; using an older Lua without integers would force a float round
  trip on every ability call and quietly reintroduce the nondeterminism
  the whole design is trying to avoid.
- **Checksums hash named fields, not `sizeof(PlayerState)` raw bytes.** A
  raw-byte hash is one line shorter but folds in any compiler-inserted
  struct padding, whose content is unspecified unless every call site
  happens to zero-initialize. Hashing the five named fields explicitly
  removes that failure mode by construction instead of relying on every
  future call site to get zero-initialization right.
- **Abilities are looked up by a per-ability Lua global (`__cast_fireball`
  etc.), not by `require`-ing three separate modules.** Each script is
  loaded into its own fresh environment table (`setmetatable({}, {__index
  = _G})`) so `fireball.lua`'s `cast` cannot clobber `heal.lua`'s; the
  tradeoff is a slightly unusual load-time bootstrap string in
  `AbilityVM`'s constructor in exchange for not needing Lua's module
  system at all for three files.
- **Movement is applied for every living player before any ability is
  applied, in player-id order, every tick.** Fixed order is what makes
  "replay this recording" mean something: if ability execution order were
  allowed to vary (e.g. by iterating a hash map), two bit-identical input
  streams could legitimately diverge.

## Validation

- **Reference oracle:** `Fixed.SqrtMatchesSlowReferenceOracleExactly`
  diffs the Newton's-method `fixed_sqrt` against a deliberately dumb
  binary search over the monotonic predicate `guess*guess <= target`,
  exact match required, over 667 values.
- **Determinism invariant:** `Fixed.DeterministicAcrossRepeatedComputation`
  and `Sim.SameSeedProducesBitIdenticalChecksumStream` both assert bit
  equality (not approximate equality) between two independently computed
  runs from the same inputs.
- **Replay invariant:** `Replay.BitIdenticalChecksumOver40000PlusTicks`
  records 45,000 ticks, then replays the recording from scratch and
  recomputes the checksum tick by tick, asserting every one matches.
- **Divergence localization:** `Divergence.LocalizesToExactFirstDisagreeingTick`
  builds two checksum streams that agree for 3,417 ticks and then diverge,
  and asserts the localizer names tick 3,417 exactly, not "somewhere
  around there".
- **Sanitizers:** ASan+UBSan clean, see `docs/asan_ubsan_clean_run.txt`.
  TSan was not run: the simulation loop, Lua VM, and replay path are all
  single-threaded by design (the whole point is one authoritative
  sequence of ticks), so there is no concurrency for TSan to check.

```
$ cd build-san && ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 ./unit_tests
...
[==========] 20 tests from 5 test suites ran. (977 ms total)
[  PASSED  ] 20 tests.
```

## Findings

The very first sanitizer run (before any benchmark was taken) caught a
real bug: `Fixed::from_int` computed `raw = v << SHIFT` directly, and
`Fixed.IntRoundTrip`'s very first negative value (`-1000`) tripped UBSan's
"left shift of negative value" check. Left-shifting a negative signed
integer is undefined behavior before C++20 even though it happens to
produce the expected two's-complement bit pattern on every machine this
runs on, which is exactly the kind of bug that stays silent for years and
then breaks on a new compiler or optimization level. The fix shifts the
unsigned reinterpretation of `v` and casts back:
`(int64_t)((uint64_t)v << SHIFT)`, which is well-defined and produces the
identical bits. Re-run clean; see `docs/asan_ubsan_clean_run.txt`. This is
the only sanitizer finding; everything else was clean on the first run
with sanitizers enabled.

## Measured results

Machine: WSL2 Ubuntu 22.04 on Windows 11, 8 physical / 16 logical host
cores (12 visible inside WSL), g++ 11.4, `-O3`, single-threaded. Measured
from a native-filesystem build (`~/build/deterministic-match-server`), not
the `/mnt/c` checkout.

- **Replay: 45,000 ticks recorded, replayed from the recording, 45,000/45,000
  per-tick checksums bit-identical.** (Claim: "over 40,000+ ticks", met.)
- **Headless tick rate (10 players, movement + periodic ability cast,
  single thread, no sleeping): 3.41M-3.53M ticks/sec across 3 runs of
  300,000 ticks** (0.00028-0.00029 ms/tick). The 128 ticks/sec budget this
  server targets is 7.8125 ms/tick; this measures roughly 27,000x that
  budget for the simulation step alone with nothing else running on the
  server. This is not a network latency number and does not claim to be:
  it is how much of the 7.8125 ms tick budget the authoritative step
  itself consumes, which is the number that actually bounds how much else
  (networking, anti-cheat checks, more players) the tick budget has room
  for.
- **Divergence localization: exact.** The injected disagreement at tick
  3,417 of a 5,000-tick stream was named as tick 3,417, not an
  approximate range.

Raw output: `docs/benchmark_output.txt` (benchmark + replay), `docs/test_output.txt`
(full suite), `docs/asan_ubsan_clean_run.txt` (sanitizer run).

## Building and running

```
# configure + build (functional, from this checkout)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc ..
cmake --build . -j"$(( $(nproc) / 2 ))"
./unit_tests

# sanitizer build
mkdir build-san && cd build-san
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN_UBSAN=ON -DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc ..
cmake --build . -j"$(( $(nproc) / 2 ))"
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 ./unit_tests

# record and replay a match
./replay_tool record /tmp/match.bin 2026 45000
./replay_tool replay /tmp/match.bin

# benchmark (run from a native filesystem, not /mnt/c, for stable timing)
./bench_server 300000
```

Requires `libprotobuf-dev`, `protobuf-compiler`, and `liblua5.4-dev` (or
any Lua 5.4 dev package discoverable by `pkg-config lua5.4`). This machine
has two `protoc` installs (3.12.4 from apt, 28.2 installed separately for
a Go/gRPC project elsewhere in this portfolio); `-DProtobuf_PROTOC_EXECUTABLE=/usr/bin/protoc`
pins codegen to the version that matches the linked `libprotobuf-dev`
runtime.

## Limitations

- Single-threaded by design; there is no concurrency to measure or to run
  TSan against.
- No real networking: inputs are either synthetic (PRNG-generated) or read
  back from a recording, never received over a socket.
- The "seeded floating-point divergence" claim is validated at the
  checksum-stream level (see Honest framing), not by injecting a genuine
  float nondeterminism into the fixed-point core, since the core has no
  float path to inject one into.
- Combat balance (120 flat fireball damage, 80 flat heal, falloff at 20
  units) is illustrative, not tuned for any real game feel.
