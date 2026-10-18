"""Builds client.exe with a current Visual Studio (32-bit x86 MSVC).

Run from the repository root:  python -I scripts/build_msvc.py [--jobs N] [--vs ROOT]

Standard library only. The same sources as build_vc5.py, compiled as C++17 without the VC5 compatibility headers
(src/compat/vc5), then linked into build-msvc/client.exe. orig/CLIENT.EXE (your own copy of the 1.25.32 client)
must be present: the icons and the accelerator table are taken from it, and its SHA-256 is checked first.

Visual Studio: --vs, else env MSVC_VS_ROOT, else the newest installation vswhere reports with the x86 C++ tools.
The environment comes from its VC/Auxiliary/Build/vcvars32.bat.

The result runs on current Windows, not on Windows 9x (that is the VC5 build's job), and its code is further from
the original's than the VC5 build's.
"""
import argparse
import concurrent.futures
import hashlib
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rsrc_to_res
import vc5_orig_align

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(REPO, "build-msvc")
OBJ_DIR = os.path.join(BUILD, "obj")
LOG_DIR = os.path.join(BUILD, "log")

# ---- Compiler flags. ----------------------------------------
# The VC5 build's flags (-Oy- keeps frame pointers, -MT static CRT, -DSTRICT for the handle types), with -EHsc
# for -GX and the language level the sources are written in. _CRT_SECURE_NO_WARNINGS: the ports call strcpy,
# sprintf and fopen as the original does.
CL_FLAGS = ["-nologo", "-c", "-MT", "-EHsc", "-O2", "-Oy-", "-W3", "-std:c++17",
            "-DSTRICT", "-DWIN32_LEAN_AND_MEAN", "-DNOMINMAX", "-D_CRT_SECURE_NO_WARNINGS"]
# ---------------------------------------------------------------------------------------------

# Objects whose `.orig$` sections need their alignment set to 1 after compiling (see vc5_orig_align.py; the
# current compiler aligns them to 4 or 8 like VC5 does).
ALIGN_ORIG = ("globals.cpp",)

# ---- Link. ----------------------------------------
# As build_vc5.py, plus -largeaddressaware:no, which the current linker would otherwise set (the original does
# not). The current linker's defaults for ASLR (-dynamicbase) and DEP (-nxcompat) are kept: nothing in the ports
# depends on load addresses or executes data.
LINK_FLAGS = ["-nologo", "-subsystem:windows", "-incremental:no", "-section:.orig,RW", "-base:0x400000",
              "-largeaddressaware:no"]
LINK_LIBS = ["libcmt.lib", "kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "comctl32.lib",
             "wsock32.lib", "vfw32.lib", "winmm.lib", "ddraw.lib", "dsound.lib"]
RES_FILE = "orig_rsrc.res"
ORIG_SHA256 = "f17d6269e36cd345f177023683aed99c15a851e52ed0fed773400374eda26ec7"
# ---------------------------------------------------------------------------------------------

STANDALONE = ["main.cpp", "seams_off.cpp"]

ERROR_RE = re.compile(r"^(?P<file>.*?)\((?P<line>\d+)\): (?:fatal )?error (?P<code>[A-Z]+\d+): (?P<msg>.*)$")


def find_vs(override):
    root = override or os.environ.get("MSVC_VS_ROOT")
    if not root:
        vswhere = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
                               "Microsoft Visual Studio", "Installer", "vswhere.exe")
        if os.path.exists(vswhere):
            root = subprocess.run([vswhere, "-latest", "-products", "*", "-requires",
                                   "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
                                  stdin=subprocess.DEVNULL, capture_output=True, text=True).stdout.strip()
    vcvars = os.path.join(root or "", "VC", "Auxiliary", "Build", "vcvars32.bat")
    if not root or not os.path.exists(vcvars):
        sys.exit("error: no Visual Studio with the x86 C++ tools found%s.\n"
                 "Install the \"Desktop development with C++\" workload, or set MSVC_VS_ROOT to the installation."
                 % (" at " + root if root else ""))
    return root, vcvars


def tool_env(vcvars):
    """The environment vcvars32.bat sets up, with the paths of cl.exe and link.exe."""
    out = subprocess.run('"%s" >nul && set' % vcvars, shell=True, stdin=subprocess.DEVNULL,
                         capture_output=True, text=True).stdout
    env = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    path = env.get("Path") or env.get("PATH", "")
    cl, link = shutil.which("cl.exe", path=path), shutil.which("link.exe", path=path)
    if not cl or not link:
        sys.exit("error: %s did not put cl.exe and link.exe on PATH" % vcvars)
    return env, cl, link


def game_sources():
    game = os.path.join(REPO, "src", "game")
    return sorted(os.path.join(d, f) for d, _, fs in os.walk(game) for f in fs if f.endswith(".cpp"))


def sources():
    return game_sources() + [os.path.join(REPO, "src", "standalone", f) for f in STANDALONE]


def rel(path):
    return os.path.relpath(path, REPO).replace("\\", "/")


def object_path(src):
    return os.path.join(OBJ_DIR, rel(src)[len("src/"):].replace("/", "__")[:-4] + ".obj")


def compile_one(cl, env, timeout, src):
    """Compiles one source. Returns (repo-relative path, error message or None)."""
    r = rel(src)
    obj = object_path(src)
    log = os.path.join(LOG_DIR, r[len("src/"):].replace("/", "__")[:-4] + ".log")
    cmd = [cl] + CL_FLAGS + ["-Fo" + obj, src]
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


def link(link_exe, env):
    """Links build-msvc/client.exe. Returns 0 on success."""
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
        f.write("\n".join('"%s"' % a for a in LINK_FLAGS + ["-out:" + exe, "-map:" + mp] + objs + [RES_FILE] + LINK_LIBS)
                + "\n")
    proc = subprocess.run([link_exe, "@" + rsp], env=env, cwd=BUILD,
                          stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = proc.stdout.decode("latin-1")
    with open(os.path.join(LOG_DIR, "link_client.log"), "w") as lf:
        lf.write(out)
    if proc.returncode or not os.path.exists(exe):
        print("link: FAILED (exit %d)" % proc.returncode)
        for l in [l for l in out.splitlines() if l.strip()][:60]:
            print("   " + l)
        return 1
    print("link: ok, %s (%d bytes)" % (rel(exe), os.path.getsize(exe)))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--vs", metavar="ROOT", help="Visual Studio installation directory; overrides MSVC_VS_ROOT")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--timeout", type=int, default=120, help="seconds per file (default 120)")
    args = ap.parse_args()

    if not os.path.exists(os.path.join(REPO, "orig", "CLIENT.EXE")):
        sys.exit("error: put your copy of the 1.25.32 CLIENT.EXE at orig/CLIENT.EXE")
    root, vcvars = find_vs(args.vs)
    env, cl, link_exe = tool_env(vcvars)
    os.makedirs(OBJ_DIR, exist_ok=True)
    os.makedirs(LOG_DIR, exist_ok=True)
    srcs = sources()
    print("compiler %s; %d files, %d jobs" % (cl, len(srcs), args.jobs))
    failed = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for r, err in ex.map(lambda s: compile_one(cl, env, args.timeout, s), srcs):
            if err:
                failed.append(r)
                print("FAILED %s\n      %s" % (r, err))
    print("compiled %d of %d files" % (len(srcs) - len(failed), len(srcs)))
    if failed:
        return 1
    return link(link_exe, env)


if __name__ == "__main__":
    sys.exit(main())
