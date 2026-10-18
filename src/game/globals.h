// The client's global data: one declaration per symbol the ported code names.
//
// Written by scripts/gen_globals.py from symbols/CLIENT.EXE.globals.tsv; do not edit. Change the table
// and run `python -I scripts/gen_globals.py write`. The definitions, with the original's initial values,
// are in globals.cpp. The address in each comment is where the symbol was in the 1.25.32 image.
//
// A name ending in _storage is the memory of an object whose class the ported code still sees through
// GLOBAL_AS / GLOBAL_PTR (runtime/global_access.h); its size here runs to the next address anything names.
#pragma once

#include <cstdint>
#include <cstdio>

#include "../runtime/global_access.h"

typedef unsigned long DWORD;
typedef void *HANDLE;
struct HBITMAP__;
typedef struct HBITMAP__ *HBITMAP;
struct HICON__;
typedef struct HICON__ *HICON;
typedef HICON HCURSOR;
struct HDC__;
typedef struct HDC__ *HDC;
typedef void *HGDIOBJ;
struct HINSTANCE__;
typedef struct HINSTANCE__ *HINSTANCE;
struct HMENU__;
typedef struct HMENU__ *HMENU;
struct HPALETTE__;
typedef struct HPALETTE__ *HPALETTE;
typedef unsigned int UINT;
class CBarkGump;
class CCharCreationGump;
class CCharStatGump;
class CCityChoiceGump;
class CClientCfgGump;
class CContainerGump;
class CDynamicObj;
class CEffectSequence;
class CGump;
class CInputControlGump;
class CMissileAnim;
class CMobile;
class CMobileAnimQueue;
class CMultiPlacer;
class CObj;
class CObjVt49f730;
class CRadarGump;
class CTerrainTileOps;
struct CTextLine;
struct CacheEntry;
struct CacheNode;
class MappedFile;
struct PathNode;
struct PendingUpdate;
struct VerNode;

