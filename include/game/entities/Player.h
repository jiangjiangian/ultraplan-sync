#ifndef PLAYER_H_
#define PLAYER_H_
#include "game/entities/Character.h"
#include "engine/math/Color.h"
#include "engine/render/Texture.h"
#include "engine/math/Vec2.h"
#include <optional>
#include <string>
#include <unordered_map>

/**
 * @file Player.h
 * @brief 玩家角色：扮演可更新／可繪製／有生命值，持有業力、金錢、雨量、旗標與消耗品背包。
 */

/**
 * @brief 玩家「當前」持有的雨傘種類，作為 Player 上的純資料。
 *
 * 刻意與決定結局的持久旗標（Flag_HasTrueUmbrella／Flag_TookCursedUmbrella／
 * Flag_BoughtUglyUmbrella）分離。那些旗標整場遊戲常駐（EndingGate 讀它們挑 A/B/C，且後兩
 * 者從不清除），故無法回答「此刻背包裡是哪把傘」：Ch4 進場 SetHasUmbrella(false) 之後，舊
 * 的以旗標為鍵的背包仍會顯示玩家已不再持有的詛咒／醜傘列。此列舉追蹤即時握傘，使背包反映你
 * 「實際」握的傘（並在失去的瞬間消失），同時結局旗標不受影響。None 表示空手。Ch2 圖書館管理員
 * 的「借傘」刻意「不」是這裡的一種：它純由 Flag_LibrarianUmbrella 驅動、遮蔽靠 SetHasUmbrella，
 * 使換回的真傘（True）能與借傘並存——兩者各自一列背包，互不覆蓋。
 */
enum class HeldUmbrella { None, True, Cursed, Ugly, Victim, Fragile,
                          ProfessorTrap };

/**
 * @brief 由玩家操控的角色，繼承自可移動的 Character。
 *
 * ISP 角色：IUpdatable + IDrawable + IMortal。原本的 Interact(Player*) 為空殼（玩家不回應其
 * 他玩家），故捨棄該角色——容器也從未經它呼叫（E 互動掃描以 ForEachActiveExcept 略過玩家）。
 * Update（輸入＋動畫）與 Render（sprite）皆為實作而保留。IMortal 為戰鬥擴充骨架
 * （hp／TakeDamage／IsDead）：玩家具生命值，使存活模式可對其造成傷害；hp_ 不被序列化（自動
 * 播放僅輸出 x/y/karma/money/rain），故加入它不改變輸出。Player 為 final，故 WithRoles 以
 * Player 自身為鍵。
 */
