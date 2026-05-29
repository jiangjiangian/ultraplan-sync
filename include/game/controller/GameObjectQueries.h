#ifndef GAME_OBJECT_QUERIES_H_
#define GAME_OBJECT_QUERIES_H_
#include "engine/core/GameObject.h"
#include <memory>

/**
 * @file GameObjectQueries.h
 * @brief 走訪 std::unique_ptr<GameObject> 容器的泛型輔助函式：把
 *        「指標有效 && 仍存活」（及可選的略過指標）的防護收斂於一處，
 *        讓遊戲主迴圈讀起來像一行演算法組合。
 */
namespace nccu::queries {

/**
 * @brief 對容器中每個「有效且存活」的物件呼叫 fn。
 * @param[in,out] c  std::unique_ptr<GameObject> 的容器。
 * @param[in]     fn 對每個存活物件呼叫的可呼叫物，簽名為 void(GameObject&)。
 */
template <class Container, class Fn>
void ForEachActive(Container& c, Fn&& fn) {
    for (auto& obj : c) {
        if (obj && obj->IsActive()) fn(*obj);
    }
}

/**
 * @brief 同 ForEachActive，但額外略過指定的物件（通常是 Player 自己）。
 * @param[in,out] c    std::unique_ptr<GameObject> 的容器。
 * @param[in]     skip 要略過的物件（以指標比對，nullptr 表示不略過）。
 * @param[in]     fn   對其餘存活物件呼叫的可呼叫物，簽名為 void(GameObject&)。
 */
template <class Container, class Fn>
void ForEachActiveExcept(Container& c, const GameObject* skip, Fn&& fn) {
    for (auto& obj : c) {
        if (!obj || !obj->IsActive() || obj.get() == skip) continue;
        fn(*obj);
    }
}

} // namespace nccu::queries

#endif // GAME_OBJECT_QUERIES_H_