extern "C" {

// Constants (.rdata)
extern int32_t BOOK_LEFT_X;                         // 0x0049c6e8
extern int32_t BOOK_LEFT_Y;                         // 0x0049c6ec
extern int32_t BOOK_LEFT_R;                         // 0x0049c6f0
extern int32_t BOOK_LEFT_B;                         // 0x0049c6f4
extern int32_t BOOK_RIGHT_X;                        // 0x0049c6f8
extern int32_t BOOK_RIGHT_Y;                        // 0x0049c6fc
extern int32_t BOOK_RIGHT_R;                        // 0x0049c700
extern int32_t BOOK_RIGHT_B;                        // 0x0049c704
extern int SB_RECT_L[4];                            // 0x0049c708
extern int SB_RECT_R[4];                            // 0x0049c718
extern float g_0049e968;                            // 0x0049e968
extern double g_0049e970;                           // 0x0049e970
extern double g_sinCosStep;                         // 0x0049e988  2 * pi / 256
extern double g_sinCosScale;                        // 0x0049e990  65536.0
extern uint8_t g_fileDesc_storage[0x118];           // 0x0049f2d4
extern int32_t g_stepSounds[0xa];                   // 0x0049f700
extern float kVolumeC1;                             // 0x004a0ca4  0xb9d1b717
extern float kVolumeRange;                          // 0x004a0cac  -2500.0f
extern float kVolumeByteScale;                      // 0x004a0cb0  1/255
extern float kJitterPercent;                        // 0x004a0cc8  0.01f
extern float kJitterBase;                           // 0x004a0ccc  22048.0f
extern float uv_const_luma;                         // 0x004a0cd0  1.164
extern float uv_const_cr_r;                         // 0x004a0cd4  1.596
extern double uv_const_k;                           // 0x004a0cd8
extern float uv_const_cb_b;                         // 0x004a0ce0  2.018
extern float uv_const_cb_g;                         // 0x004a0ce4  0.391
extern float uv_const_cr_g;                         // 0x004a0ce8  -0.813
extern double uv_const_base;                        // 0x004a0cf0  107.0
extern float uv_const_step;                         // 0x004a0cf8  0.0625
extern float uv_const_65536;                        // 0x004a0cfc  65536.0

// Initialised data (.data)
extern char g_fopenReadMode[4];                     // 0x004a60e0  "rb"
extern int32_t g_barkSecondsPerLine;                // 0x004a60e4
extern int32_t g_barkMultiLine;                     // 0x004a60e8
extern char g_titlePart1[4];                        // 0x004a60ec  " "
extern char g_barkPrefixSep[4];                     // 0x004a6120  appended after the prefix
extern uint8_t g_004a619c;                          // 0x004a619c
extern char g_bbPostText[0xc];                      // 0x004a61f8
extern char g_refText[4];                           // 0x004a6214  sample string measured for the line height ("Aq")
extern char g_caretGlyph[4];                        // 0x004a6334  caret glyph appended while measuring
extern char g_fmtNumber[4];                         // 0x004a641c  "%d"-style number format
extern char g_004a6520[8];                          // 0x004a6520
extern char s_frame0Tag[0x10];                      // 0x004a653c  "status frame0"
extern char g_004a654c[0x1c];                       // 0x004a654c  "framedata for CStatusGump"
extern char g_multiStrIndex[8];                     // 0x004a65a4  "multi"
extern char g_quitText[0x1c];                       // 0x004a65ac  "Quit         Ultima Online?"
extern char str_004a65ec[0x14];                     // 0x004a65ec
extern char g_004a6774[0x1c];                       // 0x004a6774
extern char g_004a6790[0xc];                        // 0x004a6790
extern char g_004a679c[0x18];                       // 0x004a679c
extern char g_004a69d8[0x1c];                       // 0x004a69d8
extern const char *g_macroArgNames5[8];             // 0x004a6aa0  8 argument names of command type 5
extern const char *g_004a6ac8[8];                   // 0x004a6ac8
extern const char *g_004a6ae8[2];                   // 0x004a6ae8
extern const char *g_macroArgNames8[0xa];           // 0x004a6af0  names for types 8..11, 0-terminated
extern int g_macroArgFlags8[9];                     // 0x004a6b18  per-name flag used by types 10 / 11
extern char g_profanityText[0x20];                  // 0x004a6c48  "Click to edit Profanity List"
extern char g_newRowText[8];                        // 0x004a6c68  the "(new)" row caption
extern int32_t g_professionTable[5];                // 0x004a6e90  five profession ids picked at random
extern int g_scrollChoiceHoverRow;                  // 0x004a6ea4  row under the mouse, -1 none
extern int32_t g_introFade;                         // 0x004a6ea8  the ChestOpening setting of uo.cfg (0x00424fa0 case 33); here it selects the fade-in
extern char *g_tutorialTable[0xa];                  // 0x004a6eb0
extern char g_mobileText[0xc];                      // 0x004a71c0  "(mobile)"
extern char g_strEmptyServer[0x10];                 // 0x004a74c8  "Unknown Server"
extern char g_004a754c[4];                          // 0x004a754c
extern int32_t g_cursorEnabled2;                    // 0x004a7590  ? non-zero when the software cursor is drawn
extern int32_t MG_PathEnabled;                      // 0x004a7594
extern int32_t wp_warModeEnabled;                   // 0x004a7598
extern int32_t g_004a759c;                          // 0x004a759c
extern int32_t g_004a75a0;                          // 0x004a75a0
extern int32_t g_logWide;                           // 0x004a75a4  -1: follow g_unicodeText
extern int32_t g_004a75a8;                          // 0x004a75a8
extern uint8_t g_cfgTable_storage[0x1d0];           // 0x004a75b0
extern int32_t g_tipEnabled;                        // 0x004a7780
extern uint32_t g_tipNumber;                        // 0x004a7784
extern int32_t g_tipLast;                           // 0x004a7788
extern int32_t g_animThreshold;                     // 0x004a7b1c
extern char g_004a7b64[0x10];                       // 0x004a7b64
extern char g_004a7bd4[0xc];                        // 0x004a7bd4
extern uint32_t g_cpuVendorExpect[3];               // 0x004a7c40
extern char g_vendorExpectByte;                     // 0x004a7c4c
extern uint32_t g_cpuVendorInit[3];                 // 0x004a7c50
extern char g_vendorInitByte;                       // 0x004a7c5c
extern int Mc_layerOrder[0xc8];                     // 0x004a7c98  25 layers per facing (facing & 7)
extern int32_t g_foliageRanges[0x13];               // 0x004a7fb8  {lo, hi} graphic ranges, ended by lo == 0
extern int32_t g_004a8004;                          // 0x004a8004
extern int g_dirAnimBase[8];                        // 0x004a8008
extern int g_dirMirror[8];                          // 0x004a8028  indexed by facing & 0x7f
extern int g_tableMountLo;                          // 0x004a8048
extern int g_tableMountHi;                          // 0x004a804c
extern int g_tableMountList[0x14];                  // 0x004a8050
extern int g_tableWalkLo;                           // 0x004a80a0
extern int g_tableWalkHi;                           // 0x004a80a4
extern int g_tableWalkList[0xa];                    // 0x004a80a8
extern int32_t g_fontHuedFlags[9];                  // 0x004a8188  9 flags, one per font 0..8 (-> CFontBlock::hued)
extern char g_004a81bc[8];                          // 0x004a81bc
extern uint8_t g_caretString_storage[4];            // 0x004a82d8
extern uint32_t g_waitFont;                         // 0x004a8360
extern char g_004a8374[0x1c];                       // 0x004a8374
extern char g_004a84c8[0x10];                       // 0x004a84c8  "status frame1"
extern char g_004a84d8[0x14];                       // 0x004a84d8  "status framedata"
extern char g_004a84ec[0x14];                       // 0x004a84ec  "status frame1 pBMP"
extern char s_frame0Free[0x14];                     // 0x004a8500  "status frame0 pBMP"
extern char g_004a87d4[0xdc];                       // 0x004a87d4
extern char g_004a88fc[0xf8];                       // 0x004a88fc
extern char g_004a89f8[0x68];                       // 0x004a89f8
extern char g_004a8aa0[4];                          // 0x004a8aa0
extern int g_dotCommandTable[0x12];                 // 0x004a8c20  {id, name char *} pairs, ends with id -1
extern char str_004a8d34[8];                        // 0x004a8d34
extern int32_t g_NeighborDX[4];                     // 0x004a8e08  x offsets of the four neighbours
extern int32_t g_NeighborDY[4];                     // 0x004a8e18  y offsets of the four neighbours
extern int32_t g_dirMap[8];                         // 0x004a8e28  facing -> direction used for the slope tables
extern int32_t g_dirDx[0xa];                        // 0x004a8e48  7 directions, dword each
extern int32_t g_dirDy[0xa];                        // 0x004a8e70
extern int wq_pathThrottle;                         // 0x004a8e98
extern const char *g_pathSeparator;                 // 0x004a8f50  "\\"
extern int32_t g_soundEnabledSetting;               // 0x004a8f54
extern uint8_t g_soundVolumeSetting;                // 0x004a8f58
extern int32_t mo_defaultEnabled;                   // 0x004a8f5c
extern uint8_t g_musicDefaultVolume;                // 0x004a8f60
extern int32_t g_loginServerIndex;                  // 0x004a8f64
extern int32_t g_backupServerIndex;                 // 0x004a8f68
extern int32_t g_lastSpell;                         // 0x004a8f6c  spell last cast / selected (0-based, -1 = none)
extern int g_macroLastItem;                         // 0x004a8f70
extern const char *g_spellNames[0x82];              // 0x004a8f78  8-byte entries {name, ?}, empty name ends
extern char *g_spellDescs[0x42];                    // 0x004a9180
extern char *g_spellReagents[0x42];                 // 0x004a9288
extern char *g_004a9390[6];                         // 0x004a9390
extern char *g_004a93a8[0x21];                      // 0x004a93a8
extern int32_t g_pendingWarMode;                    // 0x004a942c
extern char g_004a9e1c[8];                          // 0x004a9e1c
extern char g_004a9e24[0xc];                        // 0x004a9e24
extern char g_004a9e30[4];                          // 0x004a9e30
extern char g_004a9e34[8];                          // 0x004a9e34
extern char g_004a9e3c[8];                          // 0x004a9e3c
extern char g_004a9e50[0x30];                       // 0x004a9e50
extern char g_004a9e80[0xec];                       // 0x004a9e80
extern char g_004a9f8c[0x9c];                       // 0x004a9f8c
extern char s_errorCaption[8];                      // 0x004aa028
extern char g_004aa030[8];                          // 0x004aa030
extern char g_004aa038[4];                          // 0x004aa038
extern char g_004aa03c[0xc];                        // 0x004aa03c
extern char g_004aa048[0x28];                       // 0x004aa048
extern char g_004aa090[8];                          // 0x004aa090
extern char g_004aa098[4];                          // 0x004aa098
extern char *g_soundNameTable[0x24c];               // 0x004aa128
extern int g_lightDirX;                             // 0x004ad0c8
extern int g_lightDirY;                             // 0x004ad0cc
extern int g_lightDirZ;                             // 0x004ad0d0
extern int32_t dr_screenRect[3];                    // 0x004ad108  {0, 0, 0x280, 0x1e0}
extern int32_t g_texU[8];                           // 0x004ad114  per-vertex u (texture x), indexed by texIdx + sizeShift * 4
extern int32_t g_texV[9];                           // 0x004ad134  per-vertex v (texture y)
extern char str_004ad174[4];                        // 0x004ad174
extern char str_004ad178[4];                        // 0x004ad178
extern char str_004ad17c[8];                        // 0x004ad17c
extern uint8_t g_004ad1b0_storage[0x14];            // 0x004ad1b0
extern uint8_t g_004ad1c4_storage[0x10];            // 0x004ad1c4
extern char g_scrollText[8];                        // 0x004ad1d4  "scroll"
extern const char *g_004ad1e0[0x16];                // 0x004ad1e0
extern char *g_004ad238[0x18];                      // 0x004ad238
extern char g_profanityReplace[8];                  // 0x004ad298  "!@#$%&*"
extern const char *g_pickupMessages[5];             // 0x004ad3f8  table of 5 messages indexed by the reason byte
extern char g_fmtLocation[8];                       // 0x004ad4c8  "<name>, <area>"-style location format
extern char g_titlePartC[4];                        // 0x004ad4ec
extern char g_titlePartA[4];                        // 0x004ad4f0
extern char g_strCannotConnectMsg[0x78];            // 0x004ad570
extern char g_strServerListTitle[0x40];             // 0x004ad8b4  copied into the server list gump (+0x226c)
extern char g_strNoServersMsg[0x54];                // 0x004ad8f4  "The Ultima Online servers are currently down, ..."
extern char g_004ad948[0x60];                       // 0x004ad948
extern char g_004ad9a8[0x58];                       // 0x004ad9a8
extern char g_004ada00[0x7c];                       // 0x004ada00
extern char g_004ada7c[0x28];                       // 0x004ada7c
extern char g_004adaa4[0x90];                       // 0x004adaa4
extern char g_strConnectingTitle[0x10];             // 0x004adb34
extern char g_strServerFullMsg[0x74];               // 0x004adb44
extern char g_004adbb8[0x24];                       // 0x004adbb8
extern char g_004adbdc[0x48];                       // 0x004adbdc
extern char g_004adc24[0x68];                       // 0x004adc24
extern char g_004adc8c[0x2c];                       // 0x004adc8c
extern char g_004adcb8[0x20];                       // 0x004adcb8
extern char g_004adcd8[0x24];                       // 0x004adcd8
extern uint8_t g_004adcfc_storage[0x1c];            // 0x004adcfc
extern uint8_t g_004add18_storage[8];               // 0x004add18
extern uint16_t g_pingPort;                         // 0x004add20
extern char str_004add24[0x14];                     // 0x004add24
extern char str_004add38[0x20];                     // 0x004add38
extern char str_004add58[0xc];                      // 0x004add58
extern char str_004add64[0x20];                     // 0x004add64
extern uint8_t g_packetTable[0x59c];                // 0x004add84
extern uint8_t g_equipLayerOrder[0x1c];             // 0x004ae320  layer order table used when listing equipment
extern char g_004ae33c[0x14];                       // 0x004ae33c  "paperdoll frame1"
extern char s_frameTag[0x10];                       // 0x004ae350  "paperdoll frame"
extern char g_004ae360[0x20];                       // 0x004ae360  "framedata for CPaperdollGump"
extern char g_paperdollName[0x10];                  // 0x004ae380  "paperdoll gump"
extern char g_004ae390[0x14];                       // 0x004ae390  "paperdoll framedata"
extern char g_004ae3a4[0x18];                       // 0x004ae3a4  "paperdoll frame1 pBMP"
extern char s_frameFree[0x18];                      // 0x004ae3bc  "paperdoll frame pBMP"
extern char s_nameBmpFree[8];                       // 0x004ae3d4  "nameBMP"
extern char s_guideName[0xc];                       // 0x004ae3dc  "UO Guide"
extern char g_004ae3e8[0x14];                       // 0x004ae3e8
extern char g_004ae3fc[0x20];                       // 0x004ae3fc
extern char g_statusName[0xc];                      // 0x004ae41c  "status gump"
extern char str_004ae528[8];                        // 0x004ae528
extern int32_t Mc_facingBits[8];                    // 0x004ae740
extern uint8_t g_animOverlayTable_storage[0x738];   // 0x004ae760
extern uint32_t g_huffCodes[0x202];                 // 0x004aee98  0x101 pairs {bit length, code}
extern uint32_t g_lastPacketId;                     // 0x004af6a0
extern char g_004af6a4[0x18];                       // 0x004af6a4
extern char g_004af6cc[0x18];                       // 0x004af6cc
extern char g_004af6e4[0x24];                       // 0x004af6e4
extern char g_004af708[0x1c];                       // 0x004af708
extern char g_004af724[0x24];                       // 0x004af724
extern char g_004af748[0x2c];                       // 0x004af748
extern char g_004af774[0x14];                       // 0x004af774
extern uint32_t g_004af788[0xc];                    // 0x004af788
extern int32_t g_verdataEnabled;                    // 0x004af800
extern int g_macroCmdCount;                         // 0x004af804  number of macro command names
extern const char *g_macroCmdNames[1];              // 0x004af808  indexed by command type (entry 0 unused)
extern const char *g_004af80c[0x16];                // 0x004af80c
extern char g_004af95c[0xc];                        // 0x004af95c
extern const char *chp_versionString;               // 0x004afaac
extern char g_weatherSnowArt[0x18];                 // 0x004afab8
extern char g_weatherRainArt[0x14];                 // 0x004afad0
extern int g_redShift16;                            // 0x004afb48
extern int g_greenShift16;                          // 0x004afb4c
extern int32_t g_blendMaskB;                        // 0x004afb50  blend mask
extern int32_t g_blendMaskA;                        // 0x004afb54  m1
extern int g_shadeAdd;                              // 0x004afb58  value added after the halving
extern int32_t g_useFlipChain;                      // 0x004afb5c
extern const char *g_trackNames[0x32];              // 0x004afc48  PTR_s_oldult01_mid_004afc48, 0x31 entries
extern char g_sequencerType[0xc];                   // 0x004affe4  "sequencer"
extern int g_hideCursorFlag;                        // 0x004b0260  1 while the system cursor is shown (outside the window)
extern HANDLE g_singleInstanceMapping;              // 0x004b0264
extern const char *g_classNamePtr;                  // 0x004b0268  window class name of the main window
extern wchar_t g_classNameWide[0x2c];               // 0x004b0270
extern int32_t g_introMovieFlag;                    // 0x004b02c8
extern int32_t g_ccVal2cc;                          // 0x004b02cc
extern char g_titleSeparator[4];                    // 0x004b02e0  " - "
extern char s_colorModeText[0xd0];                  // 0x004b02e4
extern char s_colorModeCaption[0x14];               // 0x004b03b4
extern char s_patchRunningText[0x2c];               // 0x004b03c8
extern char s_patchMappingName[0xc];                // 0x004b03f4
extern char s_alreadyRunningText[0x2c];             // 0x004b0400
extern char s_singleInstanceName[0xc];              // 0x004b042c  name of the instance mapping
extern int32_t uvp_coef1[4];                        // 0x004b1b10
extern int32_t uvp_coef2[4];                        // 0x004b1b20
extern uint8_t uvp_imaStep[0x1640];                 // 0x004b1b30  rows of 16 dwords, indexed by byte offset
extern int32_t uvp_imaAdjust[8];                    // 0x004b3170
extern int32_t uvp_g3190;                           // 0x004b3190
extern int32_t uvp_g3194;                           // 0x004b3194
extern int32_t uvp_g3198;                           // 0x004b3198
extern int32_t uvp_g319c;                           // 0x004b319c
extern int32_t uv_tab_a[0x40];                      // 0x004b31a0  64 entries
extern int32_t uv_tab_b[0x40];                      // 0x004b32a0  64 entries

// Uninitialised data (.data past its raw size, and the few initialised bytes before it)
extern int32_t g_animTick;                          // 0x004b6b30  animation tick counter, incremented once per update
extern uint32_t g_lastFpsTick;                      // 0x004b6b34
extern int32_t g_showFps;                           // 0x004b6b38  non-zero: prefix the line with "%3d fps"
extern int g_childDeleteFlag;                       // 0x004b6b3c  set while StackAndClamp deletes overflowing barks
extern char g_emptyString[4];                       // 0x004b6b40  ""
extern int32_t g_bbRequestSerial;                   // 0x004b6b44
extern int32_t g_bbRequestPending;                  // 0x004b6b48
extern int32_t g_bbWrapFull;                        // 0x004b6b4c  set when a wrap hit the line limit
extern int g_bmGoodSuffix[0x64];                    // 0x004b6b50  int[], good-suffix shifts
extern int g_bmBadChar[0x100];                      // 0x004b6ce0  int[256], bad-character shifts
extern char g_parenBuf[0xa0];                       // 0x004b70e0
extern CMultiPlacer *g_multiPlacer;                 // 0x004b7180  the one placer instance
extern void *g_loadedMulti;                         // 0x004b7184
extern int g_desktopChanged;                        // 0x004b7188
extern int32_t g_004b718c;                          // 0x004b718c
extern int32_t g_cascadeSlotCount;                  // 0x004b7190
extern int32_t g_cascadeCapacity;                   // 0x004b7194
extern int32_t *g_cascadeSlotX;                     // 0x004b7198
extern int32_t *g_cascadeSlotY;                     // 0x004b719c
extern char g_titlePartB[0x50];                     // 0x004b71a0  name of the chosen entry
extern CInputControlGump *g_awaitingTargetControl;  // 0x004b71f0  ? "control awaiting a target" (pass2 comment)
extern uint8_t g_circleTrans_storage[0x10];         // 0x004b71f8  CCircleTrans object; its bounding rect is at +0
extern int32_t g_004b7208;                          // 0x004b7208
extern int32_t g_004b7214;                          // 0x004b7214
extern int32_t g_004b7218;                          // 0x004b7218
extern int32_t g_004b721c;                          // 0x004b721c
extern int g_ccVal220;                              // 0x004b7220
extern int32_t g_loginGumpHook;                     // 0x004b7224
extern int32_t g_004b7228;                          // 0x004b7228
extern int32_t g_004b722c;                          // 0x004b722c
extern int32_t g_004b7230;                          // 0x004b7230
extern int g_useWideWindow;                         // 0x004b7234  nonzero: RegisterClassW / CreateWindowExW path
extern uint32_t US_Language;                        // 0x004b7238
extern int32_t g_004b723c;                          // 0x004b723c
extern int32_t so_stereoFlip;                       // 0x004b7240  nonzero: pan is negated
extern uint32_t g_cpuVendorFlag;                    // 0x004b7244  set when the CPUID vendor differs from the expected string
extern CMissileAnim *g_missileTable[0x16];          // 0x004b7250  20 slots, 0 = free
extern int g_visibleCells[0x800];                   // 0x004b72a8  cell indices (y << 6 | x) relative to the window
extern uint8_t g_roofVisited[0x1000];               // 0x004b92a8  64 x 64 visited map of the roof flood fill
extern int g_scrollX;                               // 0x004ba2a8
extern int g_scrollY;                               // 0x004ba2ac
extern CMobileAnimQueue *g_activeAnim[0x14];        // 0x004ba2b0  20 slots, 0 = free
extern int g_fadingPass;                            // 0x004ba300  1 while the foliage cache is being refreshed
extern int g_showHidden;                            // 0x004ba304  tile id 2 is skipped while this is 0
extern int g_visibleCellCount;                      // 0x004ba308  entries of the visible-cell index list
extern int32_t g_clipBusyBB;                        // 0x004ba30c
extern int g_cosTable[0x100];                       // 0x004ba310  256 entries, 16.16 fixed point
extern int g_sinTable[0x101];                       // 0x004ba710
extern uint16_t g_004bab14;                         // 0x004bab14
extern uint16_t g_004bab18;                         // 0x004bab18
extern uint16_t g_004bab1c;                         // 0x004bab1c
extern uint16_t g_004bab20;                         // 0x004bab20
extern int g_dtorFlag;                              // 0x004bab24
extern int g_inputRedrawing;                        // 0x004bab28  set while the text is being drawn into the bitmap
extern signed char g_stepFrameTable[1];             // 0x004bab38  2 bytes per graphic: walk frames, run frames
extern uint8_t PM_animInfo[0x7d0];                  // 0x004bab39
extern int g_suppressSpeech;                        // 0x004bb310  0 = not fading; counts 1..0x1f
extern uint8_t *g_flushTabA[0x1000];                // 0x004bb318  per entry: pointer into the back buffer
extern uint8_t *g_flushTabB[0x1000];                // 0x004bf318  per entry: pointer into the ring buffer
extern uint32_t g_ringBase;                         // 0x004c3318
extern int g_flushRects[0x4000];                    // 0x004c3320  4 ints (l, t, r, b) per entry
extern int g_dirtyL;                                // 0x004d3320  dirty rect being flushed
extern int g_dirtyT;                                // 0x004d3324
extern int g_dirtyR;                                // 0x004d3328
extern int g_dirtyB;                                // 0x004d332c
extern int g_ringScratchB;                          // 0x004d3330
extern int g_flushCount;                            // 0x004d3334  entries in the flush list
extern HDC g_memDC;                                 // 0x004d3338
extern CGump *g_updateCursor;                       // 0x004d333c
extern int32_t g_localTimezone;                     // 0x004d3340  bias / 60
extern uint8_t *g_verdataBase;                      // 0x004d3348  mapped view of the file
extern void *g_verdataUnk;                          // 0x004d334c  file stream object (0x90 bytes)
extern VerNode *g_verdataTable[0x4400];             // 0x004d3350  chains of 0x14-byte nodes, index file * 0x400 + (block & 0x3ff)
extern CacheNode *g_hashTable[0x10e45];             // 0x004e4350  0x11000 slots: [(file << 12) + (key & 0xfff)]
extern uint8_t g_00527c66[0x6ea];                   // 0x00527c66
extern int g_verdataCount;                          // 0x00528350
extern CacheNode *g_nodeFreeList;                   // 0x00528354  free hash nodes, linked through +0x10
extern CacheNode *g_nodePool;                       // 0x00528358
extern int g_fileOnHD[0x13];                        // 0x00528360  17 flags: 1 = whole file mapped from the hard disk
extern uint8_t dat_005283ac[4];                     // 0x005283ac
extern int g_skinHueTable[0x3e];                    // 0x005283b0  first hue index of each skin group
extern int HG_GridHit;                              // 0x005284a8  the last mouse event was inside the hue grid
extern int g_005284b0;                              // 0x005284b0
extern int g_skinHueCount;                          // 0x005284b4  entries of the skin hue table
extern int g_005284b8;                              // 0x005284b8
extern int HG_Closing;                              // 0x00528538  set while a hue gump is being closed by the right button
extern int g_inputActive;                           // 0x005285c8  ? 0 forces cursor 0x11
extern int g_promptReply;                           // 0x005285cc  1: the line answers a prompt
extern int g_promptSerialA;                         // 0x005285d0
extern int g_promptSerialB;                         // 0x005285d4
extern int g_entrySkipNext;                         // 0x005285d8  swallow one character
extern uint8_t g_unknownObj2_storage[4];            // 0x005285e0
extern uint32_t g_005285e4;                         // 0x005285e4
extern uint32_t g_005285e8;                         // 0x005285e8
extern void *g_005285ec;                            // 0x005285ec
extern uint8_t g_005285f0;                          // 0x005285f0
extern uint8_t g_005285f1;                          // 0x005285f1
extern uint8_t g_005285f2;                          // 0x005285f2
extern uint8_t g_005285f3;                          // 0x005285f3
extern void *g_005285f4;                            // 0x005285f4
extern void *g_dlgOwner;                            // 0x005285f8
extern int32_t MG_NotorietyOwner;                   // 0x005285fc
extern int32_t g_warnOnAttack;                      // 0x00528604  option: confirm before attacking an innocent
extern uint8_t g_NeighborZ4[8];                     // 0x00528608  z of the four neighbour cells, filled by GetNeighborZ
extern uint8_t g_mousePosLock_storage[0x18];        // 0x00528610  guards posX / posY
extern uint8_t g_mouseCursorLock_storage[0x18];     // 0x00528628  guards curRecord
extern uint8_t g_presentLock_storage[0x18];         // 0x00528640  guards the whole draw
extern uint8_t g_mouseSizeLock_storage[0x18];       // 0x00528658  guards curW / curH
extern int g_cursorSkipImage;                       // 0x00528670  ? set by SaveBackground, cleared before the capture
extern int g_tintColor;                             // 0x00528674  ? 15-bit colour multiplied into cursors 0 .. 0x16
extern int g_dropTargetZ;                           // 0x00528680  z of the step
extern int wq_stepFrames;                           // 0x00528684  frames this step takes
extern int g_dropZ;                                 // 0x00528688  player z when the step started
extern uint32_t g_combatTimeB;                      // 0x0052868c  set from 0x006f47d0 at every request
extern int g_dropping;                              // 0x00528690  the current walk step changes z
extern int g_getZValWarned;                         // 0x00528694  the bad-offset message box was shown
extern uint8_t g_regionMusic_storage[0x18];         // 0x00528698
extern uint8_t g_animData_storage[0x40];            // 0x005286b0  0x48 bytes per art id; +0x41 frame count, +0x43 delay
extern uint8_t g_005286f0[0x11ffc0];                // 0x005286f0  0x48-byte slots
extern uint8_t g_msgTarget_storage[4];              // 0x006486b0  on-screen message / journal object
extern CTextLine *g_textListHead;                   // 0x006486b4  node: text at +0x00, next at +0x18
extern CTextLine *TG_TextListTail;                  // 0x006486b8
extern uint8_t g_barkState;                         // 0x006486c0  1 = normal, 2 = prefixed, 3 = "You see:"
extern char g_barkPrefix[0x1f];                     // 0x006486c1  name the bark gump shows ("YOU" / "???")
extern uint8_t g_weatherObj_storage[0x140];         // 0x006486e0
extern uint8_t g_ddraw_storage[4];                  // 0x00648820
extern void *g_ddSurface;                           // 0x00648824  IDirectDrawSurface used for GetDC / ReleaseDC
extern int g_bFullscreen;                           // 0x0064882c
extern uint16_t g_loginServerPort[8];               // 0x00648838
extern uint8_t g_walk_storage[0x10];                // 0x00648848
extern int g_altFacingFlag;                         // 0x00648858
extern int g_walkTick;                              // 0x0064885c  first GetAnimFrame argument of the player
extern uint32_t g_walkFlag648860;                   // 0x00648860
extern uint32_t g_followingFlag;                    // 0x00648864
extern int g_walkDirState;                          // 0x00648868  fifth GetAnimFrame argument of the player
extern uint8_t g_gumpMgr_storage[4];                // 0x006488c0  this for gump-list calls
extern CGump *g_gumpListTail;                       // 0x006488c4  last top-level gump (previous at +0x3c)
extern CGump *g_gumpListHead;                       // 0x006488c8
extern CBarkGump *g_barkListHead;                   // 0x006488cc
extern void *g_focusedInput;                        // 0x006488d0  gump that currently has the keyboard focus
extern CGump *g_scrollChoiceGump;                   // 0x006488d4  ? set to the newest YesNo gump (the same global holds other live gumps)
extern CGump *g_pointerGump;                        // 0x006488d8  gump that last received "enter"
extern CClientCfgGump *g_clientCfgGump;             // 0x006488dc  client config gump
extern CRadarGump *g_radarGump;                     // 0x006488e0
extern CGump *g_prevGump;                           // 0x006488e8  gump that is made visible again when this one goes away
extern CGump *g_dlgParent;                          // 0x006488ec  parent gump of the dialogs
extern CGump *g_pMainMenu;                          // 0x006488f0  ? name from the first pass (constructor 0x00423180)
extern CCharCreationGump *g_charCreationGump;       // 0x006488f4
extern CCharStatGump *g_pCharStatGump;              // 0x006488f8
extern uint8_t *g_pInitSkillGump;                   // 0x006488fc
extern CCityChoiceGump *g_cityChoiceGump;           // 0x00648900
extern CGump *g_pSkillWindow;                       // 0x00648904
extern CGump *g_fightingGump;                       // 0x00648908
extern CGump *g_waitingGump;                        // 0x00648910  modal message / waiting gump
extern CGump *g_loginObj648914;                     // 0x00648914
extern CContainerGump *g_openContainerGump;         // 0x00648918
extern uint32_t g_paperdollRedrawFlag;              // 0x00648920
extern int32_t g_deskQueueActive;                   // 0x00648924  saved desktop layout is being applied
extern uint32_t g_loginFlag648928;                  // 0x00648928
extern int32_t g_deskQueueCount;                    // 0x0064892c
extern uint8_t g_deskQueue[0x578];                  // 0x00648930  0x1c-byte entries (see gump_manager.h DesktopEntry)
extern int32_t dr_publishedCount2;                  // 0x00648ea8
extern int32_t g_dirtyCount;                        // 0x00648eac
extern int32_t g_00648eb0[0xfa1];                   // 0x00648eb0  1000 rects of 4 ints
extern HBITMAP dop_backBufferBitmap;                // 0x0064cd34
extern CObj *g_statusBlock;                         // 0x0064cd38
extern CGump *g_dragGump2;                          // 0x0064cd3c
extern char g_0064cd40[0x1e];                       // 0x0064cd40
extern uint8_t g_0064cd5e;                          // 0x0064cd5e
extern uint16_t g_0064cd60;                         // 0x0064cd60
extern uint16_t g_0064cd62;                         // 0x0064cd62
extern uint16_t g_0064cd64;                         // 0x0064cd64
extern uint16_t g_0064cd66;                         // 0x0064cd66
extern uint16_t g_0064cd68;                         // 0x0064cd68
extern uint16_t g_0064cd6a;                         // 0x0064cd6a
extern uint16_t g_0064cd6c;                         // 0x0064cd6c
extern uint16_t g_0064cd6e;                         // 0x0064cd6e
extern uint16_t g_0064cd70;                         // 0x0064cd70
extern uint32_t g_0064cd74;                         // 0x0064cd74
extern uint16_t g_0064cd78;                         // 0x0064cd78
extern uint16_t g_0064cd7a;                         // 0x0064cd7a
extern CGump *g_gumpDeleteCursor;                   // 0x0064cd7c  iteration cursor the gump destructor adjusts
extern const char *g_backupServerSpec[8];           // 0x0064cd88
extern uint8_t g_0064cda8_storage[4];               // 0x0064cda8
extern uint8_t g_frameDataAllocator_storage[4];     // 0x0064cdac
extern uint8_t g_pathSearch_storage[0xc];           // 0x0064cdb0  the A* search object
extern PathNode *g_pathCurrentNode;                 // 0x0064cdbc  current node of the path being followed
extern PathNode *g_pathStartNode;                   // 0x0064cdc0
extern int g_pathActive;                            // 0x0064cdc4
extern int g_pathX;                                 // 0x0064cdc8
extern int g_pathY;                                 // 0x0064cdcc
extern int g_pathZ;                                 // 0x0064cdd0
extern int g_pathFacing;                            // 0x0064cdd4
extern CObj *g_selectedObj;                         // 0x0064cddc
extern uint8_t g_0064cde0_storage[8];               // 0x0064cde0
extern uint8_t g_fontMgr_storage[0x40070];          // 0x0064cde8  per file-0x21 block: object of 0x704 bytes
extern uint8_t g_objMgr_storage[4];                 // 0x0068ce58  object manager: world list tail/head, serial list at +0x24
extern CObj *g_worldListHead;                       // 0x0068ce5c  newest object (previous at +0x44)
extern CObj *g_lightListHead;                       // 0x0068ce60  light list (next at +0x48, prev at +0x4c)
extern CObj *g_lightListTail;                       // 0x0068ce64
extern CObj *g_list20000Head;                       // 0x0068ce68  objects with tiledata flag 0x20000, next at +0x58
extern CObj *g_list20000Tail;                       // 0x0068ce6c
extern CObj *g_list70Head;                          // 0x0068ce70  next at +0x60, prev at +0x64
extern CObj *g_list70Tail;                          // 0x0068ce74
extern uint8_t g_objectManager_storage[4];          // 0x0068ce78  objects with tiledata flag 0x1000000 (next at +0x50)
extern CDynamicObj *g_dynListHead;                  // 0x0068ce7c  list of live items: next at +0x80, prev at +0x84
extern CDynamicObj *g_dynListTail;                  // 0x0068ce80
extern CMobile *g_mobileListHead;                   // 0x0068ce84  next at +0xbc
extern void *g_heldGump;                            // 0x0068ce8c  the drag (held item) gump
extern uint16_t g_heldGraphic;                      // 0x0068ce96
extern int8_t g_heldByteAt99;                       // 0x0068ce99  ? signed byte added to the graphic for the tiledata lookup
extern int g_heldSerial;                            // 0x0068ce9c  serial of the held item, -1 when none
extern uint16_t g_heldHue;                          // 0x0068cea0  low word = hue + 1 (0 = none)
extern uint16_t g_heldCount;                        // 0x0068cea2
extern uint32_t g_heldFlags;                        // 0x0068cea4
extern int g_mobileCount;                           // 0x0068ceb4  number of mobiles in the world
extern uint8_t *g_tileData;                         // 0x0068ceb8  0x28-byte records: +0 flags, +0xc animation body
extern uint8_t *g_landTable;                        // 0x0068cebc  0x1c-byte land tile records, a word at +4 is the flag tested
extern int g_holdingFlag;                           // 0x0068cec8
extern char g_resolvedPath[0x400];                  // 0x0068ced8
extern uint8_t g_artCache_storage[8];               // 0x0068d2d8
extern uint8_t g_mouse_storage[0x14];               // 0x0068d2e0
extern void *g_dragGump;                            // 0x0068d2f4  gump that captures the mouse while a drag is on
extern uint8_t g_fileManager_storage[0xa4];         // 0x0068d980  data file pair reader (files 0xd / 0xe)
extern uint8_t *g_cacheBase;                        // 0x0068da24  = g_fileManager.base
extern CacheEntry *g_cacheEntries;                  // 0x0068da28  = g_fileManager.entries
extern uint32_t g_fpsSecondTick;                    // 0x0068da2c  tick count at the start of the current frame-rate second
extern char g_lineBuf[0x100];                       // 0x0068da30
extern uint8_t g_soundManager_storage[0x18];        // 0x0068db30  the object at 0x0068db30
extern int g_ccMusicVal;                            // 0x0068db48
extern uint8_t g_byte_68db4c;                       // 0x0068db4c
extern uint8_t g_map_storage[4];                    // 0x0068dbb8  world map / tile-block view
extern int g_viewCenterX;                           // 0x0068dbbc  g_mapObject + 4
extern int g_viewCenterY;                           // 0x0068dbc0  g_mapObject + 8
extern int MG_ViewZ;                                // 0x0068dbc4
extern char g_worldName[0x100];                     // 0x0068dbcc
extern int g_scrollBase;                            // 0x0068dccc  world-map constant of the screen-x formula
extern uint32_t g_sceneW0;                          // 0x0068dce4
extern uint32_t g_sceneW1;                          // 0x0068dce8
extern uint32_t g_sceneW2;                          // 0x0068dcec
extern uint32_t g_sceneW3;                          // 0x0068dcf0
extern int g_mapBlocksY;                            // 0x0068dcf4
extern int g_mapBlocksX;                            // 0x0068dcf8
extern uint32_t g_0068dd94;                         // 0x0068dd94
extern uint8_t g_clientState_storage[4];            // 0x0068dd98
extern uint16_t g_colourMap[0x8000];                // 0x0068dd9c  16-bit colour remap table (index = source pixel)
extern uint16_t *g_backBuffer;                      // 0x0069dd9c  640-wide 16-bit surface
extern CGump *g_screenGump;                         // 0x0069dda0  object whose +0xd4 is the targeted mobile
extern int g_viewOffsetX;                           // 0x0069dda4
extern int g_viewOffsetY;                           // 0x0069dda8
extern int g_paletteOn;                             // 0x0069e0ac
extern HPALETTE g_palette;                          // 0x0069e0b4
extern uint8_t g_worldPicker_storage[4];            // 0x0069e0c0  first word of the picker object; zero when the map is used
extern int g_cutawayZ;                              // 0x0069e0c4
extern uint8_t g_packetBuilder_storage[8];          // 0x0069e0e0  this for the packet builders 0x004631xx..0x004654xx
extern uint8_t g_app_storage[4];                    // 0x0069e0e8  CApp; 0x00482be0 takes the cursor id
extern void *g_pktSender;                           // 0x0069e0ec  connection object; the dword at +0x20028 goes out in 0x00 and 0x5D
extern uint32_t g_playerSerial;                     // 0x0069e0f0  serial of the player mobile
extern char g_password[0x1e];                       // 0x0069e274
extern char g_playerName[0x1f];                     // 0x0069e292
extern uint8_t g_0069e2b1;                          // 0x0069e2b1
extern uint8_t g_0069e2b4;                          // 0x0069e2b4
extern uint8_t g_0069e2b5;                          // 0x0069e2b5
extern uint8_t g_0069e2b6;                          // 0x0069e2b6
extern uint16_t g_0069e2b8;                         // 0x0069e2b8
extern uint16_t g_0069e2ba;                         // 0x0069e2ba
extern uint16_t g_0069e2bc;                         // 0x0069e2bc
extern uint16_t g_0069e2be;                         // 0x0069e2be
extern uint16_t g_0069e2c0;                         // 0x0069e2c0
extern uint8_t g_0069e2c2;                          // 0x0069e2c2
extern uint8_t g_0069e2c3;                          // 0x0069e2c3
extern uint8_t g_0069e2c4;                          // 0x0069e2c4
extern uint8_t g_0069e2c5;                          // 0x0069e2c5
extern uint8_t g_0069e2c6;                          // 0x0069e2c6
extern uint8_t g_0069e2c7;                          // 0x0069e2c7
extern uint8_t g_selectedServer;                    // 0x0069e2c8  signed byte compared against the table
extern uint8_t g_0069e2c9;                          // 0x0069e2c9
extern DWORD g_frameInterval;                       // 0x0069e2cc
extern char g_appDataPath[0x200];                   // 0x0069e2d0  CApp::dataPath of the global app object
extern uint16_t g_colorSay;                         // 0x0069e4d0
extern uint16_t g_colorSayFont;                     // 0x0069e4d2
extern uint16_t g_hue1;                             // 0x0069e4d4
extern uint16_t g_colorEmoteB;                      // 0x0069e4d6
extern uint16_t g_slotHue0[5];                      // 0x0069e4d8
extern uint16_t g_slotHue1[5];                      // 0x0069e4e2
extern void *g_textEntry;                           // 0x0069e4ec  CTextEntry: text at +0x0c, CInputGump at +0x11c
extern int g_gameHour;                              // 0x0069e4f0
extern int g_gameMinute;                            // 0x0069e4f4
extern int g_gameSecond;                            // 0x0069e4f8
extern int32_t g_modalActive;                       // 0x0069e50c
extern uint32_t wq_ackLatency;                      // 0x0069e514  running average of request -> ack time
extern int32_t g_installMode;                       // 0x0069e518  1 = maximum install
extern int32_t g_cacheMB;                           // 0x0069e51c
extern char g_0069e520[0x104];                      // 0x0069e520
extern int g_profanityOn;                           // 0x0069e624
extern int32_t g_0069e628;                          // 0x0069e628
extern int32_t g_ccVal62c;                          // 0x0069e62c
extern char g_0069e630[0x20];                       // 0x0069e630
extern int g_ccSel650;                              // 0x0069e650
extern int32_t g_ccSel654;                          // 0x0069e654  number of banned words
extern char g_ccNames658[0x258];                    // 0x0069e658  0x1e-byte name slots
extern int g_ccVals8b0[0x14];                       // 0x0069e8b0
extern char g_ccNames900[0x960];                    // 0x0069e900  0x1e bytes each
extern int32_t g_ccSel260;                          // 0x0069f260  mode 2 list: entry count
extern int32_t g_ccSel264;                          // 0x0069f264  mode 1 list: entry count
extern int32_t g_ccVal268;                          // 0x0069f268
extern char g_ccNames26c[0x258];                    // 0x0069f26c  mode 2: 0x1e-byte names
extern char g_ccNames4c4[0x258];                    // 0x0069f4c4  mode 1: 0x1e-byte names
extern int32_t g_ccVals71c[0x14];                   // 0x0069f71c  mode 2: one dword per name
extern int32_t g_ccVals76c[0x14];                   // 0x0069f76c  mode 1: one dword per name
extern int32_t g_ccVals7bc[0x14];                   // 0x0069f7bc  parallel to g_values2 (g_values2 + 0xa0 bytes)
extern int g_flags_69f80c[0x14];                    // 0x0069f80c
extern uint8_t *g_macroListHead;                    // 0x0069f85c  macro nodes: key +0, ctrl +8, alt +0xc, shift +0x10, next +0xe0
extern int32_t g_ccListCount;                       // 0x0069f860
extern uint32_t g_targetCursorId;                   // 0x0069f864  cursor id sent back in 0x6C
extern int32_t g_fightMode;                         // 0x0069f868  1 selects the first toggle
extern int32_t g_fightValueA;                       // 0x0069f86c
extern int32_t g_fightValueB;                       // 0x0069f870
extern int32_t g_serverCount;                       // 0x0069f874
extern char **g_serverNames;                        // 0x0069f878
extern uint16_t *g_serverValues;                    // 0x0069f87c  2 bytes per server (read as short and as signed byte)
extern uint8_t *g_serverKeyA;                       // 0x0069f880  per server byte (percent full)
extern uint8_t *g_serverFlags;                      // 0x0069f884  per server byte (time zone); '2' is skipped by the sort
extern uint8_t *g_serverSortKeys;                   // 0x0069f888
extern uint16_t *g_serverPing;                      // 0x0069f88c  per server: sum of the 20 rtts
extern uint32_t *g_pingServerIps;                   // 0x0069f890  4 bytes per server
extern int g_locCount;                              // 0x0069f894  number of city slots
extern char **g_locNames;                           // 0x0069f898  pointer to an array of city entries (0 = unused)
extern int32_t *g_locIds;                           // 0x0069f89c
extern uint8_t g_charFlags;                         // 0x0069f8a0
extern int32_t g_charSlotCount;                     // 0x0069f8a4
extern char g_charNames[0x96];                      // 0x0069f8a8  5 x 0x1e: slotPtr - 0x96 in the loop
extern char g_slotNeedsPw[0x96];                    // 0x0069f93e  5 x 0x1e, ends at 0x0069f9d4; the first table is 0x96 below
extern int g_selectedCharSlot;                      // 0x0069f9e8  index of the field that starts focused
extern char g_titlePart2[0x3c];                     // 0x0069f9ec
extern char g_acctName[0x1e];                       // 0x0069fa28  30 bytes, sent in 0x91
extern char g_accountPassword[0x22];                // 0x0069fa46  30 bytes, sent in 0x91
extern const char *g_loginServerSpec[8];            // 0x0069fa68  "host,port" per login server
extern uint16_t g_backupServerPort[8];              // 0x0069fa88
extern uint8_t g_musicManager_storage[0x10];        // 0x0069fa98
extern int g_ccSoundVal;                            // 0x0069faa8
extern uint8_t g_byte_69faac;                       // 0x0069faac
extern UINT mo_mciDeviceGlobal;                     // 0x0069fbbc  MusicManager::mciDevice of the static instance
extern char g_resolvedPath2[0x400];                 // 0x0069fbc8
extern uint32_t g_loginServerIp[8];                 // 0x0069ffc8  resolved address per login server (0 = not resolved)
extern uint8_t g_cellGrid_storage[4];               // 0x0069ffe8  cell table origin added to the cell's x
extern int g_cellOriginY;                           // 0x0069ffec
extern CObj *g_tileBuckets[0xfea];                  // 0x0069fff8  64 x 64 heads of per-cell object chains (next at +0x14)
extern uint16_t g_hueColors[0x2c];                  // 0x006a3fa0  [colour + hue * 0x2c]
extern uint8_t g_hueTable_storage[0x3c];            // 0x006a3ff8  0x2c entries per hue, indexed by the pixel's top 5 bits
extern uint8_t g_fontHueTable[0x7764];              // 0x006a4034  0x58 bytes per entry, hue word first
extern uint16_t g_mapPalette[0x1c7d0];              // 0x006ab798  32-entry colour ramp (555)
extern uint8_t g_006e4738_storage[0x10008];         // 0x006e4738
extern uint8_t g_tickIndexHead_storage[8];          // 0x006f4740
extern uint32_t g_backupServerIp[8];                // 0x006f4748
extern int32_t g_loginByAddr;                       // 0x006f4768  nonzero: character-list refresh variant of the login flow
extern uint16_t g_loginByAddrPort;                  // 0x006f476c
extern const char *g_cmdPrefix;                     // 0x006f4770  text of the command being targeted
extern int32_t g_targetCallbackActive;              // 0x006f4774  1 while the busy / target cursor is shown
extern int32_t g_targetCursorType;                  // 0x006f4778  0 text only, 1 location, 2 object
extern uint8_t g_targetFlags;                       // 0x006f4780  cursor type byte of the pending target request
extern uintptr_t g_targetCallback;                  // 0x006f4784  code address, called with ECX = clicked object
extern int32_t g_cmdNumber;                         // 0x006f4788  non-zero: send "<number> ..." with packet 0x2f
extern int g_animDirtyFlag;                         // 0x006f478c
extern int g_suppressFlag;                          // 0x006f4794  nonzero: focus changes do nothing / deleting children
extern uint32_t g_authKey;                          // 0x006f4798  key from the 0x8C packet
extern int g_flag6f479c;                            // 0x006f479c
extern int32_t g_useHuffman;                        // 0x006f47a0
extern int g_006f47a4;                              // 0x006f47a4
extern int32_t g_catchUpFlag;                       // 0x006f47a8
extern int32_t g_rememberPassword;                  // 0x006f47ac  ? meaning unknown, passed to the check-box ctor
extern uint32_t g_hostIp;                           // 0x006f47b0
extern uint16_t g_hostPort;                         // 0x006f47b4
extern int32_t g_006f47b8;                          // 0x006f47b8
extern uint16_t g_006f47bc;                         // 0x006f47bc
extern int32_t g_loginServerCount;                  // 0x006f47c0
extern int32_t g_backupServerCount;                 // 0x006f47c4
extern int g_macroTarget;                           // 0x006f47c8
extern uint32_t g_gameTick;                         // 0x006f47cc
extern int g_combatTimeA;                           // 0x006f47d0
extern char *g_keynameSlots[0x59];                  // 0x006f47d8  0x59 key name strings, scan code order
extern int32_t g_flag6f493c;                        // 0x006f493c  ? cleared when going back to the main menu
extern uint8_t g_006f4940_storage[6];               // 0x006f4940
extern uint16_t lt_edgeB[1];                        // 0x006f4946
extern uint16_t lt_edgeA[0x400];                    // 0x006f4948
extern int32_t dr_rects[0x1000];                    // 0x006f5148  0x100 entries of 4 ints
extern int32_t lt_rowEnd[0x2c];                     // 0x006f9148  W: Z + B
extern int32_t lt_rowStart[0x2c];                   // 0x006f91f8  Z: first column of the row
extern uint8_t g_lightLut[0x400];                   // 0x006f92a8  [light level 0..31][channel value 0..31]
extern uint32_t lt_lookup[0x8000];                  // 0x006f96a8  0x8000 entries
extern uint8_t g_shadeTableB[0x10];                 // 0x007196a8
extern int32_t lt_blue4[0x3fc];                     // 0x007196b8  the same tables at column 4
extern uint8_t g_shadeTableG[0x10];                 // 0x0071a6a8
extern int32_t lt_green4[0x3fc];                    // 0x0071a6b8
extern uint8_t g_shadeTableR[0x10];                 // 0x0071b6a8
extern int32_t lt_red4[0x3fc];                      // 0x0071b6b8
extern uint8_t g_0071c6a8_storage[8];               // 0x0071c6a8
extern int32_t lt_rowLength[0x2c];                  // 0x0071c6b0  B: pixels in the row
extern void *g_lightFile;                           // 0x0071c760  memory-mapped light.mul object
extern uint8_t *g_lightIndex;                       // 0x0071c764  view of lightidx.mul: 12-byte records
extern int32_t lt_srcOffset[0x2c];                  // 0x0071c768  A: pixel offset of the row inside the footprint bitmap
extern int32_t dr_published[0xfff];                 // 0x0071c818  copy of the queue handed to the next stage
extern uint16_t lt_spanMaskB[1];                    // 0x00720816  the same table seen one word earlier
extern uint16_t lt_spanMaskA[0x400];                // 0x00720818  0x400 words
extern uint8_t *g_lightBitmaps;                     // 0x00721018  view of light.mul
extern void *g_lightIdxFile;                        // 0x0072101c  memory-mapped lightidx.mul object
extern int g_scrollOffset;                          // 0x00721020  pixel offset of the ring start (0 = no scroll)
extern int32_t g_scrollOffsetX;                     // 0x00721024
extern int32_t g_scrollOffsetY;                     // 0x00721028
extern int g_lightAmbient;                          // 0x0072102c
extern int g_playerLight;                           // 0x00721030
extern int32_t dr_fullRedraw;                       // 0x00721034
extern int32_t dr_count;                            // 0x0072103c
extern int32_t dr_publishedCount;                   // 0x00721040
extern int g_poolBuilding;                          // 0x00721044  0x78-byte object slab construction in progress
extern int g_poolLive;                              // 0x00721048  live objects
extern int g_poolTotal;                             // 0x0072104c  total slots
extern char *g_poolFree;                            // 0x00721050  free list head, next pointer at +0x40
extern int g_dynPoolBuilding;                       // 0x00721054  dynamic object slab construction in progress
extern int32_t g_pool_dynLive;                      // 0x00721058
extern int32_t pool_dynTotal;                       // 0x0072105c
extern char *g_pool_dynFree;                        // 0x00721060  free list of CDynamicObj, next pointer at +0x80
extern int32_t pool_tileBuilding;                   // 0x00721064
extern int g_obj730Live;                            // 0x00721068
extern int32_t pool_tileTotal;                      // 0x0072106c
extern CObjVt49f730 *g_obj730Free;                  // 0x00721070  free list, next pointer at +0x14
extern int g_terrBuilding;                          // 0x00721074  land tile slab construction in progress
extern int g_terrLive;                              // 0x00721078
extern int g_terrTotal;                             // 0x0072107c
extern CTerrainTileOps *g_terrFree;                 // 0x00721080
extern char g_itemNameBuf[0x104];                   // 0x00721088
extern int32_t g_combatTargetSerial;                // 0x0072118c
extern CObj *g_player;                              // 0x00721190  player mobile; equipment slots at +0xc4
extern PendingUpdate *g_pendingUpdates;             // 0x00721194
extern int g_pendingCount;                          // 0x00721198
extern int g_zeroStepWarned;                        // 0x0072119c
extern int32_t g_ccVal1a0;                          // 0x007211a0
extern CEffectSequence *g_effectSequence;           // 0x007211a4  queue object, head node pointer at +4
extern int32_t g_redrawBlock;                       // 0x007211a8
extern int32_t g_inGame7211ac;                      // 0x007211ac  nonzero once the game server connection is established
extern int g_sequencing;                            // 0x007211b0  nonzero: queue the request instead of creating it
extern void *g_oldConn;                             // 0x007211b4  connection kept until login completes
extern int g_attackSerial;                          // 0x007211bc
extern uint32_t g_pingSendTicks[0x280];             // 0x007211c0
extern int32_t g_pingRtts[0x280];                   // 0x00721bc0
extern uint32_t g_statusRing[8];                    // 0x007225c0  ring of up to 8 status gumps of other mobiles
extern int32_t g_statusRingCount;                   // 0x007225e0
extern int g_radarDefaultTile;                      // 0x007225e4  colour used for tiles >= 0x4000 and tile 2
extern uint8_t g_animOverlayBuffer_storage[0x24018];// 0x007226a0  first dword of the buffer: table entry in use
extern int32_t g_skillRowTop;                       // 0x007466b8  top of the group row last hit-tested
extern char g_skillNames[0xfa0];                    // 0x007466c0  0x50-byte records
extern int32_t g_groupCount;                        // 0x00747660
extern int32_t g_skillLockState[0x32];              // 0x00747668  [skill]
extern uint8_t g_skillOrder[0x38];                  // 0x00747730  skill indices sorted by name
extern int32_t g_groupFlags[0xc];                   // 0x00747768  [group], cleared by the constructor
extern int g_skillCount;                            // 0x00747798  number of skills listed
extern char g_groupNames[0xd0];                     // 0x007477a0  0x11 bytes each
extern int32_t g_skillGroup[0x32];                  // 0x00747870  [skill] -> group, ends at 0x00747938
extern uint8_t g_skillFlags[0x34];                  // 0x00747938  [skill], bit 0 = show button
extern int g_hueOverride;                           // 0x0074796c  hue index read by the hued art drawer (0x004765a0)
extern int32_t g_blitStride;                        // 0x00747970  destination row stride in pixels
extern uint16_t *g_blitDest;                        // 0x00747974  base address of the destination surface
extern uint16_t g_pktFixedCursor;                   // 0x00747978
extern uint8_t g_selectTimeout_storage[8];          // 0x00747980
extern uint8_t g_huffOut[0x10000];                  // 0x00747988  0x10000 bytes
extern int g_reconnectCountdown;                    // 0x00757988
extern int32_t *g_huffmanRoot;                      // 0x0075798c  node: {value, child0, child1}, value -1 = inner
extern int32_t g_huffOutLen;                        // 0x00757990  bytes decoded into the buffer below
extern int32_t g_huffBitPos;                        // 0x00757994  bit position of the last complete symbol
extern FILE *g_textLogFile;                         // 0x00757998  opened on first use ("a+")
extern char *g_textLogName;                         // 0x0075799c  log file name, null = no log
extern uint32_t g_labelFlag;                        // 0x007579a0  non-zero: AddLine*AndLog does nothing
extern int g_unifontLoaded;                         // 0x007581a8
extern MappedFile *uf_file;                         // 0x007581ac
extern uint8_t *g_unifontTable;                     // 0x007581b0  unifont.mul data: 0x10000 offsets, then glyph records
extern int32_t g_00758210;                          // 0x00758210
extern int g_00758214;                              // 0x00758214
extern int g_00758218;                              // 0x00758218
extern int AUO_unk75821c;                           // 0x0075821c
extern int g_00758220;                              // 0x00758220
extern int32_t g_00758224;                          // 0x00758224
extern int g_00758228;                              // 0x00758228
extern int g_0075822c;                              // 0x0075822c
extern int g_tileGroupBase;                         // 0x00758230  added to the tiledata group index for the (empty) 0x004242b0 call
extern int g_00758234;                              // 0x00758234
extern int g_00758238;                              // 0x00758238
extern int32_t g_loadProgressArg;                   // 0x0075823c
extern int g_regionIdBase;                          // 0x00758240
extern int32_t g_00758244;                          // 0x00758244
extern int g_00758248;                              // 0x00758248
extern int g_0075824c;                              // 0x0075824c
extern int g_00758250;                              // 0x00758250
extern int g_00758254;                              // 0x00758254
extern int g_00758258;                              // 0x00758258
extern int g_0075825c;                              // 0x0075825c
extern int g_weatherFrame;                          // 0x00758260  frame counter
extern int g_weatherParticles;                      // 0x00758264  live particle count
extern int g_launchDirError;                        // 0x00758268  set when CreateProcessA failed with ERROR_DIRECTORY (0x10b)
extern int g_is565;                                 // 0x0075826c  non-zero: inline 0x7bef blend, zero: row helper 0x00476550
extern uint16_t *dop_backBufferCopy;                // 0x00758270
extern int dop_windowedFlag;                        // 0x00758274
extern uint8_t g_soundLock_storage[0x18];           // 0x00758288
extern int wp_shiftDown;                            // 0x007582a0
extern int wh_fullscreenFlag;                       // 0x007582a4  window has a caption and frame
extern int wp_altDown;                              // 0x007582a8
extern int wp_flag7582ac;                           // 0x007582ac
extern void *wp_macroCursor;                        // 0x007582b0
extern int32_t g_savedKeyLParam;                    // 0x007582b4
extern uint32_t im_flags;                           // 0x007582b8
extern void *im_pDD;                                // 0x007582c0
extern void *g_introSurface;                        // 0x007582c4  primary DirectDraw surface used by the intro movie
extern void *im_dsound;                             // 0x00758300
extern void *im_dsPrimary;                          // 0x00758304
extern int im_destX;                                // 0x00758314
extern int im_destY;                                // 0x00758318
extern int im_destCenterX;                          // 0x0075831c
extern uintptr_t im_frameCallback;                  // 0x00758358
extern int32_t g_savedKeyScan;                      // 0x00758360  key code of the last key press (stored in the list node)
extern uint8_t g_mainWindowObject_storage[0x400];   // 0x00758368
extern void *g_mainWindow;                          // 0x00758768  HWND of the main window
extern uint32_t g_0075876c;                         // 0x0075876c
extern HINSTANCE im_hInstance;                      // 0x007587a0
extern HCURSOR wp_cursors[0x1e];                    // 0x007587a4  0x40 dwords, zeroed by WM_CREATE
extern HCURSOR g_0075881c;                          // 0x0075881c
extern HCURSOR g_00758820;                          // 0x00758820
extern HCURSOR g_00758824;                          // 0x00758824
extern HCURSOR g_00758828;                          // 0x00758828
extern HCURSOR g_0075882c;                          // 0x0075882c
extern HCURSOR g_00758830;                          // 0x00758830
extern HCURSOR g_00758834;                          // 0x00758834
extern HCURSOR g_00758838;                          // 0x00758838
extern HCURSOR g_0075883c;                          // 0x0075883c
extern HCURSOR g_00758840;                          // 0x00758840
extern uint32_t wp_activeFlag;                      // 0x007588a4
extern uint32_t g_007588a8;                         // 0x007588a8
extern HMENU g_menuHandle;                          // 0x007588ac
extern UINT g_timerId;                              // 0x007588b0
extern uint16_t g_skillValues[0x9c];                // 0x007588b8  50 entries
extern int wq_runKey;                               // 0x007589f0
extern int g_introFrameCounter;                     // 0x007589f4
extern uint32_t wp_frameBusy;                       // 0x007589f8  a mode switch / resize is in progress
extern int im_playing;                              // 0x00758a04  intro movie playing
extern HGDIOBJ im_movieBitmap;                      // 0x00758a08
extern int32_t pb_gdiMode;                          // 0x00758a0c  blit through the surface's GDI DC instead of Lock
extern int g_soundBusy;                             // 0x00758a14
extern int g_timerReentry;                          // 0x00758a18

// The video player's data (UVA_DATA)
extern int32_t UVD_Coeff[8];                        // 0x0075d000  64 dequantised coefficients (natural order)
extern int32_t uv_0075d020[8];                      // 0x0075d020
extern int32_t uv_0075d040[8];                      // 0x0075d040
extern int32_t uv_0075d060[8];                      // 0x0075d060
extern int32_t uv_0075d080[8];                      // 0x0075d080
extern int32_t uv_0075d0a0[8];                      // 0x0075d0a0
extern int32_t uv_0075d0c0[8];                      // 0x0075d0c0
extern int32_t uv_0075d0e0[8];                      // 0x0075d0e0
extern int32_t uv_0075d100[1];                      // 0x0075d100
extern int32_t uv_0075d104[1];                      // 0x0075d104
extern int32_t uv_0075d108[1];                      // 0x0075d108
extern int32_t uv_0075d10c[1];                      // 0x0075d10c
extern int32_t uv_0075d110[1];                      // 0x0075d110
extern int32_t uv_0075d114[1];                      // 0x0075d114
extern int32_t uv_0075d118[1];                      // 0x0075d118
extern int32_t uv_0075d11c[2];                      // 0x0075d11c
extern int32_t uv_0075d124[9];                      // 0x0075d124
extern int32_t uv_0075d148[9];                      // 0x0075d148
extern int32_t uv_0075d16c[9];                      // 0x0075d16c
extern int32_t uv_0075d190[9];                      // 0x0075d190
extern int32_t uv_0075d1b4[9];                      // 0x0075d1b4
extern int32_t uv_0075d1d8[9];                      // 0x0075d1d8
extern int32_t uv_0075d1fc[9];                      // 0x0075d1fc
extern int32_t uv_0075d220[8];                      // 0x0075d220
extern int32_t uv_0075d240[8];                      // 0x0075d240
extern int32_t uv_0075d260[8];                      // 0x0075d260
extern int32_t uv_0075d280[8];                      // 0x0075d280
extern int32_t uv_0075d2a0[8];                      // 0x0075d2a0
extern int32_t uv_0075d2c0[8];                      // 0x0075d2c0
extern int32_t uv_0075d2e0[8];                      // 0x0075d2e0
extern int32_t uv_0075d300[8];                      // 0x0075d300
extern int32_t uv_0075d320[8];                      // 0x0075d320
extern int32_t uv_0075d340[8];                      // 0x0075d340
extern int32_t uv_0075d360[8];                      // 0x0075d360
extern int32_t uv_0075d380[8];                      // 0x0075d380
extern int32_t uv_0075d3a0[8];                      // 0x0075d3a0
extern int32_t uv_0075d3c0[8];                      // 0x0075d3c0
extern int32_t uv_0075d3e0[8];                      // 0x0075d3e0
extern int32_t uv_0075d400[8];                      // 0x0075d400
extern int32_t uv_0075d420[8];                      // 0x0075d420
extern int32_t uv_0075d440[8];                      // 0x0075d440
extern int32_t uv_0075d460[8];                      // 0x0075d460
extern int32_t uv_0075d480[8];                      // 0x0075d480
extern int32_t uv_0075d4a0[8];                      // 0x0075d4a0
extern int32_t uv_0075d4c0[8];                      // 0x0075d4c0
extern int32_t uv_0075d4e0[8];                      // 0x0075d4e0
extern int32_t uv_0075d500[8];                      // 0x0075d500
extern int32_t uv_0075d520[8];                      // 0x0075d520
extern int32_t uv_0075d540[8];                      // 0x0075d540
extern int32_t uv_0075d560[8];                      // 0x0075d560
extern int32_t uv_0075d580[8];                      // 0x0075d580
extern int32_t uv_0075d5a0[8];                      // 0x0075d5a0
extern int32_t uv_0075d5c0[8];                      // 0x0075d5c0
extern int32_t uv_0075d5e0[8];                      // 0x0075d5e0
extern int32_t uv_0075d600[8];                      // 0x0075d600
extern uint32_t uv_0075d620[8];                     // 0x0075d620
extern uint32_t uv_0075d640[8];                     // 0x0075d640
extern uint32_t uv_0075d660[8];                     // 0x0075d660
extern uint32_t uv_0075d680[8];                     // 0x0075d680
extern uint32_t uv_0075d6a0[8];                     // 0x0075d6a0
extern uint32_t uv_0075d6c0[8];                     // 0x0075d6c0
extern uint32_t uv_0075d6e0[8];                     // 0x0075d6e0
extern uint32_t uv_0075d700[8];                     // 0x0075d700
extern int32_t uv_0075d720[8];                      // 0x0075d720
extern int32_t uv_0075d740[8];                      // 0x0075d740
extern int32_t uv_0075d760[8];                      // 0x0075d760
extern int32_t uv_0075d780[8];                      // 0x0075d780
extern int32_t uv_0075d7a0[8];                      // 0x0075d7a0
extern int32_t uv_0075d7c0[8];                      // 0x0075d7c0
extern int32_t uv_0075d7e0[8];                      // 0x0075d7e0
extern int32_t uv_0075d800[8];                      // 0x0075d800
extern uint32_t uv_tab_red[0x60];                   // 0x0075d820  512 entries
extern uint8_t dat_0075d9a0[0x680];                 // 0x0075d9a0
extern uint32_t uv_tab_green[0xfa];                 // 0x0075e020  512 entries
extern uint8_t dat_0075e408[0x418];                 // 0x0075e408
extern uint32_t uv_tab_blue[0x49];                  // 0x0075e820  512 entries
extern uint8_t dat_0075e944[0x6dc];                 // 0x0075e944
extern int32_t UVD_Quant[0x40];                     // 0x0075f020  the brightness-scaled first table entry
extern uint32_t UVD_BitPtr;                         // 0x0075f120  address of the stream (dword aligned reads) / byte cursor
extern uint32_t UVD_BitPos;                         // 0x0075f124  bit position (VLC decoders) / valid bits in the buffer (sparse)
extern uint32_t UVD_BitBuf;                         // 0x0075f128  bit buffer of the sparse decoder
extern uint32_t uv_tab_y[0x80];                     // 0x0075f12c  128 entries
extern uint32_t uv_tab_cr_g[0x80];                  // 0x0075f32c  128 entries
extern uint32_t uv_tab_cb_b[0x80];                  // 0x0075f52c  128 entries
extern uint32_t uv_tab_cb_b32[0x100];               // 0x0075f72c
extern uint32_t uv_tab_cr_r[0x100];                 // 0x0075fb2c  256 entries, indexed by the signed byte
extern uint32_t uv_tab_cb_g[0x100];                 // 0x0075ff2c
extern uint32_t uv_tab_cr_gptr[0x100];              // 0x0076032c
extern int32_t uv_0076072c;                         // 0x0076072c
extern float uv_00760730;                           // 0x00760730
extern float uv_00760734;                           // 0x00760734
extern float uv_00760738;                           // 0x00760738
extern float uv_0076073c;                           // 0x0076073c
extern uint32_t UVD_Scan[0x3c];                     // 0x00760740  64 entries: coefficient index for each scan position
extern uint8_t dat_00760830[0x30];                  // 0x00760830
extern uint8_t dat_00760860[0xe0];                  // 0x00760860
extern uint8_t dat_00760940[0x40];                  // 0x00760940
extern uint8_t dat_00760980[0x40];                  // 0x00760980
extern uint8_t dat_007609c0[0x40];                  // 0x007609c0
extern uint8_t dat_00760a00[0x40];                  // 0x00760a00
extern uint8_t dat_00760a40[0x80];                  // 0x00760a40
extern uint8_t dat_00760ac0[0x80];                  // 0x00760ac0
extern uint8_t dat_00760b40[0x120];                 // 0x00760b40

} // extern "C"

// Views of objects that several files share, written by hand.
#include "global_views.h"
