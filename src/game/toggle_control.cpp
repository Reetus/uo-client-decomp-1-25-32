#include "toggle_control.h"

#include <cstdlib>

#include "../runtime/ported.h"
#include "batch01_helpers.h"

// 0x004163f0  size 0x192  CToggleControl::Redraw()   (name guessed)
// Clears frame0 and draws the art that matches the checked / disabled / hover state.
void __thiscall CToggleControl::Redraw()
{
    int w;
    int h;
    uint16_t *bmp;

    gfx::ArtSize(unk_50, &w, &h);

    // round an odd positive width up
    if (w % 2 == 1) {
        w = w + 1;
    }

    bmp = gfx::bitmapOf(this);
    gfx::FillRect(0, 0, w - 1, h - 1, 0, bmp);

    if (artHover != 0 && hovering != 0) {
        if (checked != 0) {
            bmp = gfx::bitmapOf(this);
            gfx::DrawArt(gfx::artPlain(), artHoverChecked, 0, 0, bmp + 2, bmp[0], bmp[1]);
            return;
        }
        bmp = gfx::bitmapOf(this);
        gfx::DrawArt(gfx::artPlain(), artHover, 0, 0, bmp + 2, bmp[0], bmp[1]);
        return;
    }

    if (disabled != 0) {
        if (checked != 0) {
            bmp = gfx::bitmapOf(this);
            gfx::DrawArt(gfx::artPlain(), artDisabledChecked, 0, 0, bmp + 2, bmp[0], bmp[1]);
            return;
        }
        bmp = gfx::bitmapOf(this);
        gfx::DrawArt(gfx::artPlain(), artDisabled, 0, 0, bmp + 2, bmp[0], bmp[1]);
        return;
    }

    if (checked != 0) {
        bmp = gfx::bitmapOf(this);
        gfx::DrawArt(gfx::artPlain(), artChecked, 0, 0, bmp + 2, bmp[0], bmp[1]);
        return;
    }
    bmp = gfx::bitmapOf(this);
    gfx::DrawArt(gfx::artPlain(), unk_50, 0, 0, bmp + 2, bmp[0], bmp[1]);
}
HOOK_METHOD(0x004163f0, CToggleControl, Redraw);
