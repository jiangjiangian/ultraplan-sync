#ifndef GAME_OBJECT_QUERIES_H_
#define GAME_OBJECT_QUERIES_H_
#include "GameObject.h"
#include <memory>

// Generic helpers for iterating containers of std::unique_ptr<GameObject>.
// Hide the (obj && obj->IsActive()) + optional skip-pointer guard so the
// game loop reads as one-line algorithm composition.
namespace nccu::queries {

template <class Container, class Fn>
void ForEachActive(Container& c, Fn&& fn) {
    for (auto& obj : c) {
        if (obj && obj->IsActive()) fn(*obj);
    }
}

template <class Container, class Fn>
void ForEachActiveExcept(Container& c, const GameObject* skip, Fn&& fn) {
    for (auto& obj : c) {
        if (!obj || !obj->IsActive() || obj.get() == skip) continue;
        fn(*obj);
    }
}

} // namespace nccu::queries

#endif // GAME_OBJECT_QUERIES_H_
