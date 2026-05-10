#pragma once
#include "GameObject.h"
#include "gfx/Vec2.h"
#include <memory>

enum class ObjectType {
    Player,
    TrueUmbrella,
    FragileUmbrella,
    ProfessorTrapUmbrella,
    CursedUmbrella,
};

class GameObjectFactory {
public:
    static std::unique_ptr<GameObject> Create(ObjectType type, nccu::gfx::Vec2 position);
};
