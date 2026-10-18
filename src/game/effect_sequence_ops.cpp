// CEffectSequence: constructor, destructor, append and advance (0x0043dfa0 .. 0x0043e730).
#include "effect_sequence.h"

#include <cstring>

#include "../runtime/ported.h"
#include "crt.h"
#include "vtables.h"

// 0x0043dfa0  size 0x14
CEffectSequence::CEffectSequence()
{
    current = nullptr;
    head = nullptr;
    repeat = 0;
}
HOOK_CTOR(0x0043dfa0, CEffectSequence, ());

// 0x0043dfc0  size 0x56
// Destructor (inlined into the scalar deleting destructor): frees the node list.
inline CEffectSequence::~CEffectSequence()
{
    if (head != nullptr) {
        SeqNode *nextNode;
        do {
            SeqNode *n = head;
            nextNode = n->next;
            if (n->data != nullptr)
                operator delete(n->data);
            operator delete(head);
            head = nextNode;
        } while (nextNode != nullptr);
    }
}
HOOK_VTABLE(0x0049f1b4, CEffectSequence);
HOOK_SDD(0x0043dfc0, CEffectSequence);   // slot 0

// 0x0043e020  size 0x78
void __thiscall CEffectSequence::Append(int type, const void *data, unsigned size)
{
    SeqNode *node = static_cast<SeqNode *>(::operator new(0xc));
    node->type = type;
    node->data = static_cast<uint8_t *>(::operator new(size));
    memcpy(node->data, data, size);
    node->next = nullptr;
    if (head == nullptr) {
        head = node;
        return;
    }
    SeqNode *tail = head;
    while (tail->next != nullptr)
        tail = tail->next;
    tail->next = node;
}
HOOK_METHOD(0x0043e020, CEffectSequence, Append);

// 0x0043e730  size 0x36
void __thiscall CEffectSequence::Advance()
{
    SeqNode *nextNode = current->next;
    if (nextNode != nullptr) {
        current = nextNode;
        Step(nextNode);
        return;
    }
    repeat = repeat - 1;
    if (repeat != 0) {
        current = head;
        Step(head);
        return;
    }
    if (this != nullptr)
        PORT_VSLOT1(this, 0x00, void, int)(1);
}
HOOK_METHOD(0x0043e730, CEffectSequence, Advance);
