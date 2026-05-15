# awsome_cpp.md — OOP Lab 6–10 精煉筆記

> 記助教在 lab 中提過的 C++ 巧妙寫法、容易踩雷的細節、與物件導向設計的關鍵抉擇。
> 來源：政大 1142 OOP Lab 6/7/8/9/10。

---

## 1. Template（Lab10）

### 1.1 函式 template

```cpp
template <typename T>           // 也可寫 template <class T>
T add(T a, T b) { return a + b; }

template <typename T1, typename T2, typename T3>
T3 combine(T1 a, T2 b);         // 多型別
```

關鍵規則：
- 宣告 / 定義分開時，**兩邊都要寫 `template<...>`**。
- 函式體只要用到 `T`，就一定要在前面宣告 `template<typename T>`。
- 只宣告一個 `T`，呼叫端就只能傳入同型別參數。

### 1.2 Class template

```cpp
template <typename T>
class Box {
    T value_;
public:
    Box(T v) : value_(v) {}
    T get() const { return value_; }
};

Box<int> a(3);
Box<std::string> b("hi");
```

### 1.3 Template 的編譯特性 — 易踩雷

- **每一個被使用到的型別都會獨立產生一份 machine code**（程式碼膨脹）。
- **沒被用到的型別不會被編譯** —— 所以 template 函式如果把宣告放 `.h`、實作放 `.cpp`，**連結期會找不到符號**。
- **解法**：template 的宣告與實作要寫在同一個 header（或 `.inl/.tcc` 並在 header 末尾 `#include`）。

---

## 2. CRTP（Curiously Recurring Template Pattern）— Lab10

### 2.1 寫法

```cpp
template <typename Derived>
class Base {
public:
    void interface() {                    // 父類呼叫子類方法
        static_cast<Derived*>(this)->impl();
    }
private:
    Base() = default;                     // 防止使用者直接實例化 Base<T>
    friend Derived;                       // 只開放給正確的子類
};

class A : public Base<A> {                // template 名稱必須與子類同名
    friend Base<A>;
    void impl() { /* ... */ }
};
```

`friend Derived` + `Base() = default;` 的組合可以同時防止 typo 與防止使用者直接用 `Base<X>`。

### 2.2 CRTP vs Virtual

| 維度 | CRTP（靜態） | Virtual（動態） |
|---|---|---|
| 記憶體 | 低（無 vtable） | 高（每個物件多一個 vptr，每類多一個 vtable） |
| 呼叫成本 | 編譯時 `static_cast`，可 inline | 執行時一次間接跳轉 |
| 動態性 | 低 | 高 |
| 語法 | 複雜 | 簡單 |

### 2.3 CRTP 致命弱點（這就是助教提的缺點）

> **無法 `std::vector<Base*>` 統一儲存**。
>
> 因為 `Base<A>` 和 `Base<B>` 是「兩個完全不同的型別」，沒有共同父類能當容器元素型別。

### 2.4 神奇解法（CRTP 的現代替代品）

#### 解法 A：`std::variant` + `std::visit`（C++17，最常用）

```cpp
using Shape = std::variant<Circle, Square, Triangle>;
std::vector<Shape> shapes;          // 真‧異質容器

for (auto& s : shapes) {
    std::visit([](auto& concrete) { concrete.draw(); }, s);
}
```

- 優點：**值語意**（無 heap、無虛擬呼叫、cache 友善），編譯期完整型別檢查。
- 缺點：**所有型別必須先知道**（無法做 plugin），每個 `std::visit` 會生一個小 vtable，散得太碎反而比 virtual 慢。

#### 解法 B：Type Erasure（Sean Parent 的招牌技巧）

```cpp
class Drawable {                                          // 對外是 value 型別
    struct Concept { virtual ~Concept() = default; virtual void draw() const = 0; };
    template <typename T> struct Model : Concept {
        T data_;
        Model(T t) : data_(std::move(t)) {}
        void draw() const override { data_.draw(); }
    };
    std::unique_ptr<Concept> self_;
public:
    template <typename T> Drawable(T x) : self_(std::make_unique<Model<T>>(std::move(x))) {}
    void draw() const { self_->draw(); }
};

std::vector<Drawable> shapes;     // 真正的多型容器，但繼承被藏起來了
```

