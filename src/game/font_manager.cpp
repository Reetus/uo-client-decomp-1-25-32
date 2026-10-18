// CFontManager.
#include "font_manager.h"

#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"

// 0x004327b0: measure one line (font, text, &w, &h, &base). ECX = font manager.
#define Font_MeasureText(self, a1, a2, a3, a4, a5) (reinterpret_cast<CFontManager *>(self)->MeasureText(a1, a2, a3, a4, a5))

// 0x004328e0  size 0x2c0
int __thiscall CFontManager::MeasureTextWrapped(int font, char *text, int maxWidth, int *outW, int *outH,
                                                int *outBase, int indent)
{
    char line[200];
    int maxW = 0;
    int totalH = 0;
    int firstBase = 0;
    int lastSpace;
    int brk;
    int i, len, width, gw, skipSpace, count, rem, k, j;
    int w, h, base;
    char *buf;
    char c;

    if (!(font <= 0x19 && font >= 0 && fontBlock[font] != 0))
        return 0;
    if (text == nullptr)
        return 0;

    buf = static_cast<char *>(::operator new(strlen(text) + 1));
    strcpy(buf, text);

    do {
        lastSpace = -1;
        width = 0;
        len = static_cast<int>(strlen(buf));
        i = 0;
        brk = -1;
        if (len > 0) {
            for (;;) {
                c = buf[i];
                if (font <= 0x19 && font >= 0 && fontBlock[font] != 0 && c >= 0x20 && static_cast<int>(c) - 0x20 < 0xe0) {
                    // glyph pointer table at block + ch * 4 - 0x7c; the glyph starts with its u16 advance
                    const uint8_t *block = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(static_cast<uint32_t>(fontBlock[font])));
                    const uint8_t *glyph = *reinterpret_cast<const uint8_t *const *>(block + static_cast<int>(c) * 4 - 0x7c);
                    gw = *reinterpret_cast<const uint16_t *>(glyph);
                } else {
                    gw = 0;
                }
                width += gw;
                if (c == ' ' && i > 1)
                    lastSpace = i - 1;
                if (width > maxWidth) {
                    if (lastSpace != -1)
                        brk = lastSpace;
                    else
                        brk = i - 1;
                    break;
                }
                i++;
                if (i >= len)
                    break;
            }
        }

        if (brk != -1) {
            skipSpace = (buf[brk + 1] == ' ');
            count = brk + 1;
            lastSpace = count;                        // copied length
            memcpy(line, buf, static_cast<size_t>(count));
            line[count] = 0;

            Font_MeasureText(this, font, line, &w, &h, &base);
            if (w > maxW)
                maxW = w;
            totalH += h;
            if (firstBase == 0)
                firstBase = base;

            rem = static_cast<int>(strlen(buf)) - skipSpace - count;
            k = 0;
            if (rem > 0) {
                do {
                    buf[k] = buf[count + skipSpace + k];
                    k++;
                } while (k < rem);
            }
            buf[k] = 0;

            if (indent != 0) {
                // writes past the allocation when indent > 0, as the original does  // ?
                buf[strlen(buf) + static_cast<size_t>(indent)] = 0;
                j = static_cast<int>(strlen(buf)) + indent - 1;
                if (j >= indent) {
                    do {
                        buf[j] = buf[j - indent];
                        j--;
                    } while (j >= indent);
                }
                if (indent > 0)
                    memset(buf, ' ', static_cast<size_t>(indent));
            }
        }
    } while (brk != -1);

    Font_MeasureText(this, font, buf, &w, &h, &base);
    if (w > maxW)
        maxW = w;
    if (firstBase == 0)
        firstBase = base;
    *outW = maxW;
    *outH = totalH + h;
    *outBase = firstBase;
    operator delete(buf);
    return 1;
}
HOOK_METHOD(0x004328e0, CFontManager, MeasureTextWrapped);
