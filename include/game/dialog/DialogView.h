#ifndef DIALOG_VIEW_H_
#define DIALOG_VIEW_H_
#include "game/dialog/DialogState.h"

namespace nccu::engine::render { class IRenderer; }

namespace nccu {

// Screen-space dialog box. Draws nothing when the dialog is inactive.
// Panel Rect{20,320,760,110}: fill then a thin border. Line mode -> the
// current line as one text. Choice mode -> one text per option, the
// selected one prefixed "> ". Placeholder framing; swap to a
// resources/assets/ui/ sprite when that art lands.
void DrawDialog(nccu::engine::render::IRenderer& r, const DialogState& d);

} // namespace nccu
#endif // DIALOG_VIEW_H_
