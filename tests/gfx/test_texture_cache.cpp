#include "doctest/doctest.h"
#include "engine/render/Texture.h"
#include <string>

/**
 * @file test_texture_cache.cpp
 * @brief 驗證載入一次即快取的貼圖快取：重複載入命中快取、缺檔為乾淨的 no-op、不同路徑各新增一筆。
 */

// 注意：此處不要 `using namespace nccu::gfx;`——raylib.h（由 gfx/Texture.h 引入）
// 宣告了一個全域 C 的 `Texture` 結構，未限定的 `Texture` 會與
// nccu::engine::render::Texture 產生歧義。一律完整限定。
namespace ng = nccu::engine::render;

// 在 ctest 下這些測試以無 GL context 執行，raylib 的 ::LoadTexture 對任何路徑
// 都回傳 Texture2D{0}（影像資料為 NULL）。這正好等同於缺檔／空資源路徑，故這些
// 測試在不需要任何真實資產的情況下，釘住快取「同一性」與缺檔 no-op 契約：
//   * 對同一檔案第二次 Load(path) 不會讓快取變大（命中而非重新載入），且回傳與
//     第一次相同的 texture id；
//   * 缺檔仍是乾淨的 no-op（id==0、IsValid()==false），且其非擁有視圖不會重複釋放；
//   * 不同路徑各新增恰好一筆快取項目。
// 使用獨特的合成路徑，讓這些測試不受其他測試載入的影響。

// 對同一路徑重複 Load 為命中（不重新載入）。
TEST_CASE("Texture 快取：對同一路徑重複 Load 為命中（不重新載入）") {
    const std::string p = "test::__cache_probe_alpha.png";

    const std::size_t before = ng::TextureCacheSize();
    ng::Texture a = ng::Texture::Load(p);          // 未命中 -> 新增一筆
    const std::size_t afterFirst = ng::TextureCacheSize();
    CHECK(afterFirst == before + 1);

    ng::Texture b = ng::Texture::Load(p);          // 命中 -> 不新增
    CHECK(ng::TextureCacheSize() == afterFirst);

    // 同一個快取的 texture id 交給兩個呼叫端（此處皆為 0，因無 GL；重點是兩者
    // 別名到同一筆快取項目）。
    CHECK(a.Raw().id == b.Raw().id);
    CHECK(a.Width()  == b.Width());
    CHECK(a.Height() == b.Height());
}

// 缺檔為乾淨的 no-op（契約不變）。
TEST_CASE("Texture 快取：缺檔為乾淨的 no-op（契約不變）") {
    // 無 GL／無此檔 -> 無效貼圖，與未加快取前完全相同。
    ng::Texture t = ng::Texture::Load("test::__definitely_absent_zzz.png");
    CHECK_FALSE(t.IsValid());
    CHECK(t.Raw().id == 0);
    // 對同一缺檔路徑取得第二個 handle 仍無效，且離開作用域時不得崩潰：快取中的
    // 擁有者是非擁有的 ({0})，兩個 handle 也都是非擁有視圖——故兩者的解構都不會
    // 呼叫 ::UnloadTexture。
    ng::Texture t2 = ng::Texture::Load("test::__definitely_absent_zzz.png");
    CHECK_FALSE(t2.IsValid());
}

// 不同路徑各新增恰好一筆。
TEST_CASE("Texture 快取：不同路徑各新增恰好一筆") {
    const std::size_t before = ng::TextureCacheSize();
    ng::Texture a = ng::Texture::Load("test::__cache_probe_beta.png");
    ng::Texture b = ng::Texture::Load("test::__cache_probe_gamma.png");
    CHECK(ng::TextureCacheSize() == before + 2);
    // 再次載入任一個仍是命中（數量不變）。
    ng::Texture a2 = ng::Texture::Load("test::__cache_probe_beta.png");
    ng::Texture b2 = ng::Texture::Load("test::__cache_probe_gamma.png");
    CHECK(ng::TextureCacheSize() == before + 2);
}

// PreloadTexture 預熱快取但不回傳 handle。
TEST_CASE("Texture 快取：PreloadTexture 預熱快取但不回傳 handle") {
    const std::string p = "test::__cache_preload_delta.png";
    const std::size_t before = ng::TextureCacheSize();
    ng::PreloadTexture(p);                 // 預熱
    CHECK(ng::TextureCacheSize() == before + 1);
    // 之後 Load 一個已預熱的路徑為命中（已在快取中）。
    ng::Texture t = ng::Texture::Load(p);
    CHECK(ng::TextureCacheSize() == before + 1);
    CHECK_FALSE(t.IsValid());              // 仍是缺檔 no-op
}
