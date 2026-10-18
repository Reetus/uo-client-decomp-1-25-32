"""Builds client.exe with Visual C++ 5.0.

Run from the repository root:  python -I scripts/build_vc5.py [--jobs N] [--toolchain ROOT]

Standard library only. Compiles every .cpp under src/game plus the entry point, in parallel, then links
build-vc5/client.exe. orig/CLIENT.EXE (your own copy of the 1.25.32 client) must be present: the icons and
the accelerator table are taken from it, and its SHA-256 is checked first.

Toolchain: --toolchain, else env VC5_ROOT, else <repo>/../_dev/vc5-c2-7182. It needs VC/BIN, VC/INCLUDE,
VC/LIB and SHAREDIDE/BIN (MSPDB50.DLL, RC.EXE, CVTRES.EXE).
"""
import argparse
import concurrent.futures
import hashlib
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rsrc_to_res
import vc5_orig_align

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(REPO, "build-vc5")
OBJ_DIR = os.path.join(BUILD, "obj")
LOG_DIR = os.path.join(BUILD, "log")
COMPAT_DIR = os.path.join(REPO, "src", "compat", "vc5")

# ---- Compiler flags. ----------------------------------------
# -Oy- keeps frame pointers, as the original does. -MT static multithreaded CRT, -GX exception handling,
# -DSTRICT makes the SDK declare HBITMAP etc. as pointers to HBITMAP__ the way the sources forward-declare them.
CL_FLAGS = ["-nologo", "-c", "-MT", "-GX", "-O2", "-Oy-", "-W3",
            "-DSTRICT", "-DWIN32_LEAN_AND_MEAN", "-DNOMINMAX"]
CL_INCLUDES = [COMPAT_DIR]            # searched before the toolchain's own headers
CL_FORCE_INCLUDE = "vc5_compat.h"
# ---------------------------------------------------------------------------------------------

# Objects whose `.orig$` sections need their alignment set to 1 after compiling (see vc5_orig_align.py).
ALIGN_ORIG = ("globals.cpp",)

# ---- Link. ----------------------------------------
# -section:.orig,RW  the global data (see vc5_orig_align.py). LINK 5.00 has no /LARGEADDRESSAWARE (the flag
# is never set, as in the original). libcmt.lib is the static multithreaded CRT; the other libraries are the
# ones whose symbols the objects reference.
LINK_FLAGS = ["-nologo", "-subsystem:windows", "-incremental:no", "-section:.orig,RW", "-base:0x400000"]
LINK_LIBS = ["libcmt.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "comctl32.lib",
             "wsock32.lib", "vfw32.lib", "winmm.lib", "ddraw.lib", "dsound.lib"]
RES_FILE = "orig_rsrc.res"
ORIG_SHA256 = "f17d6269e36cd345f177023683aed99c15a851e52ed0fed773400374eda26ec7"
# ---------------------------------------------------------------------------------------------

STANDALONE = ["main.cpp", "seams_off.cpp"]

ERROR_RE = re.compile(r"^(?P<file>.*?)\((?P<line>\d+)\) : (?:fatal )?error (?P<code>[A-Z]+\d+): (?P<msg>.*)$")
NUL = bytes(1)


def find_toolchain(override):
    root = override or os.environ.get("VC5_ROOT") or os.path.join(os.path.dirname(REPO), "_dev", "vc5-c2-7182")
    root = os.path.abspath(root)
    missing = [p for p in (os.path.join(root, "VC", "BIN", "CL.EXE"), os.path.join(root, "VC", "INCLUDE"),
                           os.path.join(root, "VC", "LIB"), os.path.join(root, "SHAREDIDE", "BIN"))
               if not os.path.exists(p)]
    if missing:
        sys.exit("error: Visual C++ 5.0 toolchain not found at %s (missing: %s).\n"
                 "Set VC5_ROOT to a directory holding VC/BIN, VC/INCLUDE, VC/LIB and SHAREDIDE/BIN."
                 % (root, ", ".join(os.path.relpath(m, root) for m in missing)))
    return root


def tool_env(root):
    env = dict(os.environ)
    env["PATH"] = os.pathsep.join([os.path.join(root, "VC", "BIN"),
                                   os.path.join(root, "SHAREDIDE", "BIN"), env.get("PATH", "")])
    env["INCLUDE"] = os.path.join(root, "VC", "INCLUDE")
    env["LIB"] = os.path.join(root, "VC", "LIB")
    return env


def game_sources():
    game = os.path.join(REPO, "src", "game")
    return sorted(os.path.join(d, f) for d, _, fs in os.walk(game) for f in fs if f.endswith(".cpp"))


