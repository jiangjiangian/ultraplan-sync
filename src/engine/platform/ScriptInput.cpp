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
namespace {

int KeyCode(std::string_view tok) {
    using nccu::gfx::Key;
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

// A line is a high-level verb iff its first non-space char is not a digit
// and not '-' (no negative frame numbers exist) — keeps the classic
// "<frame> ..." grammar working untouched (additive, never ambiguous).
bool LooksLikeVerb(const std::string& line) {
    for (char c : line) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        if (c == '#') return false;                 // comment
        return !(std::isdigit(static_cast<unsigned char>(c)) || c == '-');
    }
    return false;                                   // blank
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
                // `interact <npcId>`                — talk to / pick up
                //                                     the object with
                //                                     that NpcId.
                // `interact <label> <x> <y>`        — drive to world
                //                                     (x,y) and tap E
                //                                     there. The harness
                //                                     plan has no other
                //                                     way to E-actuate a
                //                                     non-NPC world
                //                                     object (umbrellas /
                //                                     QuestFlagPickups
                //                                     have an empty
                //                                     NpcId(), so they
                //                                     are unreachable by
                //                                     the NPC form — the
                //                                     whole A/B/C spine
                //                                     needs this to claim
                //                                     the TrueUmbrella /
                //                                     the 申請書). <label>
                //                                     is a human-readable
                //                                     comment token only;
                //                                     the coords are the
                //                                     actual target. Edge
                //                                     case: an NpcId that
                //                                     also has coords
                //                                     still resolves as
                //                                     the NPC (coords
                //                                     ignored) so the
                //                                     existing form is
                //                                     byte-unchanged.
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
            continue;                               // not a timed directive
        }
        std::istringstream ls(line);
        int frame;
        std::string verb, key;
        if (!(ls >> frame)) continue;               // blank / comment / '#'
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

// --- synthetic edge helpers (shared by classic + plan) ------------------
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



bool ScriptInput::IsDown(gfx::Key k) const noexcept {
    return down_.find(ToRaylibKey(k)) != down_.end();
}
bool ScriptInput::IsPressed(gfx::Key k) const noexcept {
    return pressed_.find(ToRaylibKey(k)) != pressed_.end();
}
bool ScriptInput::IsReleased(gfx::Key k) const noexcept {
    return released_.find(ToRaylibKey(k)) != released_.end();
}

} // namespace nccu
