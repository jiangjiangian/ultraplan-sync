#ifndef GAME_OBJECT_FACTORY_H_
#define GAME_OBJECT_FACTORY_H_
#include "engine/core/GameObject.h"
#include "engine/math/Vec2.h"
#include <memory>

/**
 * @file GameObjectFactory.h
 * @brief GoF Factory Method：以列舉標籤集中生產各種 GameObject 子類別，
 *        呼叫端只認得抽象基底而不必 #include 任何具體實體型別。
 */

/** @brief 工廠可生產的物件種類；每個列舉值對應一個具體 GameObject 子類別。 */
enum class ObjectType {
    Player,
    TrueUmbrella,
    FragileUmbrella,
    ProfessorTrapUmbrella,
    CursedUmbrella,
    HotPack,
    WaterproofSpray,
    EnergyDrink,
    /// 商店櫃檯 NPC。工廠僅產生佔位攤位；正式市集攤位於 main.cpp 直接以填好的
    /// VendorConfig 建構 Vendor。
    Vendor,
    /// 地面金錢拾取物，分三種面額——數量少到「一值一列舉」比帶參數的工廠更易讀。
    CashPickup5,
    CashPickup10,
    CashPickup20,
};

/**
 * @brief GoF Factory：把「列舉標籤 -> 具體物件」的建構集中於單一 Create()。
 *
 * 將 new 的決策從呼叫端抽離，呼叫端只需 ObjectType 與座標即可取得抽象
 * GameObject，新增物件種類只改這裡（OCP），不必散落各處改 include 與建構碼。
 */
class GameObjectFactory {
public:
    /**
     * @brief 依種類建構對應的 GameObject 子類別。
     * @param[in] type     要生產的物件種類。
     * @param[in] position 物件在世界座標系的初始位置（像素）。
     * @return 持有新物件的 unique_ptr；type 無對應時回傳 nullptr。
     */
    static std::unique_ptr<GameObject> Create(ObjectType type, nccu::engine::math::Vec2 position);
};

#endif // GAME_OBJECT_FACTORY_H_
