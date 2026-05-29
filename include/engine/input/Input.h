#ifndef GFX_INPUT_H_
#define GFX_INPUT_H_
#include "raylib.h"
#include "engine/input/Key.h"

namespace nccu::engine::input {

/**
 * @file Input.h
 * @brief 多型輸入來源：把按鍵讀取抽象成可替換的單一入口，支援腳本化的無頭重播。
 */

/**
 * @brief 多型輸入提供者的抽象介面。
 *
 * LiveInput（預設）讀取真實 raylib 裝置，正常遊玩逐位元不變；自動遊玩透過
 * Input::SetSource() 安裝腳本化來源，使錄製的時間軸能以可決定、無頭的方式驅動
 * 遊戲。此為 Player／GameController／CharacterSelect 一切按鍵讀取的唯一匯聚點，
 * 替換來源無須改任何呼叫點。
 */
class InputSource {
public:
    virtual ~InputSource() = default;
    /** @brief 按鍵此幀是否被按住。@param[in] k 查詢的鍵。@return 按住回傳 true。 */
    virtual bool IsDown(Key k)     const noexcept = 0;
    /** @brief 按鍵此幀是否剛按下（邊緣）。@param[in] k 查詢的鍵。@return 剛按下回傳 true。 */
    virtual bool IsPressed(Key k)  const noexcept = 0;
    /** @brief 按鍵此幀是否剛放開（邊緣）。@param[in] k 查詢的鍵。@return 剛放開回傳 true。 */
    virtual bool IsReleased(Key k) const noexcept = 0;
};

/** @brief 讀取真實 raylib 裝置的輸入來源（正常遊玩使用）。 */
class LiveInput final : public InputSource {
public:
    bool IsDown(Key k)     const noexcept override { return ::IsKeyDown(ToRaylibKey(k)); }
    bool IsPressed(Key k)  const noexcept override { return ::IsKeyPressed(ToRaylibKey(k)); }
    bool IsReleased(Key k) const noexcept override { return ::IsKeyReleased(ToRaylibKey(k)); }
};

/** @brief 靜態輸入門面：把全程序按鍵查詢轉發到當前生效的 InputSource。 */
struct Input {
    /**
     * @brief 取得當前生效的輸入來源。
     * @return 已設定的來源，否則預設的 LiveInput。
     *
     * 自動遊玩會為該局設定 ScriptInput，並在收尾時還原為 nullptr（即回到 live）。
     * Input 從不擁有該來源；其生命週期由自動遊玩維持整局有效。
     */
    static InputSource* Source() noexcept {
        static LiveInput live;
        return current_ ? current_ : &live;
    }
    /** @brief 設定當前輸入來源（傳 nullptr 回到 live）。@param[in] s 新來源（非擁有）。 */
    static void SetSource(InputSource* s) noexcept { current_ = s; }

    /** @brief 轉發按住查詢至當前來源。@param[in] k 查詢的鍵。@return 按住回傳 true。 */
    static bool IsDown(Key k) noexcept     { return Source()->IsDown(k); }
    /** @brief 轉發剛按下查詢至當前來源。@param[in] k 查詢的鍵。@return 剛按下回傳 true。 */
    static bool IsPressed(Key k) noexcept  { return Source()->IsPressed(k); }
    /** @brief 轉發剛放開查詢至當前來源。@param[in] k 查詢的鍵。@return 剛放開回傳 true。 */
    static bool IsReleased(Key k) noexcept { return Source()->IsReleased(k); }

private:
    inline static InputSource* current_ = nullptr;   ///< 當前來源；null 表示用 live
};

} // namespace nccu::engine::input

#endif // GFX_INPUT_H_
