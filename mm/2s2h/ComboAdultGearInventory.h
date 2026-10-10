#pragma once
// Pinned kMmTrade2/kMmTrade3 equipment entries, item_add.c addItemTradeMm2/3.
// Native quest ownership and spells remain in their existing save fields.
namespace ComboAdultGearInventory {
inline int Group(unsigned item) {
    return item==ITEM_COMBO_BOOTS_IRON||item==ITEM_COMBO_TUNIC_GORON ? 2 :
           item==ITEM_COMBO_BOOTS_HOVER||item==ITEM_COMBO_TUNIC_ZORA ? 3 : 0;
}
inline unsigned Param(unsigned item) {
    return item==ITEM_COMBO_BOOTS_IRON||item==ITEM_COMBO_BOOTS_HOVER ? 1 : 2;
}
inline bool Grant(u8& extraBits,u8& slot,unsigned item) {
    if(!Group(item)||(extraBits&~6u))return false;
    if(slot==ITEM_NONE)slot=u8(item);
    extraBits|=u8(1u<<Param(item));
    return true;
}
inline u8 Next(u8 current,unsigned flags,const u8 (&table)[5]) {
    int bit=-1;for(int i=0;i<5;++i)if(current==table[i]){bit=i;break;}
    if(bit<0)return ITEM_NONE;
    const int initial=bit;
    for(;;){bit=(bit+1)%5;if((flags&(1u<<bit))||bit==initial)return table[bit];}
}
inline bool Has(u8 extraBits,unsigned item) {
    return Group(item)&&!(extraBits&~6u)&&(extraBits&(1u<<Param(item)));
}
}
