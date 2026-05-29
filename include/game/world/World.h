#ifndef WORLD_H_
#define WORLD_H_
#include "engine/core/GameObject.h"
#include "engine/events/HudSlot.h"
#include "game/world/HudTiming.h"   // kHudTtl + kHudFade
#include "game/entities/Player.h"
#include "game/state/SemesterState.h"
#include "game/state/SemesterStateMachine.h"
#include "game/world/BuildingTracker.h"
#include "game/world/CollisionMask.h"
#include "game/world/WorldOptions.h"  // Plan P2 step 4: ctor-injected accessibility flags
#include "game/dialog/DialogState.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * @file World.h
 * @brief 遊戲世界模型：持有全部 GameObject、學期狀態機、建築追蹤器與地形遮罩，
 *        提供章節生成、延遲清除（Sweep）與純資料的 UI 狀態。
 */

namespace nccu {

/**
 * @brief 遊戲的資料模型——擁有世界中的一切，但不涉渲染與輸入。
 *
 * 持有每個 GameObject、學期狀態機、建築追蹤器與地形碰撞遮罩。View 以 const 讀取它、
 * GameController 改動它。player_ 是對 front 物件的非擁有快取，當玩家於幀末清除被移除
 * 時一併清掉。雖然承載若干 UI 狀態旗標（背包／選單／說明等），但這些皆為純資料：沒有
 * raylib、沒有輸入處理；由 View 反映、由 GameController 凍結模擬。不可複製：它是唯一
 * 的世界。
 */
class World {
public:
    /**
     * @brief 建構世界：生成玩家與初始章節名冊，載入地形遮罩。
     * @param playerSpritePath 玩家 sprite 的貼圖路徑。
     * @param loadSprites      是否載入 sprite；正式執行傳預設 true，無頭單元測試
     *                         （無 GL 環境）傳 false 以略過會導致 SIGSEGV 的 GPU 上傳。
     * @param opts             無障礙選項（減少動畫／擴大目標），由組裝根注入；預設空
     *                         結構（兩旗標皆 false）對應環境變數未設時的行為。
     */
    explicit World(const std::string& playerSpritePath,
                   bool loadSprites = true,
                   WorldOptions opts = {});

    World(const World&)            = delete;
    World& operator=(const World&) = delete;

    using ObjectList = std::vector<std::unique_ptr<GameObject>>;

    /// @brief 取得世界物件容器（front 永遠是玩家）。
    [[nodiscard]] ObjectList&       Objects()       noexcept { return objects_; }
    /// @brief 取得世界物件容器（唯讀）。
    [[nodiscard]] const ObjectList& Objects() const noexcept { return objects_; }

    /// @brief 取得玩家非擁有快取指標（被清除後為 nullptr）。
    [[nodiscard]] Player*       GetPlayer()       noexcept { return player_; }
    /// @brief 取得玩家非擁有快取指標（唯讀）。
    [[nodiscard]] const Player* GetPlayer() const noexcept { return player_; }
    /// @brief 清掉玩家快取指標（於擁有它的 unique_ptr 被銷毀前呼叫，避免懸空）。
    void                        ClearPlayer()     noexcept { player_ = nullptr; }

    /// @brief 取得學期狀態機。
    [[nodiscard]] SemesterStateMachine&       Semester()       noexcept { return semester_; }
    /// @brief 取得學期狀態機（唯讀）。
    [[nodiscard]] const SemesterStateMachine& Semester() const noexcept { return semester_; }
    /// @brief 取得建築追蹤器。
    [[nodiscard]] BuildingTracker&            Tracker()        noexcept { return tracker_; }

