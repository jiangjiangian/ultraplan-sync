# **《尋傘記：政大山下篇》系統架構與 UML 分析設計書**

本文件詳細描述《尋傘記》C++ / Raylib 2D 遊戲的底層架構，重點應用**物件導向設計 (OOD)** 的封裝、繼承、多型觀念，以及**狀態機 (State Machine)** 來管理學期時序。透過這些進階的軟體工程技術，我們確保了遊戲程式碼的「高內聚、低耦合」，不僅能輕鬆應對期末專案的開發需求，更為未來可能擴充的「山上校區 DLC」或「全新天氣系統」打下堅實的擴充基礎。

## **1\. 系統靜態架構：UML 類別圖 (Class Diagram)**

遊戲的核心是實體系統 (Entity System)。在 2D 俯視角遊戲中，地圖上充斥著各種會動與不會動的物件。為了避免主迴圈（Game Loop）中出現混亂的程式碼，我們建立了一個高度解耦的繼承樹，讓地圖上的玩家、NPC、道具（如盲盒透明傘）都能由同一個系統統一管理、更新與渲染。  
classDiagram  
    %% 定義介面與抽象基底類別  
    class GameObject {  
        \<\<Abstract\>\>  
        \#Vector2 position  
        \#Rectangle hitBox  
        \#bool isActive  
        \#int collisionLayer  
        \+Update(float deltaTime)\*  
        \+Draw()\*  
        \+Interact(Player\* initiator)\*  
        \+CheckCollision(Rectangle other) bool  
    }

    class Character {  
        \<\<Abstract\>\>  
        \#float speed  
        \#Vector2 direction  
        \#Texture2D spriteSheet  
        \#int currentFrame  
        \+Move(Vector2 dir)  
        \+PlayAnimation()  
    }

    class Item {  
        \<\<Abstract\>\>  
        \#string itemName  
        \#bool isPickable  
        \#Texture2D icon  
        \+OnPickup(Player\* player)\*  
    }

    %% 玩家與 NPC  
    class Player {  
        \-float rainMeter  
        \-int karma  
        \-bool hasUmbrella  
        \-Inventory inventory  
        \+HandleInput(float deltaTime)  
        \+decreaseKarma(int amount)  
        \+resetRainMeter()  
        \+Interact(Player\* initiator)  
    }

    class NPC {  
        \-string\[\] dialogLines  
        \-int currentLine  
        \-bool isQuestGiver  
        \+Talk()  
        \+Interact(Player\* initiator)  
        \+UpdateDialogByState(SemesterState state)  
    }

    %% 透明傘多型系統  
    class TransparentUmbrella {  
        \<\<Abstract\>\>  
        \#Color umbrellaTint  
        \+Interact(Player\* initiator)\*  
        \+beClaimed(Player\* player)\*  
    }

    class TrueUmbrella {  
        \+beClaimed(Player\* player)  
    }

    class FragileUmbrella {  
        \-float leakRate  
        \+beClaimed(Player\* player)  
    }

    class ProfessorTrapUmbrella {  
        \-int spawnedEnemiesCount  
        \+beClaimed(Player\* player)  
    }

    class CursedUmbrella {  
        \-int karmaPenalty  
        \+beClaimed(Player\* player)  
    }

    %% 繼承關係  
    GameObject \<|-- Character  
    GameObject \<|-- Item  
    Character \<|-- Player  
    Character \<|-- NPC  
    Item \<|-- TransparentUmbrella  
    TransparentUmbrella \<|-- TrueUmbrella  
    TransparentUmbrella \<|-- FragileUmbrella  
    TransparentUmbrella \<|-- ProfessorTrapUmbrella  
    TransparentUmbrella \<|-- CursedUmbrella  
      
    %% 關聯關係  
    Player \--\> GameObject : "Interacts with \>"

### **類別設計與技術細節說明：**

* **GameObject (抽象類別與實體根基)**：  
  這是所有地圖物件的絕對源頭。它不僅封裝了 2D 空間中的座標 (position) 與碰撞框 (hitBox)，更引入了 isActive 標籤以支援「物件池 (Object Pooling)」或「畫面外剔除 (Culling)」的效能優化機制。它擁有純虛擬函式 Interact()，這是一種強制性的合約，確保所有繼承它的子類別都必須有自己的互動邏輯，主程式便能以統一的 GameObject\* 指標陣列來迴圈呼叫繪圖與互動。  
