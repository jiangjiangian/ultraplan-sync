#pragma once
#include "GameObject.h"
#include "raylib.h"
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
    static std::unique_ptr<GameObject> Create(ObjectType type, Vector2 position);
};