- 把繼承藏在內部，外部只看到一個值型別。
- 任何「有 `draw()` 函式」的東西都能塞進去（duck typing）。

#### 解法 C：C++20 Concepts（取代 CRTP 的「靜態介面」用途）

```cpp
template <typename T>
concept Shape = requires(T t) { { t.area() } -> std::same_as<double>; };

double total(const Shape auto&... s) { return (s.area() + ...); }
```

- **概念非侵入式** —— 不需要繼承任何東西，比 CRTP 乾淨。
- 但概念**無法注入成員函式**，只能限制呼叫者；要呼叫共同方法仍須靠模板或 free function。

#### 解法 D：C++23 `deducing this`

```cpp
class Base {
public:
    template <typename Self>
    void interface(this Self&& self) { self.impl(); }    // 自動拿到正確子類
};
class A : public Base { friend Base; void impl(); };
```

- 終於不必再 `static_cast<Derived*>(this)` —— **CRTP 從此可以丟掉**。

#### 建議（給尋傘記用）

- 對於 `TransparentUmbrella` 4 個變體：**繼續用 virtual**，因為遊戲世界本來就要 `std::vector<unique_ptr<GameObject>>` 統一掃。
- 若日後某個熱迴圈出現效能問題，再考慮把對應子集改成 `std::variant`。
- 不要追求極致純粹 CRTP — 它「沒辦法異質容器」的限制天生不適合 ECS / scene graph。

---

## 3. Virtual 與 Vtable 三條鐵律（Lab8）

1. **Base 的 destructor 一定要 `virtual`**。否則 `delete basePtr` 時只跑父解構子，子類資源洩漏。
2. **Constructor 不能是 virtual**。物件還沒生出來，vptr 還沒指好。
3. **不要在 constructor / destructor 裡呼叫 virtual function**。當前 vtable 還停在父類版本，會打到父類實作而不是你期望的子類版本。

### 3.1 Vtable 機制速記

- 一個 class 裡有任一個 `virtual`，編譯器就會給它做一張 vtable（每個函式一個 entry），每個物件多 8 bytes 的 vptr。
- vptr 是 constructor 寫進去的；子類 constructor 跑完前還是父類 vtable，這就是規則 3 的根因。

---

## 4. Interface（純抽象類別）設計守則（Lab8）

```cpp
class IDrawable {            // 介面常見命名：I 前綴 或 ...able
public:
    virtual ~IDrawable() = default;
    virtual void Draw() const = 0;
};
```

**介面內不應該有 attribute（資料成員）。理由：**

1. **行為契約 vs 狀態描述**：介面是「能做什麼」，不應該規定「裡面存什麼」。
2. **避免多重繼承的記憶體膨脹**：子類同時繼承多個有資料成員的介面，會出現重複欄位，必須 `virtual public` 來解菱形繼承，多一層執行期開銷。

> 對比抽象類別（abstract class）：可以有部分實作好的方法、可以有資料成員，但仍含至少一個純虛擬函式，所以不能實例化。

---

## 5. 多型的三種型態（Lab8）

| 型態 | 機制 | C++ 範例 |
|---|---|---|
| **Ad-hoc**（靜態） | Operator overloading、function overloading | `operator+(Point, Point)` |
| **Parametric**（靜態） | Template、Generics | `template<typename T> sort(vector<T>&)` |
| **Subtyping**（動態） | Virtual function | `Animal* a; a->speak();` |

「多型」≠ 只有 virtual。Ad-hoc 與 Parametric 都算。

---

## 6. MVC — UI/Data 解耦正解（Lab10 助教明示）

```
+-----------+         +-------------+         +-------+
|  Model    | <-----  | Controller  | ----->  | View  |
| (純資料)  |  狀態   | (協調 + 事件) |  畫面   | (繪製) |
+-----------+         +-------------+         +-------+
```

