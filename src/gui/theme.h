#pragma once

struct ImFont;

namespace dupcleaner::gui {

extern ImFont* font_regular;
extern ImFont* font_headline;
extern ImFont* font_small;

void ApplyTheme();

void PushPrimaryButton();
void PopPrimaryButton();
void PushDangerButton();
void PopDangerButton();
void PushSuccessText();
void PopSuccessText();

} // namespace dupcleaner::gui
