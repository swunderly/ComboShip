#include "SilverRupeeState.h"
#include <cassert>
#include <set>
#include <string_view>
using namespace Rando::SilverRupees;
// Identities come from the production SilverRupeeData.inc; its symbols are compared as text.
struct ItemRow {std::string_view symbol;uint8_t slot;bool pouch;};
const ItemRow Items[]={
#define SR_ITEM(item,slot,pouch,setting,name) {#item,slot,pouch},
#include "SilverRupeeData.inc"
#undef SR_ITEM
};
struct CheckRow {std::string_view symbol,inf,name;uint16_t id;std::string_view item,quest;};
const CheckRow Checks[]={
#define SR_CHECK(check,inf,id,item,scene,quest,name) {#check,#inf,name,id,#item,#quest},
#include "SilverRupeeData.inc"
#undef SR_CHECK
};
static const ItemRow* FindItem(std::string_view symbol) {for(const auto& item:Items)if(item.symbol==symbol)return &item;return nullptr;}
static bool IsMq(const CheckRow& check) {return check.quest=="RCQUEST_MQ";}
// PhysicalId only computes the id; the actor keeps vanilla behaviour unless a check owns it.
static bool KnownId(std::optional<uint16_t> id) {for(const auto& check:Checks)if(id && check.id==*id)return true;return false;}
int main() {
    std::set<unsigned> ids;std::set<std::string_view> symbols,flags;
    size_t vanilla=0,mq=0;
    for(const auto& check:Checks) {
        assert(ids.insert(check.id).second);
        assert(symbols.insert(check.symbol).second && flags.insert(check.inf).second);
        const auto* item=FindItem(check.item);
        assert(item && !item->pouch);
        assert(bool(check.id&0x80)==IsMq(check));
        IsMq(check) ? ++mq : ++vanilla;
    }
    assert(std::size(Items)==44);
    assert(vanilla==80 && mq==74 && ids.size()==154);
    for(bool master:{false,true}) {
        for(size_t slot=0;slot<PuzzleCount;++slot) {
            auto puzzle=GetPuzzle(slot,master);ReceivedState received;
            std::set<unsigned> physicalCollected; // Foreign grants never touch physical ownership.
            auto first=Receive(received,slot,master,true);
            assert(first.count==puzzle.count && first.rupees==5*puzzle.count);
            assert(physicalCollected.empty());
            auto second=Receive(received,slot,master,true);
            assert(second.count==puzzle.count && second.rupees==first.rupees);
            assert(physicalCollected.empty());
            assert(received.counts[slot]==puzzle.count);
            for(size_t other=0;other<PuzzleCount;++other)if(other!=slot)assert(received.counts[other]==0);
            if(puzzle.count) {
                received={};auto partial=Receive(received,slot,master,false,puzzle.scene);
                assert(partial.count==1 && partial.rupees==5);
                auto finish=Receive(received,slot,master,true,puzzle.scene);
                assert(finish.count==puzzle.count && !finish.permanentSwitch);
                auto dormant=Receive(received,slot,master,false);
                assert(dormant.permanentSwitch==(puzzle.flag<0x20));
            }
        }
    }
    assert(MasterQuest[0].flag==0x25 && MasterQuest[2].flag==0x37);
    assert(MasterQuest[6].count==10 && MasterQuest[8].count==10);
    assert(MasterQuest[12].count==6 && MasterQuest[13].count==3);
    for(const auto& item:Items)assert(item.slot<PuzzleCount);
    auto slotFor=[](std::string_view symbol) {const auto* item=FindItem(symbol);return item?item->slot:uint8_t{255};};
    assert(slotFor("RG_RUPEE_SILVER_SPIRIT_ADULT")==slotFor("RG_RUPEE_SILVER_SPIRIT_SUN"));
    assert(slotFor("RG_RUPEE_SILVER_SPIRIT_LOBBY")==slotFor("RG_RUPEE_SILVER_SPIRIT_CHILD"));
    assert(slotFor("RG_RUPEE_SILVER_GANON_SHADOW")==slotFor("RG_RUPEE_SILVER_GANON_SPIRIT"));
    assert(slotFor("RG_RUPEE_SILVER_GANON_WATER")==slotFor("RG_RUPEE_SILVER_GANON_LIGHT"));
    struct Spawn {unsigned scene;int room;unsigned base;};
    const Spawn rooms[]={{1,0,0},{8,0,5},{6,0,0xa},{6,2,0xa},{6,8,0xf},{6,0x17,0xf},{6,0xd,0x14},
       {7,6,0x19},{7,0x10,0x1e},{7,9,0x28},{7,0xb,0x2d},{9,3,0x37},{9,5,0x3c},
       {11,2,0x41},{11,6,0x46},{11,9,0x4c},{13,0xc,0x51},{13,0x11,0x51},
       {13,3,0x56},{13,8,0x56},{13,0xe,0x5b},{13,6,0x60}};
    for(const auto& check:Checks) {
        bool matched=false;
        for(const auto& spawn:rooms) {
            const auto local=(check.id&0x7f)-spawn.base;
            if(local>9)continue;
            const auto actual=PhysicalId(static_cast<uint8_t>(spawn.scene),static_cast<uint8_t>(spawn.room),static_cast<uint8_t>(local),IsMq(check));
            if(actual && *actual==check.id)matched=true;
        }
        assert(matched);
    }
    assert(!PhysicalId(0xff,0,0,false));assert(!PhysicalId(6,99,0,false));
    assert(!KnownId(PhysicalId(13,6,0,true))); // MQ forest has no puzzle.
    assert(!KnownId(PhysicalId(1,0,0,false))); // Vanilla DC has no puzzle.
    assert(!KnownId(PhysicalId(8,0,0,true)));assert(!KnownId(PhysicalId(9,3,0,true)));
    assert(!PhysicalId(13,6,999,false));
}
