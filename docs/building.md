# Building client.exe

## What you need

- Windows. The Visual C++ 5.0 tools run natively on current Windows.
- Python 3 on `PATH`. The build script uses only the standard library.
- A Visual C++ 5.0 toolchain, which is not in this repository and is not
  downloaded: your own installation, or a copy of the directories the build
  uses: `VC/BIN` (`CL.EXE`, `LINK.EXE`), `VC/INCLUDE`, `VC/LIB` and
  `SHAREDIDE/BIN` (`MSPDB50.DLL`, `RC.EXE`, `CVTRES.EXE`). If `LINK.EXE`
  exits with 0xC0000135, copy `MSVCP50.DLL` and `MSVCIRT.DLL` from the
  install media's redistributables next to it.
- Your own copy of the 1.25.32 `CLIENT.EXE`, at `orig/CLIENT.EXE`. Its
  SHA-256 must be
  `f17d6269e36cd345f177023683aed99c15a851e52ed0fed773400374eda26ec7`.

## Build

Point `VC5_ROOT` at the toolchain (or pass `--toolchain ROOT`; without
either, the script looks in `_dev/vc5-c2-7182` next to the repository
directory), then from the repository root:

```powershell
$env:VC5_ROOT = "C:\path\to\vc5"
python -I scripts/build_vc5.py
```

The script:

1. compiles every `.cpp` under `src/game/` plus `src/standalone/main.cpp`
   and `seams_off.cpp`, in parallel (`--jobs N`, default one per CPU),
   into `build-vc5/obj/`, with a log per file in `build-vc5/log/`;
2. sets the alignment of the `.orig$` sections of `globals.obj` to 1
   (`scripts/vc5_orig_align.py`), so the global data keeps the original's
   layout;
3. checks the SHA-256 of `orig/CLIENT.EXE` and converts its resources (the
   icons and the accelerator table) to `build-vc5/orig_rsrc.res`
   (`scripts/rsrc_to_res.py`);
4. links `build-vc5/client.exe` and its map `build-vc5/client.map`.

The exit code is non-zero if a file fails to compile or the link fails.

## Compiler and linker settings

Both are at the top of `scripts/build_vc5.py`.

- Compiler: `-MT -GX -O2 -Oy- -W3 -DSTRICT -DWIN32_LEAN_AND_MEAN -DNOMINMAX`,
  with `src/compat/vc5` searched first and `vc5_compat.h` force-included.
  `-Oy-` keeps frame pointers, as the original does.
- Linker: `-subsystem:windows -incremental:no -section:.orig,RW
  -base:0x400000`, the static multithreaded runtime `libcmt.lib`, and
  kernel32, user32, gdi32, advapi32, comctl32, wsock32, vfw32, winmm, ddraw
  and dsound. Like the original, the image is not large-address-aware.

The tested toolchain is Visual C++ 5.0 as first released (`cl` 11.00.7022,
`link` 5.00.7022). The original was linked by linker 5.02 (Visual Studio 97
Service Pack 1 or 2, per Microsoft KB Q170367).

## Building with a current Visual Studio

`scripts/build_msvc.py` builds the same sources with a current 32-bit x86
MSVC (tested with Visual Studio 2026, `cl` 19.51) into `build-msvc/`:

```powershell
python -I scripts/build_msvc.py
```

It finds Visual Studio with `vswhere` (or takes `--vs ROOT` or the
environment variable `MSVC_VS_ROOT`) and needs the "Desktop development with
C++" workload. It takes `orig/CLIENT.EXE` like the VC5 build does.

- Compiler: the VC5 flags with `-EHsc` for `-GX`, plus `-std:c++17` and
  `-D_CRT_SECURE_NO_WARNINGS`. It does not use `src/compat/vc5`.
- Linker: the VC5 flags plus `-largeaddressaware:no`. ASLR and DEP stay on
  (the linker's defaults). The `.orig$` alignment step is the same, and the
  global data keeps the original's layout.

The result needs a current Windows (not Windows 9x), and its code is further
from the original's than the VC5 build's. The VC5 build remains the
reference.

## Run

`client.exe` has to sit in a directory with the client data files (`*.MUL`,
`LOGIN.CFG` and the rest of a 1.25.x install). Copy it there and start it.
`rundir/` is ignored by git if you want to keep such a copy inside the
repository. The client is single-instance: one run at a time.

## What has been verified

- The build compiles all 264 files without errors and links on Windows 11.
- The resulting `client.exe` was run on Windows 98 SE (86Box: Pentium MMX
  233, S3 ViRGE, Sound Blaster 16) and reached the game world on a local
  shard. Sound and music there were not checked.
- It imports the same 200 functions as the original (the original also
  carries an empty `comdlg32` descriptor), its PE header has the original's
  image base, subsystem and OS versions, and its global data has the
  original's layout and initial bytes.
- The code is behaviorally faithful, not byte-matching.
