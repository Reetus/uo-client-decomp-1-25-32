// Winsock connection object: CSocketBase / CClientSocket, CNetInfo and the Huffman decode tree
// (0x00476a70-0x004778c0). The SEH frames of the original constructors and destructors
// (try levels around the vtable stores) are real now: CClientSocket has its constructor and destructor.
//
// Winsock is reached through the thunks inside the original image so that the port shares the
// original's Winsock instance (0x00498616 ntohl, ...).
#include <winsock.h>
#include <windows.h>

#include <cstdint>
#include <cstring>

#include "../runtime/ported.h"
#include "client_socket.h"
#include "crt.h"
#include "connection_lost_ports.h"
#include "vtables.h"
#include "globals.h"


// The connection-lost handler (ported in connection_lost.cpp): __stdcall, one argument that is never read.
#define Socket_OnConnectionLost(a1) (ConnectionLostTick((int)(a1)))
// The call sites in Pump load ECX with the socket before the call (the handler does not read it).
#define Socket_OnConnectionLostThis(a1) PORT_THISCALL1(&ConnectionLostTick, void, this, int)((int)(a1))

#define g_selectTimeout GLOBAL_PTR(void, g_selectTimeout_storage)
#define g_huffCodesEnd (GEND(g_huffCodes) + 4)                // the loop below starts at the second word of the first pair

// The table at 0x004add84 is 8 bytes per opcode; the leading word is the length, bit 0x8000 = variable (length at +1).
#define SOCK_PktWord(op) (*reinterpret_cast<const uint16_t *>(GADDR(g_packetTable) + static_cast<uint32_t>(op) * 8u))
#define SOCK_PktVariable(op) ((*reinterpret_cast<const uint8_t *>(GADDR(g_packetTable) + 1u + static_cast<uint32_t>(op) * 8u) & 0x80) != 0)



// 0x00476ab0  size 0x20  CSocketBase destructor body
// 0x004777d0  size 0x11  destructor of the deferred-packet array (the pointer is not cleared)
CPacketQueue::~CPacketQueue()
{
    if (items != nullptr)
        operator delete(items);
}
HOOK_DTOR(0x004777d0, CPacketQueue);

inline CSocketBase::CSocketBase()
{
    sock = -1;
}

inline CSocketBase::~CSocketBase()
{
    if (sock != -1) {
        closesocket(sock);
        sock = -1;
    }
}
HOOK_DTOR(0x00476ab0, CSocketBase);
HOOK_VTABLE(0x004a082c, CSocketBase);
HOOK_SDD(0x00476a70, CSocketBase);   // slot 0

// 0x00476ad0  size 0x1a  close the socket
void __thiscall CSocketBase::Close()
{
    if (sock != -1) {
        closesocket(sock);
        sock = -1;
    }
}
HOOK_METHOD(0x00476ad0, CSocketBase, Close);

// 0x00477310  size 0xd0  build the Huffman decode tree from the code table at 0x004aee98 (once)
// Each table entry is {bit length, code}; the code is walked from its top bit, 0 = child at +4, 1 = child at +8.
// Node = {value (-1 for inner), child0, child1}, 0xc bytes. 0x101 entries: symbol 0x100 is the end marker.
void BuildHuffmanTree()
{
    if (g_huffmanRoot != nullptr)
        return;
    int32_t sym = 0;
    for (uint32_t *entry = &g_huffCodes[1]; static_cast<intptr_t>(reinterpret_cast<uintptr_t>(entry)) < static_cast<intptr_t>(g_huffCodesEnd); entry += 2) {
        uint32_t code = entry[0];
        int32_t nbits = static_cast<int32_t>(entry[-1]);
        int32_t count = nbits;
        int32_t **slot = reinterpret_cast<int32_t **>(&g_huffmanRoot);
        if (g_huffmanRoot == nullptr) {
            g_huffmanRoot = static_cast<int32_t *>(::operator new(0xc));
            g_huffmanRoot[2] = 0;
            g_huffmanRoot[1] = 0;
            g_huffmanRoot[0] = -1;
        }
        for (; count > 0; count--, nbits--) {
            int32_t *node = *slot;
            if ((code & (1u << (nbits - 1))) != 0)
                slot = reinterpret_cast<int32_t **>(node + 2);
            else
                slot = reinterpret_cast<int32_t **>(node + 1);
            if (*slot == nullptr) {
                *slot = static_cast<int32_t *>(::operator new(0xc));
                (*slot)[2] = 0;
                (*slot)[1] = 0;
                (*slot)[0] = -1;
            }
        }
        (*slot)[0] = sym;
        sym++;
    }
}
HOOK_FUNC(0x00477310, BuildHuffmanTree);

