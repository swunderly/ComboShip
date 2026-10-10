#pragma once
#include <cstdint>
// C ABI: nativeGi is an opaque owning-engine GetItemId, never interpreted by the finder.
// kind0 rejects; kind1 is pinned finder Recovery Heart fallback; kind2 is genuine owning stock.
struct ComboRenewableStock {
    int32_t kind = 0, nativeGi = 0, buyable = 0, fastBuy = 0;
    char name[192] = {}, drawName[192] = {};
};
using FnScrubStockQuery = int (*)(int, const char*, ComboRenewableStock*);
using FnScrubStockGrant = int (*)(int, int);
static_assert(sizeof(ComboRenewableStock) == 400);