    /**
     * @brief 推進操場校慶繞圈進度，由控制器每幀呼叫一次。
     *
     * 讀取玩家位置，累計繞跑道中心掃過的角度，繞滿一圈後設立 Flag_SportsLapDone。
     * 自我設限於第三章且尚未完成，其餘情形為廉價的無操作。
     */
    void UpdateSportsLap() noexcept;
    /// @brief 繞圈完成比例 [0,1]，供跑道環與 HUD 環渲染。
    [[nodiscard]] float SportsLapProgress() const noexcept;
    /// @brief 是否應繪製繞圈環（位於第三章且尚未完成）。
    [[nodiscard]] bool  SportsLapActive() const noexcept;

    /**
     * @brief 第二章散落筆記的延遲生成，由控制器每幀呼叫一次。
     * @return 在實際生成的那一幀回傳 true（供測試判定）。
     *
     * 三張筆記不在進入章節時生成，而是在玩家喚醒學霸（Flag_Bookworm，他開口索取的時
     * 刻）之後才出現。自我設限於第二章／已喚醒／尚未生成，每次造訪第二章僅生成一次
     * （ch2NotesSpawned_ 守衛）。筆記與進場生成同樣納入名冊追蹤，未撿取者於下次狀態
     * 變更時一併被 Sweep。
     */
    bool MaybeSpawnChapter2Notes();

    /**
     * @brief 第一章苦主雨傘的「選擇後揭示」延遲生成，由控制器每幀呼叫一次。
     * @return 在實際生成的那一幀回傳 true（供測試判定）。
     *
     * 苦主的透明傘不在進入章節時生成，而是在玩家與西裝學長對質並做出選擇
     * （Flag_SuitSeniorChoiceMade，即學長揭示他掉傘處）之後才出現。設立旗標前它根本
     * 不存在於世界，玩家無法在學長關卡前先撿走，藉此硬性鎖定「苦主→學長→傘→苦主」主
     * 線。自我設限於第一章／選擇旗標／尚未生成，每次造訪第一章僅生成一次
     * （ch1VictimUmbrellaSpawned_）；納入名冊追蹤，未完成離開時被 Sweep。
     */
    bool MaybeSpawnChapter1VictimUmbrella();

    /**
     * @brief 第三章真傘的「線索後揭示」延遲生成，由控制器每幀呼叫一次。
     * @return 在實際生成的那一幀回傳 true（供測試判定）。
     *
     * 真傘不在進入章節時生成，而是在 C 系學姊揭示其位置（Flag_KnowsUmbrellaLoc）之後
     * 才出現。生成於 kChapter3UmbrellaPos——體育館左側，使其不再被建築遮擋。自我設限
     * 於第三章／線索旗標／尚未生成，每次造訪第三章僅生成一次（ch3UmbrellaSpawned_）；
     * 納入名冊追蹤，未完成離開時被 Sweep。
     */
    bool MaybeSpawnChapter3Umbrella();

    /**
     * @brief 幕間管理員之傘歸還點的延遲生成，由控制器每幀呼叫一次。
     * @return 在實際生成的那一幀回傳 true（供測試判定）。
     *
     * 一個小型歸還點 NPC（kNpcLibrarianReturn）出現在中正圖書館前，且僅當下列全部成
     * 立：位於第二章往第三章的市集（InterludeReturnTo() == Chapter3_SportsDay）、玩家
     * 仍持有管理員借出的傘（Flag_LibrarianUmbrella 且 HeldUmbrella::Loaner）、且尚未歸
     * 還（Flag_LibrarianUmbrellaReturned）。每次造訪幕間僅生成一次
     * （interludeReturnSpawned_）；納入名冊追蹤，下次狀態變更時被 Sweep。即使不歸還，
     * 借出的傘也會在進入第三章時自動清除——歸還純粹是責任感 +10 的可選路徑。
     */
    bool MaybeSpawnInterludeLibrarianReturn();

    /// @brief 取得對話狀態。
    [[nodiscard]] DialogState&       Dialog()       noexcept { return dialog_; }
    /// @brief 取得對話狀態（唯讀）。
    [[nodiscard]] const DialogState& Dialog() const noexcept { return dialog_; }

