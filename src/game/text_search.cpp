#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"
#include "globals.h"

// Search tables the original keeps in its image.

// 0x004067b0  size 0x23a
// Case-insensitive Boyer-Moore substring search. Needle in ECX, haystack in EDX;
// returns the index of the first match or -1. Case folding is the original CRT's
// _tolower (0x0048b270).
// The local suffix array was int[98] in the original and overflowed for long
// needles; it is larger here so an overlong needle does not corrupt this stack
// (the shared global tables at 0x004b6b50 / 0x004b6ce0 are written exactly as before).
int __fastcall StrFindNoCase(const char *needle, const char *haystack)
{
    int suffix[256];
    int *tbl1 = g_bmBadChar;
    int *tbl2 = g_bmGoodSuffix;
    int m = static_cast<int>(strlen(needle));
    int start = m - 1;
    int pos = m - 1;
    int n = static_cast<int>(strlen(haystack));
    int j;
    for (j = 0; j < 0x100; j++)
        tbl1[j] = static_cast<int>(strlen(needle));

    int k = static_cast<int>(strlen(needle)) - 1;
    for (const char *p = needle; *p != '\0'; p++) {
        tbl1[tolower(static_cast<int>(*p))] = k;
        k = k - 1;
    }

    int m2 = static_cast<int>(strlen(needle));
    if (m2 > 1) {
        int *out = suffix;
        int count = m2 - 1;
        const char *aStart = needle + (m2 - 1);
        const char *bStart = aStart - 1;
        do {
            int matched = 0;
            if (m2 > 0) {
                const char *a = aStart;
                const char *b = bStart;
                while (tolower(static_cast<int>(*a)) == tolower(static_cast<int>(*b))) {
                    matched++;
                    b--;
                    a--;
                    if (!(matched < m2))
                        break;
                }
            }
            *out = matched;
            out++;
            bStart--;
            count--;
        } while (count != 0);
    }

    tbl2[0] = 1;
    if (m2 > 1) {
        for (j = 1; j < m2; j++)
            tbl2[j] = m2;
    }
    for (k = m2 - 1; k > 0; k--)
        tbl2[suffix[k - 1]] = k;

    int sticky = 0;
    if (m2 > 1) {
        int want = m2 - 2;
        for (j = 1; j < m2; j++) {
            if (suffix[j - 1] == want)
                sticky = j;
            if (sticky != 0)
                tbl2[j] = sticky;
            want = want - 1;
        }
    }

    if (start >= n)
        return -1;
    for (;;) {
        int matched = 0;
        if (m > 0) {
            const char *b = needle + start;
            const char *h = haystack + pos;
            while (tolower(static_cast<int>(*h)) == tolower(static_cast<int>(*b))) {
                matched++;
                h--;
                b--;
                if (!(matched < m))
                    break;
            }
        }
        if (matched == m)
            return pos - start;
        int bad = tbl1[tolower(static_cast<int>(haystack[pos - matched]))] - matched + pos;
        int good = tbl2[matched] + pos;
        pos = (bad > good) ? bad : good;
        if (!(pos < n))
            return -1;
    }
}
HOOK_FUNC(0x004067b0, StrFindNoCase);
