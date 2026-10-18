// Packet dispatcher (0x00477990) and packet sender (0x00478610).
//
// HandlePacket is a method of the connection object (ECX = the CClientSocket, one stack argument, RET 4).
// While the socket is queueing (queueing != 0) only opcode 0x33 is looked at; every other packet is deferred
// through QueuePacket. Otherwise the opcode selects a handler of the packet manager object at 0x0069e0e0,
// each called as __thiscall(manager, packet) with the handler's result ignored, and the opcode is stored in
// g_LastPacketId. The switch covers opcodes 0x11..0xb0 (jump table 0x004783f4, index table 0x00478568);
// opcodes without an entry (index 0x5c) only update g_LastPacketId.
#include <cstdint>

#include "../runtime/ported.h"
#include "client_socket.h"
#include "packet_cursor.h"
#include "inbound_status_misc_ports.h"
#include "packet_handlers_objects_ports.h"
#include "packet_handlers_world_ports.h"
#include "inbound_data_updates_ports.h"
#include "inbound_book_cfg_ports.h"
#include "packet_handlers_dialogs_ports.h"
#include "login_gumps.h"
#include "packet_handlers_login_flow_ports.h"
#include "login_packets_ports.h"
#include "packet_handlers_batch24_ports.h"
#include "packet_builders_b25_ports.h"
#include "unicode_text_ports.h"
#include "bark_gump_ui_ports.h"
#include "globals.h"

#define g_packetMgr (reinterpret_cast<void *>(GADDR(g_packetBuilder_storage)))

#define PM_CALL(handler) PORT_THISCALL1(handler, void, g_packetMgr, uint8_t *)(pkt)