    /// @brief 取得目前所在建築名稱（供 HUD 顯示）。
    [[nodiscard]] std::string&       CurrentBuildingName()       noexcept { return currentBuildingName_; }
    /// @brief 取得目前所在建築名稱（唯讀）。
    [[nodiscard]] const std::string& CurrentBuildingName() const noexcept { return currentBuildingName_; }

    /// @brief 取得地形碰撞遮罩（唯讀），供物理解算取用。
    [[nodiscard]] const CollisionMask& TerrainMask() const noexcept {
        return terrainMask_;
    }

    /// @brief 背包覆蓋層是否開啟。
    ///
    /// World 上的純 UI 狀態：View 據此反映、GameController 在開啟時凍結模擬（與對話框
    /// 同一模型）；實際數量由 Player 持有。關閉背包會把游標重置到頂端，使下次開啟確定
    /// 性地停在金幣列。
    [[nodiscard]] bool InventoryOpen() const noexcept { return inventoryOpen_; }
    /// @brief 設定背包開關；關閉時把游標重置回頂端。
    /// @param v 是否開啟。
    void SetInventoryOpen(bool v) noexcept {
        inventoryOpen_ = v;
        if (!v) inventoryCursor_ = 0;
    }
    /// @brief 取得目前高亮的背包列索引。
    ///
    /// GameController 於背包開啟時以 ↑／↓ 移動、並在 E／Enter 時讀取以使用所選消耗品；
    /// View 讀取以繪製游標與說明面板。純 UI 狀態。列數會隨道具被使用而減少，故游標由
    /// 取用端夾鉗（控制器讀取前、View 繪製前各自夾鉗），而非以固定上界儲存。
    [[nodiscard]] int  InventoryCursor() const noexcept { return inventoryCursor_; }
    /// @brief 設定高亮的背包列索引。@param v 新索引。
    void SetInventoryCursor(int v) noexcept { inventoryCursor_ = v; }

    /// @brief 應用層動作意圖：由暫停選單／結局選單發出，交給最外層迴圈執行。
    ///
    /// 重新開始／離開的意圖透過 PendingAppAction 上呈給 main.cpp 的外層迴圈——那是唯一
    /// 能安全完成整個 World 拆除＋重建的地方，事件匯流排訂閱者的生命週期得以受控；
    /// GameController 絕不自我拆除。
    enum class AppAction { None, Restart, Quit };
    /// @brief 遊戲內暫停選單是否開啟（右上角入口，以 Esc／M 開啟）。
    ///
    /// World 上的純 UI 狀態，與 InventoryOpen 同慣例：View 渲染、GameController 於開啟
    /// 時凍結模擬。選單項：0 繼續、1 說明、2 減少動畫、3 擴大目標、4 重新開始、5 離開。
    [[nodiscard]] bool MenuOpen() const noexcept { return menuOpen_; }
    /// @brief 設定暫停選單開關；關閉（繼續）時清掉游標、說明閂鎖與其頁碼。
    /// @param v 是否開啟。
    void SetMenuOpen(bool v) noexcept {
        menuOpen_ = v;
        // 繼續時重置：清掉游標、說明閂鎖與其頁碼。
        if (!v) { menuCursor_ = 0; helpOpen_ = false; helpPage_ = 0; }
    }
    /// @brief 取得暫停選單目前游標索引。
    [[nodiscard]] int  MenuCursor() const noexcept { return menuCursor_; }
    /// @brief 暫停選單項目數：0 繼續、1 說明、2 減少動畫、3 擴大目標、4 重新開始、5 離開。
    ///
    /// 「說明」開啟原地的說明覆蓋層（HelpOpen），而非發出 AppAction。第 2／3 列是非破
    /// 壞性的無障礙切換，刻意夾在「說明」與「重新開始」之間，使破壞性項目（重新開始／
    /// 離開）離游標起始位置最遠。
    static constexpr int kMenuItemCount = 6;
    /// @brief 以環狀方式移動暫停選單游標。@param delta 位移量（可為負）。
    void MoveMenuCursor(int delta) noexcept {
        menuCursor_ = (menuCursor_ + delta + kMenuItemCount) %
                      kMenuItemCount;
    }

