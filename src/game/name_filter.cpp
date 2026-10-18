#include "name_filter.h"

#include <cstring>

#include "../runtime/ported.h"
#include "text_search_ports.h"
#include "globals.h"

// 0x004067b0: case-insensitive Boyer-Moore substring search; ECX = needle, EDX = haystack;
// returns the match index or -1.
static int FindSubstring(const char *needle, const char *haystack)
{
    return StrFindNoCase(needle, haystack);
}

static bool IsPunct(char c)
{
    return c == '-' || c == '.' || c == ' ' || c == '\'';
}

// One whole-word table: a NULL-terminated array of words. A word is a hit when it occurs delimited by
// the start / a punctuation character before it and the end / a punctuation character after it.
static bool WholeWordHit(char **table, char *name)
{
    if (*table != nullptr) {
        char **p = table;
        char *word = *p;
        do {
            int pos = FindSubstring(word, name);
            if (pos != -1) {
                int ok = 0;
                if (pos == 0) {
                    ok = 1;
                } else {
                    char pc = name[pos - 1];
                    if (IsPunct(pc))
                        ok = 1;
                }
                if (ok) {
                    if (strlen(*p) + pos == strlen(name))
                        return true;
                    char nc = name[strlen(*p) + pos];
                    if (IsPunct(nc))
                        return true;
                }
            }
            p++;
            word = *p;
        } while (word != nullptr);
    }
    return false;
}

// 0x0044adf0  size 0x334
// Name validity / reserved-word filter (1 = rejected, 0 = accepted). The first character must be a letter;
// only letters and single '-', '.', ' ' or '\'' are allowed (no two in a row); then the five reserved
// prefixes ("Lord", "Lady", "GM", "Counselor", "Seer" -- a match at index 0 rejects) and the word tables
// 0x4a93a8, 0x4ad238 (whole word), 0x4ad1e0 (any substring) and 0x4a9390 (whole word), in that order.
int __fastcall NameIsRejected(char *name)
{
    char c = name[0];
    if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z'))
        return 1;
    int len = static_cast<int>(strlen(name));
    int i = 0;
    if (len > 0) {
        do {
            c = name[i];
            int alpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
            if (!alpha && !IsPunct(c))
                return 1;
            if (IsPunct(c)) {
                if (i < static_cast<int>(strlen(name)) - 1) {   // no two punctuation chars in a row
                    c = name[i + 1];
                    if (IsPunct(c))
                        return 1;
                }
            }
            i++;
        } while (i < static_cast<int>(strlen(name)));
    }

    if (FindSubstring(g_004a9e3c, name) == 0)   // "Lord"
        return 1;
    if (FindSubstring(g_004a9e34, name) == 0)   // "Lady"
        return 1;
    if (FindSubstring(g_004a9e30, name) == 0)   // "GM"
        return 1;
    if (FindSubstring(g_004a9e24, name) == 0)   // "Counselor"
        return 1;
    if (FindSubstring(g_004a9e1c, name) == 0)   // "Seer"
        return 1;

    if (WholeWordHit(g_004a93a8, name))
        return 1;
    if (WholeWordHit(g_004ad238, name))
        return 1;
    // substring table: any occurrence rejects
    char **q = GLOBAL_PTR(char *, g_004ad1e0);
    if (*q != nullptr) {
        char *word = *q;
        do {
            if (FindSubstring(word, name) != -1)
                return 1;
            q++;
            word = *q;
        } while (word != nullptr);
    }
    if (WholeWordHit(g_004a9390, name))
        return 1;
    return 0;
}
HOOK_FUNC(0x0044adf0, NameIsRejected);
