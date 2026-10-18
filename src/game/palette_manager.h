// CPaletteManager: the 8-bit palette (palette.mul) and the 15-bit to gray lookup table.
// vtable 0x0049ebc4 (the compiler's). Layout partial: from the constructor 0x004368f0 and destructor 0x00436b20.
#pragma once

#include <cstddef>
#include <cstdint>

class CPaletteManager {
public:
    // 0x004368f0: the compiler's vptr (table 0x0049ebc4); read palette.mul, probe the display, build the gray table,
    // load animinfo.mul.
    CPaletteManager();
    // Deletes the GDI palette. Defined inline in palette_manager.cpp so the scalar deleting destructor (0x00436ae0)
    // expands it as the original's does.
    virtual ~CPaletteManager();

    uint16_t unk_04;                // +0x04  cleared by the constructor
    uint16_t grayTable[0x7fff];     // +0x06  gray of 15-bit colour i + 1, in the display's pixel format
    uint8_t pad_10004[4];
    int32_t unk_10008;              // +0x10008  cleared by the constructor
    uint8_t pad_1000c[8];
    uint8_t palette[0x300];         // +0x10014  palette.mul: 256 * (r, g, b)
    int32_t paletteDisplay;         // +0x10314  1 when the display is 8 bits per pixel
    void *logPalette;               // +0x10318  LOGPALETTE (version 0x300, 256 entries), 8-bit displays only
    int32_t hPalette;               // +0x1031c  HPALETTE made from logPalette
    int32_t unk_10320;              // +0x10320  cleared by the constructor
};

// 0x00436b20: the destructor for the atexit thunk (a plain function expanding the inline destructor).
void __fastcall PaletteManager_Dtor_00436b20(void *self, void *);

static_assert(offsetof(CPaletteManager, grayTable) == 0x06, "CPaletteManager::grayTable");
static_assert(offsetof(CPaletteManager, unk_10008) == 0x10008, "CPaletteManager::unk_10008");
static_assert(offsetof(CPaletteManager, palette) == 0x10014, "CPaletteManager::palette");
static_assert(offsetof(CPaletteManager, paletteDisplay) == 0x10314, "CPaletteManager::paletteDisplay");
static_assert(offsetof(CPaletteManager, logPalette) == 0x10318, "CPaletteManager::logPalette");
static_assert(offsetof(CPaletteManager, hPalette) == 0x1031c, "CPaletteManager::hPalette");
static_assert(offsetof(CPaletteManager, unk_10320) == 0x10320, "CPaletteManager::unk_10320");