- **Model**：只有資料 + 業務邏輯，不知道任何畫面 API。在尋傘記裡 = `Player`, `Item`, `GameObject` 樹。
- **View**：只負責畫畫面 — `DrawCircleV`, `DrawTexture` 等 raylib 呼叫都應該集中在這層。
- **Controller**：在 `update()` 裡收滑鼠 / 鍵盤事件、呼叫 Model.update()、再驅動 View 重繪。

Game loop 寫法（Raylib）：

```cpp
InitWindow(...);
SetTargetFPS(60);           // 限制 FPS — 加在 InitWindow 後
while (!WindowShouldClose()) {
    controller.update();    // 處理輸入、推進 model、發事件
    BeginDrawing();
    view.draw(model);       // 純畫圖
    EndDrawing();
}
```

**警告（Lab9）**：Controller 容易膨脹成 God Class，內部仍須職責分離 — 例如把 InputHandler / SceneRouter / EventDispatcher 拆出來。

對應到尋傘記的 EventBus：Controller 把「Item.OnPickup」轉成事件，View 訂閱 `RenderRequested / ShowMessage` 來繪製訊息框 —— 這就是現有 EventBus 的設計動機。

---

## 7. SOLID 簡記（Lab9）

| 字母 | 名稱 | 一句話 | 違反警訊 |
|---|---|---|---|
| S | Single Responsibility | 一個類別只應有一個「修改的理由」 | 一個 class 兼做計算與繪圖 |
| O | Open-Closed | 對擴充開放、對修改封閉 | 加新型別就要修現有 switch |
| L | Liskov Substitution | 子類能無痛替換父類 | 經典反例：Square is-a Rectangle |
| I | Interface Segregation | 客戶不該被迫依賴用不到的方法 | 一個介面塞 10 個方法，子類大半空實作 |
| D | Dependency Inversion | 高低階模組都依賴抽象 | 高階模組 `#include` 具體實作 |

附註：
- **Singleton 常違反 SRP / DIP**（Lab9）。EventBus 是 Singleton，要警惕。
- **MVC 一次滿足 SRP / OCP / DIP / ISP**（Lab9）。

---

## 8. 繼承順序鐵則（Lab7）

```cpp
class B : public A { ... };
B b;
// Constructor: A() -> B()
// Destructor : ~B() -> ~A()
```

- **Constructor 順序：父先，子後**
- **Destructor 順序：子先，父後**
- 子類 constructor 沒寫 `: ParentName(...)` 時，編譯器自動呼叫父類**預設 constructor**。父類沒有預設 constructor 就會編譯錯。

### 8.1 繼承存取權交集表（Lab7）

| 繼承方式 \ 父成員 | public | protected | private |
|---|---|---|---|
| `public` 繼承 | public | protected | 隱藏 |
| `protected` 繼承 | protected | protected | 隱藏 |
| `private` 繼承 | private | private | 隱藏 |

口訣：**取兩者較嚴格者**。`private` member **被繼承但子類無法存取**（封裝）。

---

## 9. 六種類別關係（Lab6）UML 速查

| 關係 | 中文 | 例子 | UML 箭頭 |
|---|---|---|---|
| Dependency | 依賴 | Waiter 用 Order | 虛線箭頭 |
| Association | 關聯 | Player has Position | 實線箭頭 |
| Aggregation | 聚合 | School owns Student | 空心菱形 |
| Composition | 組合 | CPU 屬於 Computer，生命週期共享 | 實心菱形 |
| Generalization | 繼承 | Bike is-a Vehicle | 空心三角箭頭 |
| Realization | 實現 | DomesticCat 實現 Cat 抽象 | 虛線 + 空心三角 |

---

## 10. 設計腐爛味（Lab6）— 自我檢測