def sources():
    return game_sources() + [os.path.join(REPO, "src", "standalone", f) for f in STANDALONE]


def rel(path):
    return os.path.relpath(path, REPO).replace("\\", "/")


def object_path(src):
    return os.path.join(OBJ_DIR, rel(src)[len("src/"):].replace("/", "__")[:-4] + ".obj")


def compile_one(root, env, timeout, src):
    """Compiles one source. Returns (repo-relative path, error message or None)."""
    r = rel(src)
    obj = object_path(src)
    log = os.path.join(LOG_DIR, r[len("src/"):].replace("/", "__")[:-4] + ".log")
    cmd = [os.path.join(root, "VC", "BIN", "CL.EXE")] + CL_FLAGS + ["-I" + i for i in CL_INCLUDES]
    cmd += ["-FI" + CL_FORCE_INCLUDE, "-Fo" + obj, src]
    if os.path.exists(obj):
        os.remove(obj)
    with open(log, "wb") as lf:
        proc = subprocess.Popen(cmd, env=env, cwd=REPO, stdin=subprocess.DEVNULL, stdout=lf, stderr=subprocess.STDOUT)
        try:
            rc = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], stdin=subprocess.DEVNULL,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            proc.wait()
            return r, "killed after %d s" % timeout
    if rc != 0 or not os.path.exists(obj):
        text = open(log, "rb").read().decode("latin-1")
        errors = [l.strip() for l in text.splitlines() if ERROR_RE.match(l.strip())]
        return r, "\n      ".join(errors[:5]) or "exit code %d; see %s" % (rc, rel(log))
    if os.path.basename(src) in ALIGN_ORIG:
        found, _ = vc5_orig_align.align_orig_sections(obj)
        if not found:
            return r, "no .orig$ sections in the object"
    return r, None


def link(root, env):
    """Links build-vc5/client.exe. Returns 0 on success."""
    orig = os.path.join(REPO, "orig", "CLIENT.EXE")
    data = open(orig, "rb").read()
    if hashlib.sha256(data).hexdigest() != ORIG_SHA256:
        print("error: orig/CLIENT.EXE is not the pristine 1.25.32 image")
        return 1
    rsrc_to_res.main(orig, os.path.join(BUILD, RES_FILE))
    exe = os.path.join(BUILD, "client.exe")
    mp = os.path.join(BUILD, "client.map")
    for p in (exe, mp):
        if os.path.exists(p):
            os.remove(p)
    objs = [object_path(s) for s in sources()]
    rsp = os.path.join(BUILD, "client.rsp")
    with open(rsp, "w") as f:
        f.write("\n".join(LINK_FLAGS + ["-out:" + exe, "-map:" + mp] + objs + [RES_FILE] + LINK_LIBS) + "\n")
    proc = subprocess.run([os.path.join(root, "VC", "BIN", "LINK.EXE"), "@" + rsp], env=env, cwd=BUILD,
                          stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout.replace(NUL, b"").decode("latin-1")
    with open(os.path.join(LOG_DIR, "link_client.log"), "w") as lf:
        lf.write(out)
    if proc.returncode or not os.path.exists(exe):
        print("link: FAILED (exit %d)" % proc.returncode)
        for l in [l for l in out.splitlines() if l.strip() and not l.startswith("Microsoft") and "All rights" not in l][:60]:
            print("   " + l)
        return 1
    print("link: ok, %s (%d bytes)" % (rel(exe), os.path.getsize(exe)))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--toolchain", metavar="ROOT", help="Visual C++ root (VC/BIN, VC/INCLUDE, VC/LIB, SHAREDIDE/BIN); overrides VC5_ROOT")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--timeout", type=int, default=120, help="seconds per file (default 120)")
    args = ap.parse_args()

    if not os.path.exists(os.path.join(REPO, "orig", "CLIENT.EXE")):
        sys.exit("error: put your copy of the 1.25.32 CLIENT.EXE at orig/CLIENT.EXE")
    root = find_toolchain(args.toolchain)
    env = tool_env(root)
    os.makedirs(OBJ_DIR, exist_ok=True)
    os.makedirs(LOG_DIR, exist_ok=True)
    srcs = sources()
    print("toolchain %s; %d files, %d jobs" % (root, len(srcs), args.jobs))
    failed = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for r, err in ex.map(lambda s: compile_one(root, env, args.timeout, s), srcs):
            if err:
                failed.append(r)
                print("FAILED %s\n      %s" % (r, err))
    print("compiled %d of %d files" % (len(srcs) - len(failed), len(srcs)))
    if failed:
        return 1
    return link(root, env)


if __name__ == "__main__":
    sys.exit(main())
