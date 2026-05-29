#ifndef CHARACTER_SELECT_H_
#define CHARACTER_SELECT_H_
/**
 * @file CharacterSelect.h
 * @brief Persona／kPersonas／CharacterSelectResult 的相容性轉出標頭。
 *
 * 這些型別屬於遊戲領域的模型資料（而非 UI），實體定義已移至
 * game/entities/Personas.h。本標頭僅透過遞移 include 轉出它們，維持那些
 * 仍經由 ui/ 路徑取用的 scene／測試之原始碼相容性。新程式碼應直接 include
 * game/entities/Personas.h。
 */
#include "game/entities/Personas.h"

#endif // CHARACTER_SELECT_H_