// 0x00476af0  size 0xba  constructor: clear the fields, look up the local host address, build the Huffman tree
CClientSocket::CClientSocket()
{
    char hostname[256];
    queuedBytes = 0;
    queueing = 0;
    sendLen = 0;
    recvLen = 0;
    unk_20024 = 0;
    gethostname(hostname, 0x100);
    hostent *he = gethostbyname(hostname);
    localIp = he != nullptr ? ntohl(**reinterpret_cast<uint32_t **>(he->h_addr_list)) : 0;
    BuildHuffmanTree();
}
HOOK_CTOR(0x00476af0, CClientSocket, ());

// 0x00476be0  size 0x7a  destructor: close, free the deferred-packet array, then the CSocketBase part
CClientSocket::~CClientSocket()
{
    if (sock != -1) {
        closesocket(sock);
        sock = -1;
    }
}
HOOK_DTOR(0x00476be0, CClientSocket);
HOOK_VTABLE(0x004a0830, CClientSocket);
HOOK_SDD(0x00476bb0, CClientSocket);   // slot 0


// 0x00476c60  size 0xcc  append len bytes to the send buffer (dropped if they do not fit in 0x10000);
// when the stream is keyed each byte is XORed with the low byte of key0 and the 64-bit key {key0, key1} steps.
void __thiscall CClientSocket::AppendSend(const uint8_t *data, int len)
{
    if (len + sendLen > 0x10000)
        return;
    if (encrypt == 0) {
        std::memcpy(sendBuf + sendLen, data, static_cast<size_t>(static_cast<uint32_t>(len)));
        sendLen += len;
        return;
    }
    if (len == 0)
        return;
    do {
        sendBuf[sendLen] = static_cast<uint8_t>(*data ^ static_cast<uint8_t>(key0));
        uint32_t k0 = key0;
        uint32_t k1 = key1;
        sendLen++;
        data++;
        key1 = ((k1 >> 1) | (k0 << 31)) ^ 0x389de58cu;
        key0 = ((k0 >> 1) | (k1 << 31)) ^ 0x026950c6u;
        len--;
    } while (len != 0);
}
HOOK_METHOD(0x00476c60, CClientSocket, AppendSend);

// 0x00476d30  size 0x1c0  connect to (ip, port): create the TCP socket, set TCP_NODELAY, connect, send the login seed,
// then make the socket non-blocking. Returns 1 on success, 0 on failure.
int __thiscall CClientSocket::Connect(uint32_t ip, uint32_t port)
{
    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        uint8_t zero[8];
    } sa;
    uint32_t netSeed;

    queuedBytes = 0;
    queueing = 0;
    sendLen = 0;
    recvLen = 0;
    sa.addr = htonl(ip);
    sa.port = htons(port);
    sock = socket(2, 1, 0);
    if (sock == -1) {
        OutputDebugStringA(g_004af6a4);   // "socket creation failed"
        return 0;
    }
    ip = 1;
    setsockopt(sock, 6, 1, reinterpret_cast<const char *>(&ip), 4);
    sa.family = 2;
    if (connect(sock, reinterpret_cast<const sockaddr *>(&sa), 0x10) != 0) {
        closesocket(sock);
        sock = -1;
        return 0;
    }
    if (g_loginByAddr == 0) {
        encrypt = 1;
        uint32_t seed = g_authKey;
        if (seed == 0) {
            seed = localIp;
            g_authKey = seed;
        }
        uint32_t nseed = ~seed;
        port = 0;
        key0 = ((nseed ^ 0x1357u) << 16) | ((seed ^ 0xffffaaaau) & 0xffffu);
        key1 = ((g_authKey ^ 0x43210000u) >> 16) | ((nseed ^ 0xabcdffffu) & 0xffff0000u);
        key2 = (((nseed ^ g_authKey) & 0x0f0f0f0fu) ^ g_authKey) ^ 0xa76e25eau;
        key3 = (((nseed ^ g_authKey) & 0x0f0f0f0fu) ^ nseed) ^ 0xf4bc319du;
        ioctlsocket(sock, static_cast<int32_t>(0x8004667e), reinterpret_cast<u_long *>(&port));
        netSeed = htonl(g_authKey);
        send(sock, reinterpret_cast<const char *>(&netSeed), 4, 0);
    } else {
        encrypt = 0;
    }
    port = 1;
    ioctlsocket(sock, static_cast<int32_t>(0x8004667e), reinterpret_cast<u_long *>(&port));
    return 1;
}
HOOK_METHOD(0x00476d30, CClientSocket, Connect);