* **Player (玩家控制中心)**：  
  除了處理基本的 WASD 輸入外，Player 類別更是遊戲狀態的數據中心。它管理著核心的 rainMeter (淋雨度，數值過高會觸發減速懲罰) 與 karma (道德值，決定最終結局的關鍵隱藏數值)。這種設計將「玩家的狀態」與「玩家的顯示」完美結合，外部物件（如雨傘或 NPC）只能透過公開的介面（如 decreaseKarma）來改變玩家狀態，徹底落實了「封裝 (Encapsulation)」的精神。  
* **多型展現 (TransparentUmbrella 家族)**：  
  這是本遊戲最核心的巧思。這四種傘在遊戲畫面上共用相同的 Texture2D 資源（外觀完全一樣），但卻擁有各自實作的 beClaimed() 虛擬函式。  
  * TrueUmbrella：觸發破關與溫馨音效。  
  * FragileUmbrella：裝備後玩家依然會緩慢增加淋雨度，模擬骨架斷掉漏水的慘況。  
  * ProfessorTrapUmbrella：此類別內部實作了生成機制，一旦被撿起，會在周圍座標動態實例化 (Instantiate) 數個「助教 NPC」並切換至追逐狀態。  
  * CursedUmbrella：直接呼叫玩家的指標扣除道德值，並觸發灰暗濾鏡。  
    當玩家按下 E 鍵時，系統觸發多型的**動態綁定 (Dynamic Binding)**，在執行期 (Runtime) 才決定要執行哪一種破壞力十足的邏輯，完美還原了政大「透明傘盲盒」的隨機性與驚喜感。

## **2\. 系統動態架構：UML 狀態機圖 (State Machine Diagram)**

遊戲的進程由一個全域的「學期狀態機」控制。這種設計解決了傳統 RPG 遊戲中「同一張地圖需要製作多個副本」的資源浪費問題。在《尋傘記》中，我們始終只有一張「政大山下校區」的底圖，但根據狀態的不同，渲染層與物件層會發生劇烈的動態變化。  
stateDiagram-v2  
    \[\*\] \--\> Chapter1\_AddDrop : 遊戲開始 (2月開學季)

    state Chapter1\_AddDrop {  
        \[\*\] \--\> 綜院掉傘  
        綜院掉傘 \--\> 集英樓尋找 : 獲得苦主線索  
        集英樓尋找 \--\> 找回傘1 : 完成學長跑腿任務  
    }

    Chapter1\_AddDrop \--\> Interlude\_Market : 找回傘1後觸發時間推進

    state Interlude\_Market {  
        \[\*\] \--\> 四維道市集  
        四維道市集 \--\> 購買道具 : 花費金幣換取防雨道具  
        購買道具 \--\> 狀態重置 : 降低後續關卡淋雨值累積率  
    }

    Interlude\_Market \--\> Chapter2\_Midterms : 時間流逝 (4月期中考)

    state Chapter2\_Midterms {  
        \[\*\] \--\> 圖書館掉傘  
        圖書館掉傘 \--\> 尋找學霸 : 蒐集提神飲料  
        尋找學霸 \--\> 找回傘2 : 喚醒學霸並交換  
    }

    Chapter2\_Midterms \--\> Chapter3\_SportsDay : 找回傘2後觸發時間推進

    state Chapter3\_SportsDay {  
        \[\*\] \--\> 運動會掉傘  
        運動會掉傘 \--\> 攤位物物交換 : 大型市集解謎網路  
        攤位物物交換 \--\> 找回傘3 : 體育館後台拯救被當道具的傘  
    }

    Chapter3\_SportsDay \--\> Chapter4\_Finals : 找回傘3後觸發時間推進

    state Chapter4\_Finals {  
        \[\*\] \--\> 終局掉傘  
        終局掉傘 \--\> 崩潰的校園 : 啟用最大暴雨粒子特效  
        崩潰的校園 \--\> 最終抉擇 : 抵達集英樓傘架前  
    }

    Chapter4\_Finals \--\> Ending\_A\_True : \[條件\] 道德值高 & 完整解謎  
    Chapter4\_Finals \--\> Ending\_B\_Bad : \[條件\] 道德值低 (偷竊他人的傘)  
    Chapter4\_Finals \--\> Ending\_C\_Normal : \[條件\] 於商店花錢買醜綠傘

    Ending\_A\_True \--\> \[\*\]  
    Ending\_B\_Bad \--\> \[\*\]  
    Ending\_C\_Normal \--\> \[\*\]

