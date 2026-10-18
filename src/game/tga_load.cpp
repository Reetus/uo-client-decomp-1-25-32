// LoadTGA16: reads a 24-bit TGA (uncompressed or RLE) into a 16-bit gump bitmap. (Porting pass 2, batch 15.)
#include <cstdint>

#include "../runtime/ported.h"
#include "globals.h"
#include "crt.h"
#include "palette_manager_ports.h"
#include "window_helpers_31_ports.h"

// 0x00436870 on the colour converter object 0x0068dd98: (r, g, b) -> 16-bit colour.
#define PackRGB16(o, a1, a2, a3) PORT_THISCALL3(&PackRGB16, int, (o), int, int, int)(a1, a2, a3)
// 0x00482c70: ECX = text, EDX = caption, stack = exit flag, RET 4: message box.
#define ErrorBox(a1, a2, a3) (ShowMessageBoxAndExit(a1, a2, a3))

// 0x0043ebe0  size 0x31d
// ECX = file name, EDX = destination bitmap (16-bit width, height, then pixels); plain RET. The file's size must
// equal the bitmap's. Returns 1 on success, 0 when the file is missing, the size differs or the file is rejected
// (the last case shows an "Invalid TGA file." box first). Rows are stored bottom-up in the file. Pixels of value 0
// are turned into the colour of RGB (8, 8, 8) afterwards. The row buffer holds 1024 pixels; the original does not
// check the width against it.
int __fastcall LoadTGA16(const char *fileName, uint16_t *bitmap)
{
    uint8_t buf[0xc00];
    uint8_t idLength;
    uint8_t colorMapType;
    uint8_t imageType;
    uint8_t bpp;
    uint16_t width;
    uint16_t height;

    FILE *f = fopen(fileName, "rb");
    if (f == nullptr)
        return 0;

    fread(&idLength, 1, 1, f);
    fread(&colorMapType, 1, 1, f);
    fread(&imageType, 1, 1, f);
    fseek(f, 0xc, 0);
    fread(&width, 2, 1, f);
    fread(&height, 2, 1, f);

    if (width != bitmap[0] || height != bitmap[1]) {
        fclose(f);
        return 0;
    }

    uint16_t *pixels = bitmap + 2;
    fread(&bpp, 1, 1, f);
    fseek(f, static_cast<int8_t>(idLength) + 0x12, 0);

    if (colorMapType != 0 || (imageType != 2 && imageType != 10) || bpp != 0x18) {
        ErrorBox("Invalid TGA file.", "Error", 0);
        fclose(f);
        return 0;
    }

    int row = static_cast<int>(height) - 1;
    if (row >= 0) {
        do {
            if (imageType == 2) {
                fread(buf, width, 3, f);
            } else {
                int idx = 0;
                if (width != 0) {
                    do {
                        uint8_t hdr;
                        fread(&hdr, 1, 1, f);
                        int count = (hdr & 0x7f) + 1;
                        if (hdr & 0x80) {
                            uint8_t *p = buf + idx * 3;
                            fread(p, 1, 3, f);
                            for (int k = 1; k < count; k++) {
                                p[k * 3] = p[0];
                                p[k * 3 + 1] = p[1];
                                p[k * 3 + 2] = p[2];
                            }
                        } else {
                            fread(buf + idx * 3, static_cast<size_t>(count), 3, f);
                        }
                        idx += count;
                    } while (idx < static_cast<int>(width));
                }
            }

            const uint8_t *src = buf;
            for (int i = 0; i < static_cast<int>(width); i++) {
                int b = src[0];
                int g = src[1];
                int r = src[2];
                int pixel;
                if (b + g + r == 0) {
                    pixel = 0;
                } else {
                    b >>= 3;
                    g >>= 3;
                    r >>= 3;
                    if (b + g + r == 0)
                        pixel = 1;
                    else
                        pixel = (g << (g_greenShift16 & 0x1f)) | (r << (g_redShift16 & 0x1f)) | b;
                }
                pixels[row * static_cast<int>(width) + i] = static_cast<uint16_t>(pixel);
                src += 3;
            }
            row--;
        } while (row >= 0);
    }

    fclose(f);
    uint16_t black = static_cast<uint16_t>(PackRGB16(GLOBAL_PTR(void, g_clientState_storage), 8, 8, 8));
    int total = static_cast<int>(width) * static_cast<int>(height);
    uint16_t *p = pixels;
    while (total > 0) {
        if (*p == 0)
            *p = black;
        p++;
        total--;
    }
    return 1;
}
HOOK_FUNC(0x0043ebe0, LoadTGA16);
