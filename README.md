# Ultima Online client 1.25.32 decompilation

![ported](https://img.shields.io/badge/ported-1%2C728%20of%201%2C745%20(99.7%25)-brightgreen)
![client](https://img.shields.io/badge/client-1.25.32%20(April%201998)-blue)
![platform](https://img.shields.io/badge/platform-Windows%20x86-lightgrey)
![compiler](https://img.shields.io/badge/compiler-Visual%20C%2B%2B%205.0-lightgrey)

A decompilation of Origin's Ultima Online Windows client, version 1.25.32
(linked 1998-04-13), into C++ that builds a working `client.exe` with Visual C++ 5.0, the compiler the original was built with.

The target is the pristine, unpatched image:

```text
f17d6269e36cd345f177023683aed99c15a851e52ed0fed773400374eda26ec7  CLIENT.EXE
```

## Status

- **1,728 of 1,745 game procedures are ported** (99.7% by bytes). The 17
  that are not are C runtime and exception-handling procedures, which the
  compiler's own runtime replaces.
- `client.exe` is standalone: the ported code on the C runtime of the
  compiler that builds it. Nothing of the original executable is linked in
  except its icons and accelerator table.
- The `client.exe` built here was run on Windows 98 SE (86Box: Pentium MMX
  233, S3 ViRGE, Sound Blaster 16) and reached the game world on a local
  shard.
- The code is behaviorally faithful, not byte-matching. Matching the
  original's code is the next goal.

## Building

You need Windows, Python 3, a Visual C++ 5.0 toolchain and your own copy of
the 1.25.32 `CLIENT.EXE`. None of these is in this repository.

1. Put `CLIENT.EXE` at `orig/CLIENT.EXE` (git ignores it). The build checks
   it against the SHA-256 above and takes the icons and accelerator table
   from it.
2. Point the environment variable `VC5_ROOT` at the toolchain: a directory
   holding `VC/BIN`, `VC/INCLUDE`, `VC/LIB` and `SHAREDIDE/BIN`.
3. From the repository root:

```powershell
python -I scripts/build_vc5.py
```

That compiles the 264 source files in parallel (a few seconds) and links
`build-vc5/client.exe`. To run it, copy `client.exe` into a 1.25.x client
directory (the `*.MUL` data files and the rest of an install).
[docs/building.md](docs/building.md) has the details, including
`scripts/build_msvc.py`, which builds with a current Visual Studio instead.

## Repository layout

| Path | Contents |
| --- | --- |
| `src/game/` | The ported game code. One `.cpp` per original source file or class; headers hold the class layouts with `static_assert`s on the offsets |
| `src/standalone/` | The entry point |
| `src/runtime/` | Macros the ports share |
| `src/compat/vc5/` | Headers Visual C++ 5.0 lacks |
| `scripts/` | The builds (`build_vc5.py`, `build_msvc.py`) and their two helpers |
| `docs/` | `building.md` |

## Legal

Ultima Online is a trademark of Electronic Arts. This project is not
affiliated with or endorsed by Electronic Arts. This repository contains
no original executables, game data or art: building or running the client
requires your own copy of the 1.25.32 client and its data files.