// 0x004770a0  size 0x1f7  select on the socket, read everything pending, flush the send buffer, process the received bytes
// The three fd sets: read at ebp-0x30c, write at ebp-0x104, exception at ebp-0x208 (select's argument order).
void __thiscall CClientSocket::Pump()
{
    fd_set readSet;
    fd_set writeSet;
    fd_set exceptSet;

    int32_t s = sock;
    if (s == -1) {
        if (g_suppressFlag == 0)
            Socket_OnConnectionLost(g_004af724);   // "dunno, select with invalid socket"
        return;
    }
    writeSet.fd_count = 0;
    readSet.fd_array[0] = static_cast<SOCKET>(s);
    readSet.fd_count = 1;
    if (sendLen > 0) {
        writeSet.fd_array[0] = static_cast<SOCKET>(s);
        writeSet.fd_count = 1;
    }
    exceptSet.fd_count = 1;
    exceptSet.fd_array[0] = static_cast<SOCKET>(s);
    if (select(s + 1, &readSet, &writeSet, &exceptSet, static_cast<const timeval *>(g_selectTimeout)) == -1)
        return;
    if (__WSAFDIsSet(sock, &exceptSet) != 0) {
        if (sock != -1) {
            closesocket(sock);
            sock = -1;
        }
        Socket_OnConnectionLostThis(g_004af708);   // "exception state on socket"
        return;
    }
    if (__WSAFDIsSet(sock, &readSet) != 0) {
        int n;
        int space;
        do {
            space = 0x10000 - recvLen;
            if (space > 0) {
                n = recv(sock, reinterpret_cast<char *>(recvBuf + recvLen), space, 0);
                if (n == -1) {
                    if (sock != -1) {
                        closesocket(sock);
                        sock = -1;
                    }
                    Socket_OnConnectionLostThis(g_004af6e4);   // "Exiting due to closed connection."
                    return;
                }
                recvLen += n;
            } else {
                n = 0;
            }
        } while (n == space && space > 0);
    }
    if (__WSAFDIsSet(sock, &writeSet) != 0) {
        sendBuf[sendLen] = 0;
        int sent = send(sock, reinterpret_cast<const char *>(sendBuf), sendLen, 0);
        if (sent == -1) {
            if (sock != -1) {
                closesocket(sock);
                sock = -1;
            }
            Socket_OnConnectionLostThis(g_004af6cc);   // "socket error in send"
            return;
        }
        if (sent > 0) {
            for (int i = sent; i < sendLen; i++)
                sendBuf[i - sent] = sendBuf[i];
        }
        sendLen -= sent;
    }
    if (recvLen > 0)
        ProcessReceived();   // 0x004773e0
}
HOOK_METHOD(0x004770a0, CClientSocket, Pump);

// 0x004772a0  size 0x33  CNetInfo constructor: WSAStartup(0x101, &wsaData)
CNetInfo::CNetInfo()
{
    if (WSAStartup(0x101, reinterpret_cast<WSADATA *>(wsaData)) != 0)
        MessageBoxA(nullptr, g_004af748, g_004af774, 0);   // "SockLib Error"
}
HOOK_CTOR(0x004772a0, CNetInfo, ());

// 0x004772e0  size 0x27  CNetInfo scalar deleting destructor (the destructor, WSACleanup, is expanded in it)
inline CNetInfo::~CNetInfo()
{
    WSACleanup();
}
HOOK_VTABLE(0x004a0834, CNetInfo);
HOOK_SDD(0x004772e0, CNetInfo);   // slot 0

// Packet length: the table word, or the (host order) 16-bit field at +1 when bit 0x8000 is set.
static inline uint32_t SOCK_PacketLength(const uint8_t *pkt)
{
    uint16_t w = SOCK_PktWord(pkt[0]);
    if (w & 0x8000)
        w = *reinterpret_cast<const uint16_t *>(pkt + 1);
    return w & 0xffffu;
}