| 味道 | 症狀 |
|---|---|
| Rigidity | 改一處要連動改很多處 |
| Fragility | 改一處導致看起來無關的地方壞掉 |
| Immobility | 想抽出模組重用，但拔不出來 |
| Viscosity | 走 hack 比走正規修改容易 |
| Needless Complexity | 為「萬一」預留的抽象從沒被用到 |
| Needless Repetition | 同樣邏輯散落各處 |
| Opacity | 看不懂自己一個月前寫的東西 |

---

## 11. Cohesion / Coupling（Lab6）

- **高內聚（Cohesion）**：一個模組「只做一件事」——好。
- **低耦合（Coupling）**：模組之間關聯越少越好。

> 服務生只負責點餐、把訂單送進廚房 — 不需要了解廚房怎麼做菜，也不用操作收銀機。

---

## 12. Smart Pointer 速記（Lab8）

| 種類 | 用途 |
|---|---|
| `auto_ptr` | 已 deprecated，不要用 |
| `unique_ptr<T>` | 獨佔擁有，move-only。**99% 場景用這個** |
| `shared_ptr<T>` | 共享所有權（引用計數）。需要循環引用才考慮 |
| `weak_ptr<T>` | 配合 `shared_ptr` 解循環引用 |

預設用 `unique_ptr`。`shared_ptr` 不便宜（atomic refcount + control block），不要當預設值。

---

## 13. Singleton 結構與風險（Lab6 + Lab9）

```cpp
class EventBus {
public:
    static EventBus& Instance() {
        static EventBus inst;          // C++11 起 thread-safe 區域 static
        return inst;
    }
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
private:
    EventBus() = default;
};
```

- Constructor / Copy / Assign 都要 private 或 delete。
- 用區域 static 比 `static EventBus* inst = new EventBus()` 安全（自動析構、延遲初始化、執行緒安全）。
- **Singleton 常被批評違反 SRP / DIP** —— 它把「存取點」與「資料」綁死，難以注入測試替身。

---

## 14. 多重繼承與菱形（Lab8）

```cpp
class A { public: int x; };
class B : virtual public A { ... };      // 注意 virtual
class C : virtual public A { ... };
class D : public B, public C { ... };    // 只會有一份 A::x
```

- 沒寫 `virtual public A`，D 裡面會有兩份 A 的成員 → ambiguous access。
- 助教建議：**One inheritance, multi implementation** — 真正繼承的只走一條鏈，其他用介面實現。

---

## 15. 關鍵 OOP 觀念精簡題庫（Lab8 期中複習）

- **C++ Reference 的作用？** 別名 + 避免複製 + 必須初始化且不可重新綁定。
- **OO 三大特性？** Encapsulation / Inheritance / Polymorphism。
- **Big Three？** Copy constructor / Copy assignment / Destructor。若有手動管理資源 → 三個都要實作（現代加上 move constructor / move assignment 變 Big Five）。
- **如何 overload `Point2D` 的運算子？**
  ```cpp
  Point2D operator+(const Point2D& rhs) const;   // 二元運算
  Point2D& operator+=(const Point2D& rhs);       // 回 reference 支援鏈式 a+=b
  bool operator==(const Point2D& rhs) const;
  ```

---

## 16. STL Iterator（Lab10）

迭代器封裝「位置 + 移動 + 解參考」。

```cpp
for (auto it = v.begin(); it != v.end(); ++it) { ... }
```

為什麼不用指標？因為 `std::list`、`std::map`、`std::unordered_set` **記憶體不連續**，指標算術會壞掉。

---

## 17. 引用來源

- OOP26_Lab6.pdf — OOD part 1 + Raylib
- OOP26_Lab7.pdf — Inheritance
- OOP26_Lab8.pdf — Advanced Inheritance（polymorphism, vtable, interface, multi-inheritance, smart pointer）
- OOP26_Lab9.pdf — OOD part 2（SOLID + design patterns intro）
- OOP26_Lab10.pdf — Template & STL（CRTP, MVC, iterator）
- Fluent C++ — Replacing CRTP Static Polymorphism With Concepts
- C++ Stories — Runtime Polymorphism with std::variant and std::visit
- Dave Kilian — C++ Type Erasure Explained
- Wikipedia — Curiously recurring template pattern
