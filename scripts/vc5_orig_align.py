"""Set the alignment of the `.orig$<address>` sections of a Visual C++ 5.0 object file to 1.

    python -I scripts/vc5_orig_align.py build-vc5/obj/game__globals.obj

Visual C++ 5.0 gives every data section it writes (`#pragma data_seg`) an alignment of 4, or of 8
when the definition is 8 bytes or more, and has no way to ask for less. The linker would then pad
between the definitions of game/globals.cpp, which must follow each other at the original's
distances. This rewrites the alignment field of the section headers named `.orig$...` to 1 byte, which
is what `EDITBIN /SECTION:name,a1` does for one section (the command line of 2,000 sections is too
long for one call). build_vc5.py runs it after compiling globals.cpp; the link step must have run it
on the object it links.

Prints the number of sections found and changed (running it twice is harmless). Exits non-zero when the object has no such section (it was not
compiled from game/globals.cpp) or does not look like a 32-bit x86 COFF object.
"""
import struct
import sys

ALIGN_MASK = 0x00F00000
ALIGN_1 = 0x00100000


def align_orig_sections(path):
    data = bytearray(open(path, "rb").read())
    machine, nsec, _, symptr, nsyms = struct.unpack_from("<HHIII", data, 0)
    if machine != 0x14C:
        raise SystemExit(f"{path}: not an x86 COFF object (machine {machine:#06x})")
    strtab = symptr + 18 * nsyms
    changed = found = 0
    for i in range(nsec):
        at = 20 + 40 * i                       # no optional header in an object
        raw = bytes(data[at:at + 8])
        if raw.startswith(b"/"):
            off = int(raw[1:].rstrip(b"\0"))
            end = data.index(b"\0", strtab + off)
            name = bytes(data[strtab + off:end])
        else:
            name = raw.rstrip(b"\0")
        if not name.startswith(b".orig$"):
            continue
        found += 1
        flags = struct.unpack_from("<I", data, at + 36)[0]
        new = (flags & ~ALIGN_MASK) | ALIGN_1
        if new != flags:
            struct.pack_into("<I", data, at + 36, new)
            changed += 1
    open(path, "wb").write(data)
    return found, changed


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    found, n = align_orig_sections(sys.argv[1])
    print(f"{sys.argv[1]}: {found} .orig$ sections, {n} changed to 1-byte alignment")
    if found == 0:
        sys.exit(1)