// 0x004773e0  size 0x3ee  split the receive buffer (or the Huffman-decoded stream) into packets and dispatch them.
// A variable-length packet has its length field swapped to host order while it is examined; an incomplete one is
// swapped back and left for the next call. Packets are dispatched through HandlePacket (0x00477990).
// ? the Huffman bit position in 0x00757994 is the position after the last complete symbol; each call restarts at the
// root from the stored position.
void __thiscall CClientSocket::ProcessReceived()
{
    if (g_useHuffman == 0) {
        int32_t pos = queuedBytes;
        if (recvLen < pos + 1)
            return;
        if (recvLen <= pos + 1)
            return;
        for (;;) {
            uint8_t *pkt = &recvBuf[pos];
            if (SOCK_PktVariable(pkt[0]))
                *reinterpret_cast<uint16_t *>(pkt + 1) = ntohs(*reinterpret_cast<uint16_t *>(pkt + 1));   // 0x0049862e
            uint32_t len = SOCK_PacketLength(pkt);
            int32_t cur = queuedBytes;
            if (static_cast<int32_t>(len) + cur > 0x10000) {
                pkt -= cur;
                queueing = 0;
                FlushQueuedPackets();   // 0x004778c0
            }
            cur = queuedBytes;
            if (recvLen < static_cast<int32_t>(len) + cur) {
                if (SOCK_PktVariable(pkt[0]))
                    *reinterpret_cast<uint16_t *>(pkt + 1) = htons(*reinterpret_cast<uint16_t *>(pkt + 1));   // 0x00498658
                return;
            }
            if (queueing != 0) {
                HandlePacket(pkt);   // 0x00477990
            } else {
                HandlePacket(pkt);   // 0x00477990
                for (int32_t i = static_cast<int32_t>(len); i < recvLen; i++)
                    recvBuf[i - static_cast<int32_t>(len)] = recvBuf[i];
                recvLen -= static_cast<int32_t>(len);
            }
            pos = queuedBytes;
            if (!(recvLen > pos + 1))
                return;
        }
    }

    // Huffman-compressed stream
    int32_t *node = g_huffmanRoot;
    int32_t bitPos = g_huffBitPos;
    int32_t outLen;
    uint32_t curByte = recvBuf[0];
    bool full = false;   // the output buffer filled up: skip straight to the compaction step

    if (bitPos < 8) {
        outLen = g_huffOutLen;
        int32_t shift = 7 - bitPos;
        int32_t i = bitPos;
        for (;;) {
            node = reinterpret_cast<int32_t *>((curByte & (1u << (shift & 31))) != 0 ? node[2] : node[1]);
            int32_t value = node[0];
            if (value != -1) {
                if (value == 0x100) {
                    node = g_huffmanRoot;
                    g_huffBitPos = 8;
                    break;
                }
                outLen++;
                g_huffBitPos = i + 1;
                g_huffOut[outLen - 1] = static_cast<uint8_t>(value);
                node = g_huffmanRoot;
                g_huffOutLen = outLen;
                if (outLen == 0x10000) {
                    full = true;
                    break;
                }
            }
            i++;
            shift--;
            if (!(i < 8))
                break;
        }
    } else {
        outLen = g_huffOutLen;
    }

    if (!full) {
        int32_t bi = 1;
        if (recvLen > bi) {
            int32_t base = 0x10;
            do {
                uint32_t cb = recvBuf[bi];
                int32_t k = 0;
                int32_t sh = 7;
                for (;;) {
                    node = reinterpret_cast<int32_t *>((cb & (1u << (sh & 31))) != 0 ? node[2] : node[1]);
                    int32_t value = node[0];
                    if (value != -1) {
                        if (value == 0x100) {
                            node = g_huffmanRoot;
                            g_huffBitPos = base;
                            break;
                        }
                        g_huffBitPos = k + base - 7;
                        outLen++;
                        g_huffOut[outLen - 1] = static_cast<uint8_t>(value);
                        node = g_huffmanRoot;
                        g_huffOutLen = outLen;
                        if (outLen == 0x10000) {
                            full = true;
                            break;
                        }
                    }
                    k++;
                    sh--;
                    if (!(sh > -1))
                        break;
                }
                if (full)
                    break;
                bi++;
                base += 8;
            } while (bi < recvLen);
        }
    }

    // drop the input bytes that were consumed whole
    if (g_huffBitPos >= 8) {
        int32_t used = g_huffBitPos >> 3;
        if (used < recvLen) {
            for (int32_t i = used; i < recvLen; i++)
                recvBuf[i - used] = recvBuf[i];
        }
        recvLen -= used;
        g_huffBitPos = g_huffBitPos & 7;
        outLen = g_huffOutLen;
    }

    // split the decoded stream into packets
    int32_t pos = queuedBytes;
    if (outLen < pos + 1)
        return;
    if (outLen <= pos + 1)
        return;
    for (;;) {
        uint8_t *pkt = &g_huffOut[pos];
        if (SOCK_PktVariable(pkt[0])) {
            *reinterpret_cast<uint16_t *>(pkt + 1) = ntohs(*reinterpret_cast<uint16_t *>(pkt + 1));   // 0x0049862e
            outLen = g_huffOutLen;
        }
        uint32_t len = SOCK_PacketLength(pkt);
        int32_t cur = queuedBytes;
        if (static_cast<int32_t>(len) + cur > 0x10000) {
            pkt -= cur;
            queueing = 0;
            FlushQueuedPackets();   // 0x004778c0
            outLen = g_huffOutLen;
        }
        cur = queuedBytes;
        if (outLen < static_cast<int32_t>(len) + cur) {
            if (SOCK_PktVariable(pkt[0]))
                *reinterpret_cast<uint16_t *>(pkt + 1) = htons(*reinterpret_cast<uint16_t *>(pkt + 1));   // 0x00498658
            return;
        }
        if (queueing != 0) {
            HandlePacket(pkt);   // 0x00477990
            outLen = g_huffOutLen;
        } else {
            HandlePacket(pkt);   // 0x00477990
            outLen = g_huffOutLen;
            for (int32_t i = static_cast<int32_t>(len); i < outLen; i++)
                g_huffOut[i - static_cast<int32_t>(len)] = g_huffOut[i];
            outLen -= static_cast<int32_t>(len);
            g_huffOutLen = outLen;
        }
        pos = queuedBytes;
        if (!(outLen > pos + 1))
            return;
    }
}
HOOK_METHOD(0x004773e0, CClientSocket, ProcessReceived);