    /// @brief 遊戲內「說明」（玩法）覆蓋層是否開啟。
    ///
    /// World 上的純 UI 狀態，與 MenuOpen 同慣例：View 渲染、GameController 在它（與選
    /// 單）開啟時維持模擬凍結。由暫停選單的「說明」項開啟，M／E／Enter 關回選單。
    [[nodiscard]] bool HelpOpen() const noexcept { return helpOpen_; }
    /// @brief 設定說明覆蓋層開關；開啟時把頁碼重置回第一頁。
    /// @param v 是否開啟。
    void SetHelpOpen(bool v) noexcept {
        helpOpen_ = v;
        if (v) helpPage_ = 0;   // 每次開啟都從第一頁開始
    }

    /// @brief 取得說明覆蓋層目前頁碼索引。
    ///
    /// 說明覆蓋層採分頁（內容超過單一面板）。純 UI 狀態，與 MenuCursor／InventoryCursor
    /// 同慣例：View 讀取以繪製對應頁與「第 N／M 頁」指示，GameController 於說明開啟時以
    /// ←／→ 翻頁。頁數定義於 UI 層（GameHelpPages.h kGameHelpPageCount），由控制器據此
    /// 環繞索引（World 不引入 UI 標頭）。於每次 SetHelpOpen(true)／SetMenuOpen(false)
    /// 時重置為 0，使重新開啟總是從第一頁開始。
    [[nodiscard]] int  HelpPage() const noexcept { return helpPage_; }
    /// @brief 設定說明覆蓋層頁碼索引。@param v 新頁碼。
    void SetHelpPage(int v) noexcept { helpPage_ = v; }

    /// @brief 取得「減少動畫」無障礙偏好。
    ///
    /// 預設 false；由建構子（環境變數開啟時）或未來的暫停選單 UI 設為 true。View／
    /// MessageView 經由純輔助函式讀取，使幕間標記掃動、結局卡片淡入與提示淡出在開啟時
    /// 塌縮為瞬間變化。純資料。
    [[nodiscard]] bool ReducedMotion() const noexcept { return reducedMotion_; }
    /// @brief 設定「減少動畫」偏好。@param v 是否開啟。
    void SetReducedMotion(bool v) noexcept { reducedMotion_ = v; }

    /// @brief 取得「擴大目標」無障礙偏好。
    ///
    /// 預設 false；由建構子（環境變數開啟時）或未來的暫停選單 UI 設為 true。
    /// GameController::Update 與 ScriptInput 讀取後，將玩家 E 探測的互動範圍由每側 8 px
    /// 加寬為 16 px（有效對話判定框由 40x40 增為 56x56）。移動碰撞體刻意維持不變：玩家
    /// 仍無法穿過 NPC，只有對話可達範圍變大。純資料。
    [[nodiscard]] bool LargeTargets() const noexcept { return largeTargets_; }
    /// @brief 設定「擴大目標」偏好。@param v 是否開啟。
    void SetLargeTargets(bool v) noexcept { largeTargets_ = v; }
    /// @brief 取得待處理的應用層動作意圖（由最外層迴圈消費）。
    [[nodiscard]] AppAction PendingAppAction() const noexcept {
        return pendingAppAction_;
    }
    /// @brief 登記一個待處理的應用層動作意圖。@param a 動作。
    void RequestAppAction(AppAction a) noexcept { pendingAppAction_ = a; }
    /// @brief 清除待處理的應用層動作意圖。
    void ClearAppAction() noexcept { pendingAppAction_ = AppAction::None; }

