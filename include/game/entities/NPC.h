#ifndef N_P_C_H_
#define N_P_C_H_
#include "game/entities/Character.h"
#include "game/world/CollisionMask.h"
#include "game/state/SemesterState.h"
#include "engine/render/Texture.h"
#include "engine/math/Rect.h"
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file NPC.h
 * @brief 非玩家角色：可定點、漫步或繞圈跑，並循環對話台詞；亦為 Vendor 的基底。
 */

/**
 * @brief 地圖上的非玩家角色，可定點對話、隨機漫步或繞圈奔跑。
 *
 * ISP 角色：三者俱全（IUpdatable + IDrawable + IInteractable），每個實作皆為真——Update
 * 驅動漫步／繞圈跑的移動、Render 繪 sprite、Interact 循環對話台詞。Vendor（唯一的 NPC 子
 * 類別）共用此角色集（僅新增 TryBuy 並覆寫 IsVendor），故 WithRoles 以本中介層為鍵
 * （Derived = NPC），static_cast<NPC*> 對 Vendor 亦合法。
 */
class NPC : public WithRoles<NPC, Character>,
            public IUpdatable, public IDrawable, public IInteractable {
public:
    /**
     * @brief 建構 NPC。
     * @param[in] position     世界座標位置（像素）。
     * @param[in] dialogLines  對話台詞清單。
     * @param[in] isQuestGiver  是否為任務給予者，預設 false。
     * @param[in] npcId         對話查找用的身分字串，預設為空。
     */
    NPC(nccu::engine::math::Vec2 position,
        std::vector<std::string> dialogLines,
        bool isQuestGiver = false,
        std::string_view npcId = {});

    /**
     * @brief 推進此 NPC 的移動與動畫（漫步／繞圈跑，定點者不動）。
     * @param[in] deltaTime 本幀經過的秒數。
     */
    void Update(float deltaTime) override;
    /**
     * @brief 繪出 NPC sprite（定點者為待機影格，移動者播放行走循環）。
     * @param[in,out] renderer 注入的渲染介面。
     */
    void Render(nccu::engine::render::IRenderer& renderer) const override;
    /**
     * @brief 互動：發出當前台詞並推進到下一句（循環）。
     * @param[in] initiator 互動發起者（玩家）。
     */
    void Interact(Player* initiator) override;

    /**
     * @brief 對話查找鍵。
     * @return 原型 NPC 的 npcId；Vendor／路人為空字串（改走 Interact() 退路）。
     *
     * GameController 以此組出對應 (npcId, SemesterState) 的開場。覆寫 GameObject::NpcId，
     * 使分派維持虛擬而非 dynamic_cast。
     */
    [[nodiscard]] std::string_view NpcId() const noexcept override;

    /**
     * @brief 此 NPC 是否阻擋移動。
     * @return 定點原型 NPC 回傳 true；漫步者與繞圈跑者為裝飾性，回傳 false。
     *
     * 定點原型 NPC 是玩家會撞開的實牆；環境漫步學生為裝飾，絕不可擋住玩家（擠不過去的人
     * 群令人惱火）。
     */
    [[nodiscard]] bool BlocksMovement() const noexcept override {
        return !wander_ && !circular_;   // 漫步者與人群跑者皆為裝飾性
    }

    /**
     * @brief 將此 NPC 轉為環境路人：以 speed 隨機漫步，每 1-3 秒重選一次朝向。
     * @param[in] speed 漫步速度（像素／秒）。
     * @param[in] seed  per-NPC PRNG 種子，使人群不至於齊步行進。
     * @return *this 以利串接。
     */
    NPC& EnableWander(float speed, unsigned seed) noexcept;

    /**
     * @brief 將此 NPC 轉為裝飾性的校慶繞圈跑者。
     * @param[in] center       圓心。
     * @param[in] radius       半徑。
     * @param[in] angularSpeed 角速度（弧度／秒）。
     * @param[in] startAngle   起始角度。
     * @return *this 以利串接。
     *
     * 以 angularSpeed 自 startAngle 繞 center 以 radius 旋轉，附行走動畫且不阻擋（玩家跑同
     * 一條跑道）。操場跑道由 5 名速度各異的跑者構成。
     */
    NPC& EnableCircularRun(nccu::engine::math::Vec2 center, float radius,
                           float angularSpeed, float startAngle) noexcept;

    /**
     * @brief 設定漫步用的地形遮罩，使 NPC 自行避開建築、河流與貼圖道具。
     * @param[in] mask 世界地形碰撞遮罩；其生命週期須長於本 NPC（World 同時擁有兩者，建構後固定）。
     */
    void SetWanderMask(const nccu::CollisionMask& mask) noexcept {
        wanderMask_ = &mask;
    }

    /**
     * @brief 載入 Pipoya 96×128 圖集（3 行走欄 × 4 朝向列，與 Player 同一張圖）。
     * @param[in] path 資源相對路徑。
     *
     * 定點原型 NPC 僅繪製第 0 列（朝下）的待機欄；漫步／校慶跑者則依朝向播放完整四向行走循環。
     */
    void LoadSprite(const std::string& path);

    /**
     * @brief 設定是否繪製「整張」貼圖（而非 32×32 的 Pipoya cell）。
     * @param[in] v true 表示繪整張，供美術為單張靜態圖的攤販（如自動販賣機）使用。
     */
    void SetStaticSprite(bool v) noexcept { staticSprite_ = v; }

    /** @brief 此 NPC 是否為任務給予者。@return 是則回傳 true。 */
    [[nodiscard]] bool   IsQuestGiver()     const noexcept override { return isQuestGiver_; }
    /** @brief 取得當前台詞索引。@return 目前台詞的索引。 */
    [[nodiscard]] size_t CurrentLineIndex() const noexcept { return currentLineIndex_; }
    /** @brief 取得台詞總數。@return 台詞數量。 */
    [[nodiscard]] size_t DialogLineCount()  const noexcept { return dialogLines_.size(); }
    /** @brief 取得當前台詞文字。@return 目前台詞的字串常參（無台詞時為空字串）。 */
    [[nodiscard]] const std::string& CurrentLineText() const;

    /**
     * @brief 替換對話台詞並重置當前索引。
     * @param[in] lines 新的台詞清單。
     * @return *this 以利串接（如 npc.SetDialogLines({...}).LoadSprite(...)）。
     *
     * 章節狀態改變時由狀態機把新台詞推入 NPC。
     */
    NPC& SetDialogLines(std::vector<std::string> lines);

    /**
     * @brief 依執行期章節內容替換對話，對應給定的 (npcId, state, subState)。
     * @param[in] npcId    對話查找身分字串。
     * @param[in] state    學期狀態。
     * @param[in] subState 子狀態，預設 0。
     * @return *this 以利串接；無匹配則清空對話。
     */
    NPC& LoadDialog(std::string_view npcId, nccu::SemesterState state,
                    int subState = 0);

    /** @brief 查詢對話台詞。@return 指向台詞向量的指標（覆寫 GameObject::DialogLines）。 */
    [[nodiscard]] const std::vector<std::string>*
        DialogLines() const noexcept override { return &dialogLines_; }

    /// @brief Render 本幀會繪製之 Pipoya cell 的 {欄, 列}（測試檢視用）。
    struct RenderCell { int col; int row; };
    /**
     * @brief 取得本幀 Render 會繪製的 Pipoya cell。
     * @return 本幀應繪製的 {欄, 列}。
     *
     * 此為「動畫 vs 待機」決策與共用 WalkCycle 數學的整合點。獨立暴露的原因：Render 的貼圖
     * 繪製受 GL 閘控（無頭測試沒有有效 Texture，Render 會提早回傳備援矩形而觸不到 cell 選擇）。
     * 純讀取已模擬完成的狀態，與 SceneRouter 的檢視取值器同理。
     */
    [[nodiscard]] RenderCell CurrentRenderCell() const noexcept;

private:
    std::vector<std::string> dialogLines_;
    size_t                   currentLineIndex_;
    bool                     isQuestGiver_;
    std::string              npcId_;

    std::optional<nccu::engine::render::Texture> sprite_;
    bool                              staticSprite_ = false;

    bool                                       wander_;
    float                                      retargetTimer_;
    nccu::engine::math::Vec2                             wanderDir_;
    std::uint32_t                              rng_;
    const nccu::CollisionMask*                 wanderMask_;

    // 校慶繞圈跑者（EnableCircularRun）：固定圓形跑道＋行走影格動畫，使跑者讀來像在奔跑而非滑行。
    bool                                       circular_ = false;
    nccu::engine::math::Vec2                             circleCenter_{};      ///< 圓心
    float                                      circleRadius_ = 0.0f;  ///< 半徑
    float                                      circleAngle_  = 0.0f;  ///< 當前角度
    float                                      circleSpeed_  = 0.0f;  ///< 角速度（弧度／秒）
    // 行走循環動畫狀態，由校慶跑者與環境漫步者共用，使兩者讀來像在行走而非滑行——與 Player
    // （Player.cpp）的慣用法相同：移動時 animStep_ 循環三個行走欄、靜止時停在待機欄；facing_
    // 決定 Pipoya 列。moving_ 記錄此幀 NPC 是否實際位移（暫停或撞牆時為 false），使 Render 在
    // 靜止時顯示待機姿勢。以下皆不進入序列化輸出（純渲染選擇，MVC 乾淨）。
    float                                      animTimer_    = 0.0f;  ///< 動畫計時器
    int                                        animStep_     = 0;     ///< 行走影格步進（0..3）
    bool                                       moving_       = false; ///< 此幀是否實際位移
    nccu::engine::math::Vec2                             facing_{0.0f, 1.0f};  ///< 朝向（決定 Pipoya 列）
};

#endif // N_P_C_H_