// 0x00477990  size 0xa62  dispatch one received packet
void __thiscall CClientSocket::HandlePacket(uint8_t *pkt)
{
    if (queueing != 0) {
        if (pkt[0] != 0x33) {
            QueuePacket(pkt);   // 0x004777f0
            return;
        }
        if (pkt[1] == 0) {
            // end of the deferred run: the 2-byte packet joins the consumed bytes
            queueing = 0;
            queuedBytes = queuedBytes + 2;
            FlushQueuedPackets();   // 0x004778c0
            return;
        }
        // any other 0x33 packet is removed from the stream
        uint32_t len = pkt::Flags(0x33);
        if (len & 0x8000)
            len = *reinterpret_cast<uint16_t *>(pkt + 1);
        len &= 0xffffu;
        int32_t n = static_cast<int32_t>(len);
        if (g_useHuffman != 0) {
            int32_t from = queuedBytes + n;
            int32_t end = g_huffOutLen;
            if (from < end) {
                do {
                    g_huffOut[from - n] = g_huffOut[from];
                    from++;
                } while (from < end);
            }
            g_huffOutLen = end - n;
            return;
        }
        int32_t from = queuedBytes + n;
        if (from < recvLen) {
            do {
                recvBuf[from - n] = recvBuf[from];
                from++;
            } while (from < recvLen);
        }
        recvLen -= n;
        return;
    }

    switch (pkt[0]) {
    case 0x33:
        if (pkt[1] != 0)
            queueing = 1;
        break;
    case 0x11: PM_CALL(&Packet_Handle0x11); break;   // CPacketManager::Handle_MobileStatus_0x11 [high]
    case 0x15: PM_CALL(&Packet_Handle0x15); break;   // CPacketManager::Handle_Follow_0x15 [high]
    case 0x1a: PM_CALL(&Packet_WorldItem); break;   // CPacketManager::HandlePacket_WorldItem_0x1A [high]
    case 0x1b: PM_CALL(&Packet_Handle0x1B); break;   // CPacketManager::HandlePacket_LoginConfirm_0x1B [high]
    case 0x1c: PM_CALL(&Packet_AsciiSpeech); break;   // CPacketManager::HandlePacket_AsciiSpeech_0x1C [high]
    case 0x1d: PM_CALL(&Packet_DeleteObject); break;   // CPacketManager::HandlePacket_DeleteObject_0x1D [high]
    case 0x20: PM_CALL(&Packet_DrawPlayer); break;   // CPacketManager::HandlePacket_DrawPlayer_0x20 [high]
    case 0x21: PM_CALL(&Packet_MoveReject); break;   // CPacketManager::HandlePacket_MoveReject_0x21 [medium]
    case 0x22: PM_CALL(&Packet_MoveAccept); break;   // CPacketManager::HandlePacket_MoveAccept_0x22 [medium]
    case 0x23: PM_CALL(&Packet_Handle0x23); break;   // CPacketManager::HandlePacket_DragAnimation_0x23 [high]
    case 0x24: PM_CALL(&Packet_Handle0x24); break;   // CPacketManager::HandlePacket_OpenContainer_0x24 [high]
    case 0x25: PM_CALL(&Packet_AddToContainer); break;   // CPacketManager::HandlePacket_AddToContainer_0x25 [high]
    case 0x27: PM_CALL(&Packet_Handle0x27); break;   // CPacketManager::HandlePacket_PickupReject_0x27 [high]
    case 0x28: PM_CALL(&Packet_DropClear_0x28); break;   // CPacketManager::HandlePacket_DropClear_0x28 [medium]
    case 0x29: PM_CALL(&Packet_DropClear_0x29); break;   // CPacketManager::HandlePacket_DropClear_0x29 [medium]
    case 0x2c: PM_CALL(&Packet_ClientCode); break;   // CPacketManager::HandlePacket_ClientCode_0x53 [low]
    case 0x2d: PM_CALL(&Packet_MobileAttributes); break;   // CPacketManager::Handle_MobileAttributes_0x2D [high]
    case 0x2e: PM_CALL(&Packet_EquipItem); break;   // CPacketManager::Handle_EquipItem_0x2E [medium]
    case 0x2f: PM_CALL(&Packet_FightOccurring); break;   // CPacketManager::HandlePacket_FightOccurring_0x2F [medium]
    case 0x30: PM_CALL(&Packet_Handle0x30); break;   // CPacketManager::Handle_SwingAnimation_0x30 [medium]
    case 0x31: PM_CALL(&Packet_Handle0x31); break;   // CPacketManager::Handle_PauseClient_0x31 [medium]
    case 0x38: PM_CALL(&Packet_Handle0x38); break;   // CPacketManager::Handle_PathfindingMove_0x38 [low]
    case 0x39: PM_CALL(&Packet_Group); break;   // CPacketManager::HandlePacket_Group_0x39 [medium]
    case 0x3a: PM_CALL(&Packet_Handle0x3A); break;   // CPacketManager::Handle_SendSkills_0x3A [medium]
    case 0x3b: PM_CALL(&Packet_Handle0x3B); break;   // CPacketManager::Handle_BuyItems_0x3B [low]
    case 0x3c: PM_CALL(&Packet_ContainerContents); break;   // CPacketManager::HandlePacket_ContainerContents_0x3C [high]
    case 0x3e: PM_CALL(&Packet_Handle0x3E); break;   // CPacketManager::Handle_Packet0x3E [low]
    case 0x3f: PM_CALL(&Packet_Handle0x3F); break;   // CPacketManager::Handle_UpdateStatics_0x3F [low]
    case 0x40: PM_CALL(&Packet_Handle0x40); break;   // CPacketManager::Handle_UpdateTerrain_0x40 [low]
    case 0x41: PM_CALL(&Packet_Handle0x41); break;   // CPacketManager::Handle_UpdateArtwork_0x41 [low]
    case 0x42: PM_CALL(&Packet_Handle0x42); break;   // CPacketManager::Handle_UpdateAnimation_0x42 [low]
    case 0x43: PM_CALL(&Packet_Handle0x43); break;   // CPacketManager::Handle_UpdateHue_0x43 [low]
    case 0x44: PM_CALL(&Packet_Handle0x44); break;   // CPacketManager::Handle_Packet0x44 [low]
    case 0x45: PM_CALL(&Packet_Handle0x45); break;   // CPacketManager::Handle_VersionOK_0x45 [low]
    case 0x4e: PM_CALL(&Packet_Handle0x4E); break;   // CPacketManager::Handle_PersonalLightLevel_0x4E [medium]
    case 0x4f: PM_CALL(&Packet_GlobalLight); break;   // Packet_GlobalLight in packet_handlers_objects.cpp
    case 0x53: PM_CALL(&Packet_Handle0x53); break;   // CPacketManager::Handle_LoginError_0x53 [high]
    case 0x54: PM_CALL(&Packet_Handle0x54); break;   // CPacketManager::Handle_PlaySoundEffect_0x54 [high]
    case 0x55: PM_CALL(&Packet_Handle0x55); break;   // CPacketManager::Handle_LoginComplete_0x55 [medium]
    case 0x56: PM_CALL(&Packet_Handle0x56); break;   // CPacketManager::Handle_MapPlot_0x56 [medium]
    case 0x57: PM_CALL(&Packet_HandleRegionUpdate_0045b660); break;   // CPacketManager::HandlePacket(PDSTRUCT_UPD_REGIONS *) [high]
    case 0x5b: PM_CALL(&Packet_Handle0x5B); break;   // CPacketManager::Handle_Time_0x5B [high]
    case 0x5c: PM_CALL(&Packet_Handle_004577d0); break;   // Packet_Handle_004577d0 in packet_handlers_objects.cpp
    case 0x65: PM_CALL(&Packet_Handle0x65); break;   // CPacketManager::Handle_SetWeather_0x65 [medium]
    case 0x66: PM_CALL(&Packet_Handle0x66); break;   // CPacketManager::Handle_BookPages_0x66 [high]
    case 0x69: PM_CALL(&Packet_Handle0x69); break;   // CPacketManager::Handle_Packet0x69_NameList [low]
    case 0x6a: PM_CALL(&Packet_Handle0x6A); break;   // CPacketManager::Handle_Packet0x6A_OnlineStatus [high]
    case 0x6c: PM_CALL(&Packet_Handle0x6C); break;   // CPacketManager::Handle_TargetCursor_0x6C [high]
    case 0x6d: PM_CALL(&Packet_Handle0x6D); break;   // CPacketManager::Handle_PlayMusic_0x6D [high]
    case 0x6e: PM_CALL(&Packet_Handle0x6E); break;   // CPacketManager::Handle_MobileAnimation_0x6E [high]
    case 0x6f: PM_CALL(&Packet_Handle0x6F); break;   // CPacketManager::Handle_SecureTrading_0x6F [high]
    case 0x70: PM_CALL(&Packet_Handle0x70); break;   // CPacketManager::Handle_GraphicalEffect_0x70 [high]
    case 0x71: PM_CALL(&Packet_Handle0x71); break;   // CPacketManager::HandlePacket(PDSTRUCT_BBOARD *) [high]
    case 0x72: PM_CALL(&Packet_Handle0x72); break;   // CPacketManager::Handle_WarMode_0x72 [high]
    case 0x73: PM_CALL(&Stub_00421390); break;   // Stub_00421390 in bark_gump_ui.cpp
    case 0x74: PM_CALL(&Packet_Handle0x74); break;   // CPacketManager::Handle_OpenBuyWindow_0x74 [high]
    case 0x76: PM_CALL(&Packet_NewSubserver); break;   // CPacketManager::HandlePacket_NewSubserver_0x76 [medium]
    case 0x77: PM_CALL(&Packet_MobileMoving); break;   // CPacketManager::HandlePacket_MobileMoving_0x77 [high]
    case 0x78: PM_CALL(&Packet_Handle0x78); break;   // CPacketManager::HandlePacket_MobileIncoming_0x78 [high]
    case 0x7b: PM_CALL(&Packet_Handle0x7B); break;   // CPacketManager::Handle_Sequencing_0x7B [medium]
    case 0x7c: PM_CALL(&Packet_Handle0x7C); break;   // Handle_Packet_0x7C_Menu [medium]
    case 0x81: PM_CALL(&Packet_HandleServerListWithChars); break;   // CPacketManager::HandlePacket(PDSTRUCT_ACCT_LOGIN_OK *) [medium]
    case 0x82: PM_CALL(&Packet_Handle0x82); break;   // Handle_Packet_0x82_LoginDenied [high]
    case 0x85: PM_CALL(&Packet_Handle0x85); break;   // Handle_Packet_0x85_CharDeleteResult [high]
    case 0x86: PM_CALL(&Packet_Handle0x86); break;   // Handle_Packet_0x86_CharacterListUpdate [high]
    case 0x88: PM_CALL(&Packet_Handle0x88); break;   // Handle_Packet_0x88_OpenPaperdoll [medium]
    case 0x89: PM_CALL(&Packet_CorpseEquip); break;   // CPacketManager::HandlePacket_CorpseEquip_0x89 [high]
    case 0x8b: PM_CALL(&Packet_Handle0x8B); break;   // Handle_Packet_0x8B_TwoStringDialog [low]
    case 0x8c: PM_CALL(&Packet_HandleServerRelay); break;   // Handle_Packet_0x8C_ServerRelay [high]
    case 0x90: PM_CALL(&Packet_Handle0x90); break;   // CPacketManager::HandlePacket_MapDisplay_0x90 [high]
    case 0x92: PM_CALL(&Packet_Handle0x92); break;   // CPacketManager::Handle_Packet0x92 [low]
    case 0x93: PM_CALL(&Packet_Handle0x93); break;   // CPacketManager::Handle_BookHeader_0x93 [high]
    case 0x94: PM_CALL(&Packet_Handle0x94); break;   // CPacketManager::Handle_Packet0x94 [low]
    case 0x95: PM_CALL(&Packet_Handle0x95); break;   // Handle_Packet_0x95_DyeWindow [high]
    case 0x97: PM_CALL(&Walk_ForcedFromPacket); break;   // Walk_ForcedFromPacket in packet_builders_b25.cpp
    case 0x98: PM_CALL(&Packet_Handle0x98); break;   // Handle_Packet_0x98_AllNames [medium]
    case 0x99: PM_CALL(&Packet_Handle0x99); break;   // CPacketManager::Handle_BoatPlacement_0x99 [medium]
    case 0x9a: PM_CALL(&Packet_Handle0x9A); break;   // Handle_Packet_0x9A_ConsolePrompt [high]
    case 0x9e: PM_CALL(&Packet_Handle0x9E); break;   // CPacketManager::Handle_SellList_0x9E [high]
    case 0xa1: PM_CALL(&Packet_UpdateHealthBar); break;   // CPacketManager::Handle_UpdateHealthBar_0xA1 [high]
    case 0xa2: PM_CALL(&Packet_UpdateManaBar); break;   // CPacketManager::Handle_UpdateManaBar_0xA2 [high]
    case 0xa3: PM_CALL(&Packet_UpdateStaminaBar); break;   // CPacketManager::Handle_UpdateStaminaBar_0xA3 [high]
    case 0xa5: PM_CALL(&Packet_Handle0xA5); break;   // Handle_Packet_0xA5_OpenURL [high]
    case 0xa6: PM_CALL(&Packet_HandleTipOfTheDay); break;   // Handle_Packet_0xA6_TipOfTheDay [medium]
    case 0xa8: PM_CALL(&Packet_HandleServerList); break;   // Handle_Packet_0xA8_ServerList [high]
    case 0xa9: PM_CALL(&Packet_HandleCharacterList); break;   // Handle_Packet_0xA9_CharacterList [high]
    case 0xaa: PM_CALL(&Packet_Handle0xAA); break;   // Handle_Packet_0xAA_AttackCharacter [high]
    case 0xab: PM_CALL(&Packet_Handle0xAB); break;   // Handle_Packet_0xAB_TextEntryDialog [high]
    case 0xae: PM_CALL(&HandleUnicodeSpeechPacket); break;   // HandlePacket_UnicodeSpeech_0xAE [high]
    case 0xaf: PM_CALL(&Packet_Handle0xAF); break;   // Handle_Packet_0xAF_DeathAnimation [high]
    case 0xb0: PM_CALL(&Packet_Handle0xB0); break;   // Handle_Packet_0xB0_GumpDialog [high]
    default:
        break;
    }
    g_lastPacketId = pkt[0];
}
HOOK_METHOD(0x00477990, CClientSocket, HandlePacket);

// 0x00478610  size 0x53  send a built packet: a variable-length packet has its length field (+1) swapped to
// network order first; the byte count is the length field, or for a fixed-length packet the write cursor
// word at 0x00747978. ECX = connection (CClientSocket), one stack argument, RET 4.
void __thiscall CClientSocket::SendPacket(const uint8_t *packet)
{
    uint16_t variable = static_cast<uint16_t>(pkt::Flags(packet[0]) & 0x8000);
    uint32_t len;
    if (variable != 0)
        len = *reinterpret_cast<const uint16_t *>(packet + 1);
    else
        len = GLOBAL_AS(uint32_t, g_pktFixedCursor);
    len &= 0xffffu;
    if (variable != 0)
        *const_cast<uint16_t *>(reinterpret_cast<const uint16_t *>(packet + 1)) = htons(*reinterpret_cast<const uint16_t *>(packet + 1));   // 0x00498658
    AppendSend(packet, static_cast<int>(len));   // 0x00476c60
}
HOOK_METHOD(0x00478610, CClientSocket, SendPacket);
