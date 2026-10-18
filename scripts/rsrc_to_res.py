"""Convert the .rsrc section of a PE file into a .res file for link.exe.

The host executable must carry the original client's resources (icons and the
accelerator table) because the original code loads them through the
HINSTANCE that GetModuleHandle(NULL) returns, which is the host image.

    python scripts/rsrc_to_res.py orig/CLIENT.EXE build/orig_rsrc.res
"""
import struct
import sys


def pe_sections(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    optsz = struct.unpack_from("<H", data, pe + 20)[0]
    sh = pe + 24 + optsz
    secs = []
    for i in range(nsec):
        name = data[sh + 40 * i : sh + 40 * i + 8].rstrip(b"\0").decode()
        vs, va, rs, ro = struct.unpack_from("<IIII", data, sh + 40 * i + 8)
        secs.append((name, va, vs, ro, rs))
    return secs


def main(src, dst):
    data = open(src, "rb").read()
    rsrc = [s for s in pe_sections(data) if s[0] == ".rsrc"][0]
    _, rva, _, raw, _ = rsrc

    def entries(dir_off):
        nn, ni = struct.unpack_from("<HH", data, raw + dir_off + 12)
        out = []
        for i in range(nn + ni):
            ident, off = struct.unpack_from("<II", data, raw + dir_off + 16 + 8 * i)
            if ident & 0x80000000:
                soff = raw + (ident & 0x7FFFFFFF)
                n = struct.unpack_from("<H", data, soff)[0]
                ident = data[soff + 2 : soff + 2 + 2 * n].decode("utf-16-le")
            out.append((ident, off))
        return out

    def ident_bytes(ident):
        if isinstance(ident, int):
            return struct.pack("<HH", 0xFFFF, ident)
        return ident.encode("utf-16-le") + b"\0\0"

    res = bytearray()
    # Leading empty entry, required by the format.
    res += struct.pack("<II", 0, 0x20) + struct.pack("<HH", 0xFFFF, 0) + struct.pack("<HH", 0xFFFF, 0)
    res += struct.pack("<IHHII", 0, 0, 0, 0, 0)

    count = 0
    for rtype, toff in entries(0):
        for rname, noff in entries(toff & 0x7FFFFFFF):
            for lang, loff in entries(noff & 0x7FFFFFFF):
                drva, dsize = struct.unpack_from("<II", data, raw + loff)
                blob = data[raw + (drva - rva) : raw + (drva - rva) + dsize]
                hdr = ident_bytes(rtype) + ident_bytes(rname)
                if len(hdr) % 4:
                    hdr += b"\0\0"
                hdr += struct.pack("<IHHII", 0, 0x1030, lang, 0, 0)
                hdr_size = 8 + len(hdr)
                res += struct.pack("<II", dsize, hdr_size) + hdr + blob
                while len(res) % 4:
                    res += b"\0"
                count += 1
    open(dst, "wb").write(res)
    print(f"{dst}: {count} resources")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