### **狀態機設計與環境連動說明：**

* **情境渲染 (Contextual Rendering)**：  
  主程式中維護著一個 SemesterState currentState 列舉變數。在 Raylib 的 Draw() 迴圈中，系統會大量依賴這個變數來決定繪製內容。例如，當狀態進入 Chapter4\_Finals 時，系統不僅會啟動全螢幕的暴雨粒子發射器 (Particle Emitter)，還會將環境光照調暗，並加快玩家 rainMeter 的累積速度。  
* **NPC 對話樹的動態切換**：  
  NPC 類別中包含了 UpdateDialogByState(SemesterState state) 方法。同一個福利社阿姨，在 Chapter1 時會抱怨加簽人潮，在 Chapter3 時會因為運動會生意太好而不耐煩，在 Chapter4 則會展現出期末考的疲憊。這讓整個校園具備了真實的時間流動感。  
* **事件觸發與轉換 (Event Triggers)**：  
  狀態之間的轉換並非隨機，而是由特定旗標（Flags）觸發。例如，只有當玩家身上的 inventory.hasItem("TrueUmbrella") 狀態為真，且走回宿舍的 Trigger Zone 時，系統才會將狀態機推進到下一個 Chapter。

## **3\. 系統互動架構：UML 循序圖 (Sequence Diagram)**

這個循序圖展示了遊戲中最核心、最具張力的一刻：當玩家走近一把「未知的透明傘」並按下互動鍵（E 鍵）時，系統背後是如何利用記憶體指標、碰撞偵測與動態綁定來觸發多型的。  
sequenceDiagram{  
    participant Input as Raylib Input Module  
    participant P as Player  
    participant Map as Map/Collision Manager  
    participant U as TransparentUmbrella (Abstract Pointer)  
    participant C as CursedUmbrella (Actual Instance)  
    participant UI as UI & Event Manager

    Input-\>\>P: 偵測到按下 'E' 鍵 (Interact)  
    P-\>\>Map: 請求 CheckCollision(PlayerHitBox \+ 互動面向, 互動範圍)  
      
    alt 無碰撞物件  
        Map--\>\>P: 回傳 nullptr (無事發生)  
    else 找到互動目標  
        Map--\>\>P: 回傳面前的物件指標 (GameObject\*)  
    end  
      
    Note over P, C: 玩家端不需要進行向下轉型 (Downcasting) 或型別檢查  
    P-\>\>U: 呼叫 target-\>Interact(this)  
      
    Note over U, C: C++ 虛擬函式表 (VTable) 發揮作用，動態綁定生效  
    U-\>\>C: 轉交給記憶體中實際的子類別執行 beClaimed(player)  
      
    C-\>\>P: 呼叫 player-\>decreaseKarma(50)  
    P--\>\>C: 玩家道德值成功扣除  
      
    C-\>\>UI: 發送事件 ShowMessage("你順手牽羊了！成為了你最討厭的人。")  
    UI--\>\>C: UI 顯示確認  
      
    C-\>\>Map: 請求將自己從地圖實體列表中移除 (Flag as inactive / Delete)  
    Map--\>\>C: 物件銷毀，釋放記憶體  
}

### **循序圖設計與架構優勢分析：**

這張圖完美詮釋了軟體工程中\*\*「針對介面寫程式，而非針對實作寫程式 (Program to an interface, not an implementation)」\*\*的黃金法則。

1. **極致的解耦 (Decoupling)**：  
   Player 類別從頭到尾都沒有 include "CursedUmbrella.h"，它根本不知道「惡龍傘」的存在。它只依賴於 TransparentUmbrella 這個抽象介面。這意味著未來如果要新增「會飛的傘」或「會爆炸的傘」，Player 的程式碼連一行都不需要修改。  
2. **安全的記憶體操作**：  
   在 C++ 遊戲開發中，動態建立與銷毀物件是引發 Memory Leak 的主因。在此架構中，當傘被拾取後，它會向 Map Manager 發送銷毀請求，由地圖管理器統一在當前幀 (Frame) 結束時安全地 delete 指標並從 std::vector 中移除，避免了迭代器失效 (Iterator Invalidation) 的崩潰問題。  
3. **事件驅動的 UI 更新 (Observer Pattern 雛形)**：  
   傘本身並不負責繪製文字，而是發送訊息給 UI Manager。這種關注點分離 (Separation of Concerns) 確保了遊戲的底層邏輯與畫面的渲染完全獨立。