    /// @brief 結局畫面底部選單項目數：0 回首頁、1 重新開始、2 結束。
    static constexpr int kEndingMenuItemCount = 3;
    /// @brief 取得結局畫面選單游標索引。
    ///
    /// World 上的純 UI 狀態，與 MenuCursor 同慣例：EndingView 讀取以繪製高亮選項，
    /// GameController 於結局畫面期間以 ←／→ 移動，再把所選索引對映為 World::AppAction。
    /// 只要學期處於結局狀態（IsEndingState），此選單即隱含「開啟」，無另設開關旗標。
    [[nodiscard]] int EndingMenuCursor() const noexcept { return endingMenuCursor_; }
    /// @brief 設定結局畫面選單游標索引。@param v 新索引。
    void SetEndingMenuCursor(int v) noexcept { endingMenuCursor_ = v; }
    /// @brief 以環狀方式移動結局畫面選單游標。@param delta 位移量（可為負）。
    void MoveEndingMenuCursor(int delta) noexcept {
        endingMenuCursor_ = (endingMenuCursor_ + delta + kEndingMenuItemCount) %
                            kEndingMenuItemCount;
    }

    /**
     * @brief 設定指定通道的暫時性畫面提示（由 EventType::ShowMessage 驅動）。
     * @param slot 提示通道。
     * @param text 提示文字（移入）。
     *
     * HUD 提示分為兩個獨立通道：HudSlot::Top 承載章節／結局的重大進度提示，
     * HudSlot::Bottom 承載其餘一切（拾取／業力／抵達提示／攤販／離場準備）。兩通道分
     * 開存在，是為了讓「章節清關提示」與緊接其後在 Bottom 通道發布的抵達提示能並存而不
     * 被覆蓋——單一通道時，章節提示曾僅存活一幀即被覆寫。兩通道各自以 TickHud(dt) 計
     * 齡，View 各自套用淡出。純資料：此處無 raylib、無計時來源。
     */
    void SetHudMessage(HudSlot slot, std::string text) {
        if (slot == HudSlot::Top) {
            topHudMessage_ = std::move(text);
            topHudAge_     = 0.0f;
        } else {
            bottomHudMessage_ = std::move(text);
            bottomHudAge_     = 0.0f;
        }
    }
    /// @brief 相容多載：預設投往 Bottom 通道。@param text 提示文字（移入）。
    void SetHudMessage(std::string text) {
        SetHudMessage(HudSlot::Bottom, std::move(text));
    }
    /// @brief 為兩個 HUD 通道計齡（驅動淡出與過期）。@param dt 與上一幀的間隔秒數。
    void TickHud(float dt) noexcept {
        if (!topHudMessage_.empty())    topHudAge_    += dt;
        if (!bottomHudMessage_.empty()) bottomHudAge_ += dt;
    }
    /**
     * @brief 立即強制讓指定通道的 HUD 提示過期。
     * @param slot 要關閉的通道。
     *
     * 供「跳過提示」輸入路徑使用，讓自動更新的橫幅可隨時手動關閉。將計齡值直接設到
     * kHudTtl，正是 HudExpired() 判定與繪製提前返回所依據的邊界，因此不改變渲染與外部
     * 紀錄契約——下一趟 View 對該通道單純不繪製任何內容。純資料。
     */
    void DismissHud(HudSlot slot) noexcept {
        if (slot == HudSlot::Top) {
            if (!topHudMessage_.empty())    topHudAge_    = kHudTtl;
        } else {
            if (!bottomHudMessage_.empty()) bottomHudAge_ = kHudTtl;
        }
    }
    /// @brief 一次關閉兩個 HUD 通道，使「跳過提示」維持單一按鍵不論幾個通道在播。
    void DismissHud() noexcept {
        DismissHud(HudSlot::Top);
        DismissHud(HudSlot::Bottom);
    }
    /// @brief 讀取指定通道目前的 HUD 提示文字。@param slot 通道。
    [[nodiscard]] const std::string& HudMessage(HudSlot slot) const noexcept {
        return slot == HudSlot::Top ? topHudMessage_ : bottomHudMessage_;
    }
    /// @brief 讀取指定通道目前的提示計齡（秒）。@param slot 通道。
    [[nodiscard]] float HudAge(HudSlot slot) const noexcept {
        return slot == HudSlot::Top ? topHudAge_ : bottomHudAge_;
    }
    /// @brief 預設通道（Bottom）的提示文字存取。
    [[nodiscard]] const std::string& HudMessage() const noexcept {
        return bottomHudMessage_;
    }
    /// @brief 預設通道（Bottom）的提示計齡存取。
    [[nodiscard]] float HudAge() const noexcept { return bottomHudAge_; }
    /**
     * @brief 判斷指定通道的提示是否已過期（計齡達 kHudTtl）。
     * @param slot 通道。
     * @return 該通道有文字且計齡 >= kHudTtl 時回傳 true。
     *
     * 已過期的提示在畫面上不可見（繪製端已提前返回），但其文字仍被保留以維持 View 淡
     * 出動畫契約（不突兀清空）。此述詞讓非 View 的取用端（主要是輸出狀態紀錄的自動遊
     * 玩流程）對過期提示輸出空行，而非回放一段早已不在畫面上的字串。純唯讀述詞，不改
     * 動狀態、不相依於渲染。
     */
    [[nodiscard]] bool HudExpired(HudSlot slot) const noexcept {
        if (slot == HudSlot::Top) {
            return !topHudMessage_.empty()    && topHudAge_    >= kHudTtl;
        }
        return     !bottomHudMessage_.empty() && bottomHudAge_ >= kHudTtl;
    }
    /// @brief 預設通道（Bottom）的過期判定。
    [[nodiscard]] bool HudExpired() const noexcept {
        return HudExpired(HudSlot::Bottom);
    }

