#include "engine/platform/ScriptInput.h"

#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/dialog/DialogState.h"
#include "engine/input/Key.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "raylib.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出
namespace {

int KeyCode(std::string_view tok) {
    using nccu::engine::input::Key;
    if (tok.size() == 1 && tok[0] >= 'A' && tok[0] <= 'Z')
        return KEY_A + (tok[0] - 'A');
    if (tok == "Enter")     return ToRaylibKey(Key::Enter);
    if (tok == "Escape")    return ToRaylibKey(Key::Escape);
    if (tok == "Tab")       return ToRaylibKey(Key::Tab);
    if (tok == "Space")     return ToRaylibKey(Key::Space);
    if (tok == "Backspace") return ToRaylibKey(Key::Backspace);
    if (tok == "Up")        return ToRaylibKey(Key::Up);
    if (tok == "Down")      return ToRaylibKey(Key::Down);
    if (tok == "Left")      return ToRaylibKey(Key::Left);
    if (tok == "Right")     return ToRaylibKey(Key::Right);
    return -1;
}

// 一行是高階動詞，當且僅當其第一個非空白字元既非數字、也非 '-'（不存在負的幀號）——
// 使傳統的「<幀號> ...」文法維持不變（純增添、永不歧義）。
bool LooksLikeVerb(const std::string& line) {
    for (char c : line) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        if (c == '#') return false;                 // 註解
        return !(std::isdigit(static_cast<unsigned char>(c)) || c == '-');
    }
    return false;                                   // 空行
}


} // namespace

void ScriptInput::Load(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (LooksLikeVerb(line)) {
            std::istringstream ls(line);
            std::string verb;
            ls >> verb;
            if (verb == "goto") {
                Step s; s.verb = Verb::Goto;
                if (ls >> s.x >> s.y) plan_.push_back(std::move(s));
            } else if (verb == "interact") {
                // `interact <npcId>`                — 與帶有該 NpcId 的物件
                //                                     對話／拾取它。
                // `interact <label> <x> <y>`        — 移動到世界座標 (x,y)
                //                                     並在該處按 E。harness
                //                                     計畫沒有別的方式可對非
                //                                     NPC 的世界物件按 E 觸發
                //                                     （雨傘／QuestFlagPickup
                //                                     的 NpcId() 為空，故無法
                //                                     經 NPC 形式抵達——整條
                //                                     A/B/C 主線需要它來拾取
                //                                     TrueUmbrella／申請書）。
                //                                     <label> 只是人類可讀的
                //                                     註解標記；座標才是真正的
                //                                     目標。邊界情形：同時帶有
                //                                     座標的 NpcId 仍解析為該
                //                                     NPC（座標被忽略），故既有
                //                                     形式逐位元不變。
                Step s; s.verb = Verb::Interact;
                if (ls >> s.arg) {
                    float cx, cy;
                    if (ls >> cx >> cy) { s.x = cx; s.y = cy; s.n = 1; }
                    plan_.push_back(std::move(s));
                }
            } else if (verb == "choose") {
                Step s; s.verb = Verb::Choose;
                if (ls >> s.n && s.n >= 0) plan_.push_back(std::move(s));
            } else if (verb == "advance") {
                Step s; s.verb = Verb::Advance;
                plan_.push_back(std::move(s));
            } else if (verb == "wait") {
                Step s; s.verb = Verb::Wait;
                if (ls >> s.n && s.n > 0) plan_.push_back(std::move(s));
            } else if (verb == "quit") {
                Step s; s.verb = Verb::Quit;
                plan_.push_back(std::move(s));
            }
            continue;                               // 非定時指令
        }
        std::istringstream ls(line);
        int frame;
        std::string verb, key;
        if (!(ls >> frame)) continue;               // 空行／註解／'#'
        if (!(ls >> verb)) continue;
        if (verb == "quit") {
            byFrame_[frame].push_back({Directive::Quit, -1});
            continue;
        }
        if (!(ls >> key)) continue;
        const int kc = KeyCode(key);
        if (kc < 0) continue;
        if (verb == "down")       byFrame_[frame].push_back({Directive::Down, kc});
        else if (verb == "up")    byFrame_[frame].push_back({Directive::Up, kc});
        else if (verb == "press") byFrame_[frame].push_back({Directive::Press, kc});
    }
}

void ScriptInput::LoadFile(const std::string& path) {
    std::ifstream in(path);
    Load(in);
}

void ScriptInput::Advance() {
    ++frame_;
    pressed_.clear();
    released_.clear();
    for (int k : autoUp_)
        if (down_.erase(k)) released_.insert(k);
    autoUp_.clear();

    auto it = byFrame_.find(frame_);
    if (it == byFrame_.end()) return;
    for (const Directive& d : it->second) {
        switch (d.kind) {
            case Directive::Down:  SynthDown(d.key);  break;
            case Directive::Up:    SynthUp(d.key);    break;
            case Directive::Press: SynthPress(d.key); break;
            case Directive::Quit:  quit_ = true;      break;
        }
    }
}

// --- 合成邊緣輔助函式（傳統與計畫共用） ------------------
void ScriptInput::SynthDown(int key) {
    if (down_.insert(key).second) pressed_.insert(key);
}
void ScriptInput::SynthUp(int key) {
    if (down_.erase(key)) released_.insert(key);
}
void ScriptInput::SynthPress(int key) {
    down_.insert(key);
    pressed_.insert(key);
    autoUp_.push_back(key);
}



bool ScriptInput::IsDown(nccu::engine::input::Key k) const noexcept {
    return down_.find(ToRaylibKey(k)) != down_.end();
}
bool ScriptInput::IsPressed(nccu::engine::input::Key k) const noexcept {
    return pressed_.find(ToRaylibKey(k)) != pressed_.end();
}
bool ScriptInput::IsReleased(nccu::engine::input::Key k) const noexcept {
    return released_.find(ToRaylibKey(k)) != released_.end();
}

} // namespace nccu
