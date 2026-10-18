// Rotated sprite blits and the sin / cos tables they use (0x00430df0 .. 0x00431480).
// Tables: cos at 0x004ba310, sin at 0x004ba710, 256 entries of 16.16 fixed point.
#pragma once

#include <cstdint>

// 0x00430df0  size 0x55  fill both tables for the 256 angles.
void __cdecl InitSinCosTables();
// 0x00430e50  size 0x1e4  ECX = RLE sprite ({u16 w, u16 h, row offsets, runs}), EDX = clip
// RECT {left, top, right, bottom}; stack (destX, destY, angle): draw it rotated into the back
// buffer (640 pixels per row).
void __fastcall BlitRotatedRLESprite(const uint8_t *sprite, const int *clip, int destX, int destY, int angle);
// 0x00431040  size 0x212  ECX = angle, EDX = RECT: replace it by the bounding box of its
// corners rotated about its centre.
void __fastcall RotateRectBounds(int angle, int *rect);
// 0x00431260  size 0x21f  ECX = sprite, EDX = source RECT; stack (destX, destY, angle, clip):
// rotated blit sampling pixels through the art cache (0x00476160).
void __fastcall BlitRotatedSprite(const uint8_t *sprite, const int *srcRect, int destX, int destY, uint8_t angle, const int *clip);
// 0x00431480  size 0x3c  ECX, EDX and the stack argument are passed on to the war-mode packet
// builder 0x00464be0 together with the player's byte at +0x150; the packet is then sent.
void __fastcall SendWarModePacket(int ecxArg, int edxArg, int stackArg);
