# Mota route simulator

This is a small research release for tower 51 of the H5 Mota game. It contains
the typed C++ simulator, the native route-search executable, the pinned exported
world data, and one verified route witness.

The current promoted witness finishes **特性版-TE（生命）** with **49,409 HP**
after **4,438 game inputs**. A fresh replay in the original game passed with
this route and no replay errors. The checked record is 50,675 HP, so beating it
requires 50,676 HP; this witness is 1,267 HP short. No leaderboard submission
was made, and the witness is not an optimality claim.

## Build and replay

A C++20 compiler and `make` are the only build requirements:

```sh
make build
make verify
```

`make verify` runs `build/mota-native-replay` on the included input. The replay
should report one initialization, 4,438 applied actions, `typed-native`,
`won50=true`, `native_win_reason=特性版-TE（生命）`, and final HP 49,409.
The same command can be run directly:

```sh
build/mota-native-replay world solution/49409-input.json
```

`build/mota-native-replay` is the maintained typed simulator. `build/mota-goal-route`
is the route-level LNS search used in the final improvement; its optional config
and parent interaction route are in `search/`. Searches are heuristic and may
run for minutes. They do not establish optimality.

To run the retained search configuration, use a new output directory:

```sh
build/mota-goal-route world solution/49409-input.json search/lns-config.json results/lns
```

## Contents

- `cpp/` — simulator, continuous native replay, and route-search source.
- `world/` — the 51-floor exported data pinned by the simulator.
- `solution/49409-input.json` — the complete native initial state and route.
- `solution/49409.h5route` — the packed replay accepted by the original game.
- `solution/manifest.json` — hashes and compact verification status.
- `METHOD.md` — how the witness was obtained and what each check establishes.
- `NOTICE.md` — upstream attribution and redistribution limits.

The release intentionally omits browser captures, compute logs, private paths,
historical experiments, build products, and the local game source checkout.
The native simulator supports the pinned world and route; unsupported event
commands are rejected rather than silently approximated.
