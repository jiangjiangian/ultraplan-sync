#ifndef VENDOR_LOADER_H_
#define VENDOR_LOADER_H_
#include "VendorConfig.h"
#include <string>
#include <vector>

namespace nccu::vendor {

// Runtime parser for the Interlude market stall lineup — the price-table
// sibling of dialog::LoadChapter. interlude_market.md §10 is the single
// source of truth: editing the .md changes the in-game market with no
// rebuild and no code review (exactly like chapter dialog). This replaces
// the abandoned "hand-transcribe 10 literals" approach (that was the
// codegen pattern the project deliberately retired).
//
// Section convention (sibling to "## NPC：<name>"):
//   ## 攤位：<攤名>
//   > 攤主：<人>
//   > 商品：<itemId> = <price>      (0..n; itemId is the canonical code id)
//   > 機制：<buy|donate|sell|game|flavor>
//   > tier：<N>                     (optional, design grouping)
//   > karma：<±N>                   (optional, applied on a successful buy)
//   > stock：<N>                    (optional, -1/absent = unlimited)
//   ### greeting        - "…"      (-> greetingLines)
//   ### onPurchase      - "…"      (success block; onDonate/onAccept are
//                                    synonyms, first non-empty wins)
//   ### onLeave         - "…"      (-> onLeave)
// Variant-qualified blocks (e.g. "### onPurchase（陷阱傘殘骸）") are parsed
// but only the first success block is kept for Phase 2 (S5b-4/Tier-3 may
// refine). Lines use the same `- "…"` (ASCII or CJK quote) form as dialog.
//
// On a file that cannot be opened, returns an empty vector (no throw),
// mirroring LoadChapter's degrade-to-empty contract.
std::vector<VendorConfig> LoadInterludeVendors(const std::string& path);

}  // namespace nccu::vendor

#endif // VENDOR_LOADER_H_
