#include "GameObjectFactory.h"
#include "CashPickup.h"
#include "CursedUmbrella.h"
#include "EnergyDrink.h"
#include "FragileUmbrella.h"
#include "HotPack.h"
#include "Player.h"
#include "ProfessorTrapUmbrella.h"
#include "TrueUmbrella.h"
#include "Vendor.h"
#include "VendorConfig.h"
#include "WaterproofSpray.h"

std::unique_ptr<GameObject> GameObjectFactory::Create(ObjectType type, nccu::gfx::Vec2 position) {
    switch (type) {
        case ObjectType::Player:                return std::make_unique<Player>(position);
        case ObjectType::TrueUmbrella:          return std::make_unique<TrueUmbrella>(position);
        case ObjectType::FragileUmbrella:       return std::make_unique<FragileUmbrella>(position);
        case ObjectType::ProfessorTrapUmbrella: return std::make_unique<ProfessorTrapUmbrella>(position);
        case ObjectType::CursedUmbrella:        return std::make_unique<CursedUmbrella>(position);
        case ObjectType::HotPack:               return std::make_unique<HotPack>(position);
        case ObjectType::WaterproofSpray:       return std::make_unique<WaterproofSpray>(position);
        case ObjectType::EnergyDrink:           return std::make_unique<EnergyDrink>(position);
        case ObjectType::Vendor:
            // Placeholder stall — real market stalls in main.cpp build a
            // VendorConfig with the full stock list and pass it directly.
            return std::make_unique<Vendor>(position,
                VendorConfig{"市集攤主", "歡迎光臨", {{"HotPack", 30}}});
        case ObjectType::CashPickup5:           return std::make_unique<CashPickup>(position, 5);
        case ObjectType::CashPickup10:          return std::make_unique<CashPickup>(position, 10);
        case ObjectType::CashPickup20:          return std::make_unique<CashPickup>(position, 20);
    }
    return nullptr;
}