// 0x004777f0  size 0xc9  defer a received packet: remember its pointer, count its bytes, switch queueing on
void __thiscall CClientSocket::QueuePacket(uint8_t *packet)
{
    if (!(pending.count < pending.cap)) {
        uint8_t **grown = static_cast<uint8_t **>(::operator new(static_cast<size_t>(pending.cap) * 4 + 0x40));
        for (int32_t i = 0; i < pending.count; i++)
            grown[i] = pending.items[i];
        operator delete(pending.items);
        pending.items = grown;
        pending.cap += 0x10;
    }
    uint32_t len = SOCK_PacketLength(packet);
    queueing = 1;
    pending.items[pending.count] = packet;
    pending.count++;
    queuedBytes += static_cast<int32_t>(len);
}
HOOK_METHOD(0x004777f0, CClientSocket, QueuePacket);

// 0x004778c0  size 0xc9  dispatch every deferred packet, then slide the rest of the stream down over them
void __thiscall CClientSocket::FlushQueuedPackets()
{
    int32_t i = 0;
    if (pending.count > 0) {
        do {
            HandlePacket(pending.items[i]);   // 0x00477990
            i++;
        } while (i < pending.count);
    }
    pending.count = 0;
    if (g_useHuffman != 0) {
        int32_t from = queuedBytes;
        int32_t end = g_huffOutLen;
        if (from < end) {
            do {
                g_huffOut[from - queuedBytes] = g_huffOut[from];
                from++;
            } while (from < end);
        }
        g_huffOutLen = end - queuedBytes;
        queuedBytes = 0;
        return;
    }
    int32_t from = queuedBytes;
    if (from < recvLen) {
        do {
            recvBuf[from - queuedBytes] = recvBuf[from];
            from++;
        } while (from < recvLen);
    }
    recvLen -= queuedBytes;
    queuedBytes = 0;
}
HOOK_METHOD(0x004778c0, CClientSocket, FlushQueuedPackets);
