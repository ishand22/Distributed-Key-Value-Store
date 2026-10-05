# Distributed Key-Value Store

A C++20 educational key-value store with a WAL-backed in-memory engine, sorted snapshot compaction, a reusable worker pool, and a Raft consensus state-machine core.

## Implemented

- Thread-safe put, get, delete, and ordered scan operations.
- Append-only WAL with replay after restart. Keys and values are hex encoded, so tabs, newlines, and arbitrary bytes are supported.
- Sorted snapshot compaction that keeps deletion markers (tombstones) durable.
- Raft terms, election, votes, leader-only log append, peer replication acknowledgements, and majority commit tracking.
- Interactive local node shell and a reusable fixed-size thread pool.

The CLI currently runs in single-node mode. The Raft class exposes consensus state transitions, but peer networking, RPC, persistent Raft metadata/logs, automatic sharding, and replication between running processes are not implemented yet. Treat this as a runnable educational foundation rather than a production-ready distributed service.

## Requirements

- CMake 3.20+
- C++20 compiler
- Threads support

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable is `build/dkv-node` on Linux/macOS and usually `build/Release/dkv-node.exe` with a multi-configuration Windows generator.

## Run

```sh
./build/dkv-node ./data
```

On Windows, run the generated `dkv-node.exe`, optionally passing a data directory. Commands:

```text
put <key> <value>
get <key>
del <key>
scan
compact
status
quit
```

Example:

```text
> put language C++
OK
> get language
C++
> compact
COMPACTED
```

Data is stored in `data/store.wal` and `data/store.sst`; the directory is created automatically.

## Layout

```text
include/dkv/engine.hpp       Storage engine API
include/dkv/raft.hpp         Raft protocol state API
include/dkv/thread_pool.hpp  Worker pool API
src/engine.cpp               WAL, recovery, and compaction
src/raft.cpp                 Election and quorum state transitions
src/thread_pool.cpp          Fixed worker pool
src/main.cpp                 Interactive local node shell
```

## Raft core usage

Construct a `RaftNode` with a node ID and configured cluster size. Call `begin_election`, then report granted peer votes through `record_vote`. As leader, use `append_command`; report successful peer replication with `record_replication(index, peer_id)`. A log index is committed after a majority acknowledgement. The integration layer must observe higher terms and persist Raft state before using this in a real cluster.