    /**
     * @brief 讓章節 NPC 名冊跟隨學期狀態機，由 GameController 在偵測到狀態變更時呼叫。
     * @param state 新進入的學期狀態。
     *
     * 只移除本方法上次生成、且以原始指標記錄於 chapterRoster_ 的章節 NPC，採單趟延遲
     * remove-erase（絕不於迭代途中移除），再生成新狀態的 NPC。玩家（索引 0）、四把雨
     * 傘、申請書與環境學生皆不受影響，故 objects_.front() 仍是玩家、快取 player_ 仍有效。
     */
    void RespawnChapterRoster(nccu::SemesterState state);

    /**
     * @brief 幀末延遲刪除（標記後清除）：移除所有被標記為非存活的物件。
     *
     * 各 Update 迴圈只標記死亡物件（Deactivate／認領／拾取），絕不於迭代途中 erase；本
     * 方法在幀末以單趟 remove-erase 移除它們，避免迭代器失效並維持「objects_.front()
     * 即玩家」的不變式。玩家是 front 物件、也是一幀中最後才可能被標記死亡者（淋雨致
     * 死），故存活時去除非存活物件不會動到 front。當玩家自身被清除時（其 unique_ptr 即
     * 將銷毀、快取 player_ 將懸空而造成釋放後使用），會在 erase 釋放它之前、趁物件仍存
     * 在時於此清掉快取指標。對無死亡物件的一幀為冪等。
     *
     * 置於 World（而非控制器）上：物件容器與快取 player_ 都是 World 的資料，清除它們是
     * 純粹的資料模型操作，與輸入／渲染無涉。
     */
    void Sweep();

private:
    /**
     * @brief 共用的章節 NPC 生成路徑，使建構子與 RespawnChapterRoster 生成方式一致。
     * @param state 目標學期狀態。
     *
     * 每個生成項於 objects_ 尾端附加一個 NPC，並把其原始指標記錄到 chapterRoster_。
     */
    void SpawnChapterNpcs(nccu::SemesterState state);

