#ifndef GAME_OBJECT_FACTORY_H_
#define GAME_OBJECT_FACTORY_H_
#include "engine/core/GameObject.h"
#include "engine/math/Vec2.h"
#include <memory>

enum class ObjectType {
    Player,
    TrueUmbrella,
    FragileUmbrella,
    ProfessorTrapUmbrella,
    CursedUmbrella,
    HotPack,
    WaterproofSpray,
    EnergyDrink,
    // Shop counter NPC. The factory only produces a placeholder stall;
    // production stalls in main.cpp construct Vendor directly with a
    // populated VendorConfig.
    Vendor,
    // Ground money pickups in three denominations — small enough that an
    // enum-per-value reads better than a parameterised factory.
    CashPickup5,
    CashPickup10,
    CashPickup20,
};

class GameObjectFactory {
public:
    static std::unique_ptr<GameObject> Create(ObjectType type, nccu::engine::math::Vec2 position);
};

#endif // GAME_OBJECT_FACTORY_H_
