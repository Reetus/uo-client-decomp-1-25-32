#include "skill_gump.h"

#include "../runtime/ported.h"
#include "raw_field.h"
#include "shop_gump.h"

// 0x0040e280  size 0xe2
// Skill gump: decrease handler (negated steps -1/-2/-5/-10).
// 0x0046cfd0, 0x0046ce00 and 0x0046d280 are thiscalls on the shop gump at +0xb0 (ECX stays loaded across the walk).
// ? The asm pushes EAX with only AL (0x0046ce00) or AX (0x0046cfd0) loaded from the node, so the
//   upper bits of the first argument are the high bits of the node pointer. The callees presumably read
//   only the low byte / word; this port passes the same composed value.
void __thiscall CSkillGump::OnDecrease()
{
    if (disabled != 0)
        return;
    char *list = static_cast<char *>(parentList);
    char *node = RAWF(char *, list, 0xcc);   // list head
    if (node == nullptr)
        return;
    while (RAWF(int32_t, node, 4) != unk_08) {
        node = RAWF(char *, node, 0xc);   // next
        if (node == nullptr)
            break;
    }
    if (node == nullptr)
        return;

    if (unk_0c < 100)
        unk_0c++;

    int step;
    if (unk_0c < 10)
        step = -1;
    else if (unk_0c < 0x14)
        step = -2;
    else if (unk_0c < 0x32)
        step = -5;
    else
        step = -10;

    uintptr_t nodeBits = reinterpret_cast<uintptr_t>(node);
    if (lockFlag != 0) {
        uint32_t id = static_cast<uint32_t>((nodeBits & ~static_cast<uintptr_t>(0xffff)) | RAWF(uint16_t, node, 8));
        reinterpret_cast<CShopGump *>(parentList)->SellAddToCart(id, step);   // ECX = shop gump, still loaded from +0xb0
    } else {
        uint32_t id = static_cast<uint32_t>((nodeBits & ~static_cast<uintptr_t>(0xff)) | RAWF(uint8_t, node, 0));
        reinterpret_cast<CShopGump *>(parentList)->AddToCart(id, RAWF(int32_t, node, 4), step);
    }
    reinterpret_cast<CShopGump *>(parentList)->RedrawFrame();
}
HOOK_METHOD(0x0040e280, CSkillGump, OnDecrease);
