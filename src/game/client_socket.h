// CSocketBase / CClientSocket: the Winsock connection object (global at 0x0069e0ec,
// 0x20040 bytes, vtable 0x004a0830) and CNetInfo (WSAStartup wrapper, vtable 0x004a0834).
// Virtual calls go through the original vtables; slot 0 of each is the scalar deleting destructor.
#pragma once

#include <cstddef>
#include <cstdint>

#include "../runtime/ported.h"

// Base class: vtable 0x004a082c, socket handle at +4 (-1 = none).
class CSocketBase {
public:
    int32_t sock;     // +0x04  SOCKET, -1 when closed (the vptr is +0x00, the compiler's)

    inline CSocketBase();                                    // inlined into the derived constructor: sock = -1
    virtual ~CSocketBase();                           // 0x00476ab0 (slot 0 = the compiler's scalar deleting destructor, 0x00476a70)
    void Dtor() { this->~CSocketBase(); }
    void __thiscall Close();                          // 0x00476ad0
};

// Growable array of deferred packet pointers, the member at +0x20034 of CClientSocket (12 bytes). The original's
// constructor inlines the three stores and its unwind map has a state for it that calls 0x004777d0.
class CPacketQueue {
public:
    int32_t count;        // +0x00
    int32_t cap;          // +0x04
    uint8_t **items;      // +0x08

    CPacketQueue() { cap = 0; count = 0; items = nullptr; }
    ~CPacketQueue();      // 0x004777d0
};
static_assert(sizeof(CPacketQueue) == 0xc, "CPacketQueue size");

class CClientSocket : public CSocketBase {
public:
    uint8_t recvBuf[0x10000];   // +0x00008
    uint8_t sendBuf[0x10000];   // +0x10008
    int32_t recvLen;            // +0x20008
    int32_t sendLen;            // +0x2000c
    uint32_t key0;              // +0x20010  rolling XOR key words of the outbound stream
    uint32_t key1;              // +0x20014
    uint32_t key2;              // +0x20018
    uint32_t key3;              // +0x2001c
    int32_t encrypt;            // +0x20020  outbound XOR enabled
    int32_t unk_20024;          // +0x20024  zeroed by the constructor, never read here
    uint32_t localIp;           // +0x20028  ntohl of the first address of the local host
    int32_t queueing;           // +0x2002c  non-zero: dispatched packets stay in the buffer
    int32_t queuedBytes;        // +0x20030  bytes of packets already dispatched but not yet compacted away
    CPacketQueue pending;       // +0x20034  growable array of deferred packet pointers

    CClientSocket();                                        // 0x00476af0  size 0xba, new CClientSocket (operator new(0x20040))
    ~CClientSocket();                                       // 0x00476be0
    void Dtor() { this->~CClientSocket(); }
    void __thiscall AppendSend(const uint8_t *data, int len);   // 0x00476c60
    int __thiscall Connect(uint32_t ip, uint32_t port);     // 0x00476d30
    void __thiscall Pump();                                 // 0x004770a0
    void __thiscall ProcessReceived();                      // 0x004773e0
    void __thiscall QueuePacket(uint8_t *packet);           // 0x004777f0
    void __thiscall FlushQueuedPackets();                   // 0x004778c0
    void __thiscall HandlePacket(uint8_t *packet);          // 0x00477990
    void __thiscall SendPacket(const uint8_t *packet);      // 0x00478610
};

class CNetInfo {
public:
    uint8_t wsaData[0x190];   // +0x04  WSADATA (the compiler's vptr, table 0x004a0834, is at +0x00)

    CNetInfo();                                     // 0x004772a0  size 0x33, new CNetInfo (operator new(0x194))
    virtual ~CNetInfo();                            // WSACleanup; inline in client_socket.cpp, expanded in 0x004772e0
};

static_assert(offsetof(CSocketBase, sock) == 0x04, "sock");
static_assert(offsetof(CClientSocket, recvBuf) == 0x08, "recvBuf");
static_assert(offsetof(CClientSocket, sendBuf) == 0x10008, "sendBuf");
static_assert(offsetof(CClientSocket, recvLen) == 0x20008, "recvLen");
static_assert(offsetof(CClientSocket, sendLen) == 0x2000c, "sendLen");
static_assert(offsetof(CClientSocket, key0) == 0x20010, "key0");
static_assert(offsetof(CClientSocket, key3) == 0x2001c, "key3");
static_assert(offsetof(CClientSocket, encrypt) == 0x20020, "encrypt");
static_assert(offsetof(CClientSocket, localIp) == 0x20028, "localIp");
static_assert(offsetof(CClientSocket, queueing) == 0x2002c, "queueing");
static_assert(offsetof(CClientSocket, queuedBytes) == 0x20030, "queuedBytes");
static_assert(offsetof(CClientSocket, pending) == 0x20034, "pending");
static_assert(sizeof(CNetInfo) == 0x194, "CNetInfo size (operator new(0x194) in WinMain)");
static_assert(sizeof(CClientSocket) == 0x20040, "CClientSocket size (operator new(0x20040) at 0x00476ef0)");