    /**
     * @brief 生成指定狀態的章節任務道具（QuestFlagPickup，納入名冊追蹤）。
     * @param state 目標學期狀態。
     *
     * 由進場路徑（SpawnChapterNpcs）與第二章延遲路徑（MaybeSpawnChapter2Notes）共用；
     * 第二章筆記刻意不在進場時生成，詳見 MaybeSpawnChapter2Notes。
     */
    void SpawnChapterQuestItems(nccu::SemesterState state);

    ObjectList                  objects_;             ///< 世界物件容器（front 永遠是玩家）
    Player*                     player_{nullptr};     ///< 對 front 物件的非擁有快取
    std::vector<GameObject*>    chapterRoster_;       ///< 本次生成的章節物件原始指標（換章時據此清除）
    bool                        loadSprites_{true};   ///< 是否載入 sprite（無頭測試為 false）
    SemesterStateMachine        semester_;            ///< 學期狀態機
    BuildingTracker             tracker_;             ///< 建築進入追蹤器
    DialogState                 dialog_;              ///< 對話狀態
    std::string                 currentBuildingName_; ///< 目前所在建築名稱
    // 兩個獨立的 HUD 通道（理由見上方 SetHudMessage）。Top：章節／結局重大進度提示；
    // Bottom：其餘所有 ShowMessage。各自以 TickHud 計齡。
    std::string                 topHudMessage_;       ///< Top 通道提示文字
    float                       topHudAge_{0.0f};     ///< Top 通道計齡（秒）
    std::string                 bottomHudMessage_;    ///< Bottom 通道提示文字
    float                       bottomHudAge_{0.0f};  ///< Bottom 通道計齡（秒）
    CollisionMask               terrainMask_;         ///< 地形可走遮罩
    // 操場繞圈進度（第三章）：繞跑道中心累計的帶號角度；|Σ| ≥ ~2π（一圈）即設立
    // Flag_SportsLapDone。
    bool                        lapStarted_{false};   ///< 是否已錨定首個在跑道帶上的取樣幀
    float                       lapPrevAngle_{0.0f};  ///< 上一幀的角度（用於計算帶號步進）
    float                       lapSwept_{0.0f};      ///< 累計掃過的帶號角度
    /// 第二章散落筆記一次性守衛；本次造訪第二章已生成則為 true，避免重複生成。
    /// 由 RespawnChapterRoster 重置（重新造訪時重新武裝）。
    bool                        ch2NotesSpawned_{false};
    /// 第三章真傘一次性守衛；本次造訪第三章已生成則為 true。由 RespawnChapterRoster 重置。
    bool                        ch3UmbrellaSpawned_{false};
    /// 第一章苦主雨傘一次性守衛；本次造訪第一章已生成則為 true。由 RespawnChapterRoster 重置。
    bool                        ch1VictimUmbrellaSpawned_{false};
    /// 幕間管理員之傘歸還點一次性守衛；本次造訪幕間已生成則為 true。由 RespawnChapterRoster 重置。
    bool                        interludeReturnSpawned_{false};
    bool                        inventoryOpen_{false};   ///< 背包覆蓋層是否開啟
    int                         inventoryCursor_{0};     ///< 背包高亮列索引
    bool                        menuOpen_{false};        ///< 暫停選單是否開啟
    int                         menuCursor_{0};          ///< 暫停選單游標索引
    bool                        helpOpen_{false};        ///< 說明覆蓋層是否開啟
    int                         helpPage_{0};            ///< 說明覆蓋層頁碼索引
    bool                        reducedMotion_{false};   ///< 減少動畫無障礙偏好
    bool                        largeTargets_{false};    ///< 擴大目標無障礙偏好
    AppAction                   pendingAppAction_{AppAction::None}; ///< 待處理的應用層動作意圖
    /// 結局畫面選單游標（0 回首頁、1 重新開始、2 結束）。起始停在回首頁，使結局落地當
    /// 幀的誤觸確認導向非破壞性的回首頁而非結束——與暫停選單同樣的「破壞性項目離起點
    /// 最遠」原則。
    int                         endingMenuCursor_{0};
};

} // namespace nccu

#endif // WORLD_H_
