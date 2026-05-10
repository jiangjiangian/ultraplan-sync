#include "GameObjectFactory.h"
#include "Player.h"
#include "TrueUmbrella.h"

std::unique_ptr<GameObject> GameObjectFactory::Create(ObjectType type, Vector2 position) {
    switch (type) {
        case ObjectType::Player:        return std::make_unique<Player>(position);
        case ObjectType::TrueUmbrella:  return std::make_unique<TrueUmbrella>(position);
    }
    return nullptr;
}
