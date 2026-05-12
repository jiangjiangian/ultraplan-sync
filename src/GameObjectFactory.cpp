#include "GameObjectFactory.h"
#include "Player.h"
#include "TrueUmbrella.h"
#include "FragileUmbrella.h"
#include "ProfessorTrapUmbrella.h"
#include "CursedUmbrella.h"
#include "HotPack.h"
#include "WaterproofSpray.h"
#include "EnergyDrink.h"

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
    }
    return nullptr;
}
