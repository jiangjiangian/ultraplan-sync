#ifndef DIALOG_VIEW_H_
#define DIALOG_VIEW_H_
#include "DialogState.h"

namespace nccu { namespace gfx { class IRenderer; } }

namespace nccu {

// Screen-space dialog box. Draws nothing when the dialog is inactive.
// Panel Rect{20,320,760,110}: fill then a thin border. Line mode -> the
// current line as one text. Choice mode -> one text per option, the
// selected one prefixed "> ". Placeholder framing; swap to a
// resources/assets/ui/ sprite when that art lands.
void DrawDialog(nccu::gfx::IRenderer& r, const DialogState& d);

} // namespace nccu
#endif // DIALOG_VIEW_H_