class Player final : public WithRoles<Player, Character>,
                     public IUpdatable, public IDrawable, public IMortal {
public:
    /** @brief 以世界座標建構玩家。@param[in] position 世界座標位置（像素）。 */
    explicit Player(nccu::engine::math::Vec2 position);

    /**
     * @brief 每幀更新：處理輸入並推進行走動畫。
     * @param[in] deltaTime 本幀經過的秒數。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 繪出玩家 sprite（含人設色調）。
     * @param[in,out] renderer 注入的渲染介面。
     */
    void Render(nccu::engine::render::IRenderer& renderer) const override;

    /**
     * @brief 讀取方向鍵並驅動移動。
     * @param[in] deltaTime 本幀經過的秒數。
     */
    void HandleInput(float deltaTime);

    // ── IMortal（戰鬥擴充骨架）──────────────────
    /// 起始／上限生命值。取整數 100 以便戰鬥以近似百分比的級距調傷；尚未接上任何現行玩法（目前無敵人造成傷害）。
    static constexpr int kMaxHp = 100;
    /**
     * @brief 扣除生命值。
     * @param[in] amount 扣血量；非正值忽略（治療為另一回事），下限夾制在 0。
     */
    void TakeDamage(int amount) noexcept override {
        if (amount <= 0) return;
        hp_ = (amount >= hp_) ? 0 : hp_ - amount;
    }
    /** @brief 是否已死亡。@return hp 歸零時回傳 true。 */
    [[nodiscard]] bool IsDead() const noexcept override { return hp_ <= 0; }
    /** @brief 取得當前生命值。@return 目前 hp。 */
    [[nodiscard]] int  Hp()     const noexcept override { return hp_; }

    // 變更器皆回傳 *this 以利串接：
    //   player.AddKarma(10).AddMoney(50).SetHasUmbrella(true);
    /** @brief 增減業力（夾制於 [-100, 100]）並發出 KarmaChanged。@param[in] delta 業力變化量。@return *this。 */
    Player& AddKarma(int delta);
    /** @brief 扣減業力的薄包裝（建議優先用 AddKarma）。@param[in] amount 扣減量。@return *this。 */
    Player& decreaseKarma(int amount);
    /** @brief 將雨量歸零。@return *this。 */
    Player& resetRainMeter() noexcept;

    /**
     * @brief 累加詛咒污點計數。
     * @return *this。
     *
     * 詛咒污點機制（取代先前拾取時一次性的 -30）：每次認領 CursedUmbrella 即遞增污點計數，
     * 而每次章節過場（Ch2/Ch3/Ch4 進場——與重置握傘相同的鉤子）扣 karma -= 5 × cursedTaint_。
     * 兩次拾取為每章 -10、三次為每章 -15，使詛咒疊加讀來像一條玩家能跨越整場感受到的滑動道德
     * 代價，而非一次性的當頭重擊。從不清除（道德污點永久）；taint=0 時靜默，故非詛咒流程的輸出不變。
     */
    Player& IncCursedTaint() noexcept { ++cursedTaint_; return *this; }
    /** @brief 套用每章的詛咒污點衰減（扣 5 × 污點業力）。@return *this。 */
    Player& ApplyCursedTaintDecay();
    /** @brief 取得詛咒污點計數。@return 目前污點數。 */
    [[nodiscard]] int GetCursedTaint() const noexcept { return cursedTaint_; }

    /**
     * @brief 設定是否持有雨傘（遮蔽開關）。
     * @param[in] v true 開啟遮蔽；false 表示「傘不見了」。
     * @return *this。
     *
     * SetHasUmbrella(false) 是規範化的「傘不見了」呼叫（Ch4 進場傘再度失蹤；每章的「傘又掉了」
     * 重置）。失去傘時也必須清空握傘欄位，使背包的雨傘列消失——否則會殘留陳舊列（握傘種類才是
     * 背包的真實來源，而非持久的結局旗標）。此處設為 true「不」挑選種類（明確挑選請用
     * SetHeldUmbrella）；此多載為只切換遮蔽的呼叫者（測試、雨量迴圈）保留舊的 bool 介面。
     */
    Player& SetHasUmbrella(bool v) noexcept {
        hasUmbrella_ = v;
        if (!v) heldUmbrella_ = HeldUmbrella::None;
        return *this;
    }
    /**
     * @brief 設定「當前」握持的雨傘種類。
     * @param[in] kind 握傘種類；None 同時清除遮蔽。
     * @return *this。
     *
     * 非 None 的種類意味玩家處於遮蔽下（hasUmbrella_ = true），使自動雨量衰減
     * （ApplyRainSheltered）生效。這是「唯一」推導背包雨傘列之處——絕不觸碰結局旗標（呼叫者於
     * 結果真正確定時另行設立）。本質上冪等（重設相同種類為無動作的指派），故再認領／再對話永不疊加。
     */
    Player& SetHeldUmbrella(HeldUmbrella kind) noexcept {
        heldUmbrella_ = kind;
        hasUmbrella_  = (kind != HeldUmbrella::None);
        return *this;
    }
    /** @brief 取得當前握持的雨傘種類。@return 目前的 HeldUmbrella。 */
    [[nodiscard]] HeldUmbrella HeldUmbrellaKind() const noexcept {
        return heldUmbrella_;
    }
    /// 金錢軟上限（循環經濟）：在 3 段賺->花循環下，不設上限會讓徹底探索的玩家輕易輾平每個市集。300 是依 3 循環結構重新調校的數值。
    static constexpr int kMoneySoftCap = 300;
    /**
     * @brief 增加金錢（夾制於軟上限）。
     * @param[in] amount 增加金額。
     * @return *this。
     *
     * 僅夾制上限、不夾下限（負的 amount 仍會扣減）；DeductMoney 不受影響。
     */
    Player& AddMoney(int amount) noexcept {
        money_ += amount;
        if (money_ > kMoneySoftCap) money_ = kMoneySoftCap;
        return *this;
    }
    /** @brief 設立具名旗標。@param[in] name 旗標名稱。@return *this。 */
    Player& SetFlag(const std::string& name)   { flags_[name] = true; return *this; }
    /** @brief 清除具名旗標。@param[in] name 旗標名稱。@return *this。 */
    Player& ClearFlag(const std::string& name) { flags_.erase(name);  return *this; }

    /**
     * @brief 在背包中為某 itemId 計數加一。
     * @param[in] itemId 消耗品識別字。
     * @return *this。
     *
     * 純計數的消耗品背包：玩家對每個 itemId 持有一個計數，而非 GameObject 實例——刻意精簡
     * （無持續時間／數量物件；背包 UI 僅渲染這些計數）。Vendor::TryBuy 增加；某章「用掉」消耗品
     * 時呼叫 ConsumeOne；換章時清空（消耗品於其章節內用完）。
     */
    Player& AddConsumable(const std::string& itemId) {
        ++consumables_[itemId];
        return *this;
    }
    /** @brief 查詢某消耗品的持有數。@param[in] itemId 消耗品識別字。@return 持有數量（未持有為 0）。 */
    [[nodiscard]] int ConsumableCount(const std::string& itemId) const {
        auto it = consumables_.find(itemId);
        return it == consumables_.end() ? 0 : it->second;
    }
    /** @brief 若持有則消耗一個。@param[in] itemId 消耗品識別字。@return 成功消耗回傳 true；未持有則為無動作並回傳 false。 */
    bool ConsumeOne(const std::string& itemId) {
        auto it = consumables_.find(itemId);
        if (it == consumables_.end() || it->second <= 0) return false;
        if (--it->second == 0) consumables_.erase(it);
        return true;
    }
    /**
     * @brief 清空整個消耗品背包。
     * @return *this。
     *
     * 玩家重回市集時由 GameController 清空，使某章購入的消耗品無法越過市集邊界囤積到下一章。
     */
    Player& ClearConsumables() noexcept {
        consumables_.clear();
        return *this;
    }
    /** @brief 取得整份消耗品背包的唯讀檢視（供背包 UI 渲染）。@return 對 itemId->數量映射的常參。 */
    [[nodiscard]] const std::unordered_map<std::string, int>&
    Consumables() const noexcept { return consumables_; }

    /**
     * @brief 載入 Pipoya 96×128 sprite 圖集（3 行走影格 × 4 方向，每格 32×32）。
     * @param[in] path 資源相對路徑；替換先前載入的任何圖集。
     */
    void LoadSprite(const std::string& path);

    /**
     * @brief 設定所選人設的繪製色彩調變。
     * @param[in] t 色調（白＝不調色，為預設值）。
     * @return *this。
     *
     * 純資料：Render() 將其傳給 IRenderer::DrawSprite，使共用同一張 Pipoya 底圖的五個人設無須
     * 任何新 sprite 二進位即讀來有別。非玩法狀態——純屬外觀，與其他一切一同在新 World 重置（重新
     * 開始會建立新的 Player）。
     */
    Player& SetTint(nccu::engine::math::Color t) noexcept { tint_ = t; return *this; }
    /** @brief 取得當前繪製色調。@return 目前的色調。 */
    [[nodiscard]] nccu::engine::math::Color GetTint() const noexcept { return tint_; }

    /** @brief 取得業力。@return 目前業力值。 */
    [[nodiscard]] int   GetKarma()      const noexcept { return karma_; }
    /** @brief 取得雨量。@return 目前雨量值。 */
    [[nodiscard]] float GetRainMeter()  const noexcept { return rainMeter_; }
    /** @brief 是否持有雨傘。@return 持有回傳 true。 */
    [[nodiscard]] bool  HasUmbrella()   const noexcept { return hasUmbrella_; }
    /** @brief 取得金錢。@return 目前金錢值。 */
    [[nodiscard]] int   GetMoney()      const noexcept { return money_; }
    /** @brief 查詢是否持有具名旗標。@param[in] name 旗標名稱。@return 已設立回傳 true。 */
    [[nodiscard]] bool  HasFlag(const std::string& name) const {
        auto it = flags_.find(name);
        return it != flags_.end() && it->second;
    }

    /**
     * @brief 扣款。
     * @param[in] amount 扣款金額。
     * @return 金額大於現有金錢則回傳 false（無副作用）；否則扣款並回傳 true。
     */
    [[nodiscard]] bool DeductMoney(int amount) noexcept;

    /**
     * @brief 套用無傘淋雨：每秒 +5 點，夾制於 [0,100]。
     * @param[in] dt     經過的秒數。
     * @param[in] lethal true（預設）時，雨量滿格即把玩家送回正門（重置雨量＋ShowMessage）；false 僅抑制該傳送（供單元測試／停用）。
     * @return *this。
     *
     * 存活迴圈為活躍機制——GameController 在「戶外且無傘」時累積此值、在遮蔽時以 DrainRain 消減，
     * 故滿格是「管理曝露失敗」而非隱藏的一次性計時。
     */
    Player& ApplyRain(float dt, bool lethal = true);

    /**
     * @brief 完全遮蔽（建築內）時的雨量回復：每秒 -10 點，夾制於 [0,100]，從不傳送。
     * @param[in] dt 經過的秒數。
     * @return *this。
     */
    Player& DrainRain(float dt) noexcept;

    /**
     * @brief 固定量的雨量減免（為「量」而非速率），夾制於 [0,100]，從不傳送。
     * @param[in] amount 減免的雨量點數。
     * @return *this。
     *
     * 於背包「使用」消耗品時套用（防水噴霧 -35／暖暖包 -25／提神飲料 -15／小吃 -15），使食物與
     * 裝備在使用時於業力／任務效果之外另行買回曝露。雨傘的雨量減免仍走「自動」的持傘路徑
     * （ApplyRainSheltered），維持不變——此處是離散道具路徑。
     */
    Player& DrainRainBy(float amount) noexcept;

    /**
     * @brief 持傘但仍曝露於雨中的部分累積：每秒 +1.5 點（約曝露速率的 30%），夾制於 [0,100]。
     * @param[in] dt     經過的秒數。
     * @param[in] lethal true（預設）時雨量滿格仍會送回正門；false 抑制該傳送。
     * @return *this。
     *
     * 雨壓必須存在於「每一章」而非僅 Ch1。山下大雨中的傘應「減緩」浸濕而非阻止（chapter2.md：
     * 持傘仍以較低速率累積），故傘換來的是「時間」而非免疫，玩家仍須不時躲進建築完全回復
     * （DrainRain）。GameController 每幀依情境呼叫：建築內 -> DrainRain（完全回復）；戶外持傘
     * -> ApplyRainSheltered（緩慢累積）；戶外無傘 -> ApplyRain（完全累積）。本函式刻意「不」理會
     * hasUmbrella_（此正是持傘卻仍曝露的情形）；曝露速率的 ApplyRain 維持其自身的持傘無動作不變。
     */
    Player& ApplyRainSheltered(float dt, bool lethal = true);

private:
    void RespawnAtGate();

    float rainMeter_;                                ///< 雨量值 [0,100]
    int karma_;                                      ///< 業力值 [-100,100]
    bool hasUmbrella_;                               ///< 是否持有雨傘（遮蔽旗標）
    HeldUmbrella heldUmbrella_{HeldUmbrella::None};  ///< 背包雨傘列的真實來源
    int money_;                                      ///< 金錢
    // 詛咒拾取計數，驅動 ApplyCursedTaintDecay 的每章業力衰減。類內初始化，故現有 Player 建構子
    // 無須變動；不被 SetHasUmbrella／換章重置清除（道德污點是詛咒中整場永久的那一半，與
    // Flag_TookCursedUmbrella 成對）。
    int cursedTaint_{0};
    // 戰鬥生命值。不被序列化，故不進入輸出——目前無敵人傷害玩家，整場維持 kMaxHp。類內初始化，
    // 故現有 Player 建構子無須變動。
    int hp_{kMaxHp};
    std::unordered_map<std::string, bool> flags_;        ///< 具名旗標表
    std::unordered_map<std::string, int>  consumables_;  ///< 消耗品計數背包（itemId -> 數量）

    std::optional<nccu::engine::render::Texture> sprite_;  ///< 玩家 sprite 圖集
    nccu::engine::math::Color tint_{255, 255, 255, 255};  ///< 人設色彩調變
    nccu::engine::math::Vec2 lastFacing_{0.0f, 1.0f};  ///< 最近朝向（起始朝下）
    float animTimer_{0.0f};                   ///< 動畫計時器
    int   animStep_{0};                       ///< 行走影格步進（0..3 -> 欄 1,0,1,2）
};

#endif // PLAYER_H_
