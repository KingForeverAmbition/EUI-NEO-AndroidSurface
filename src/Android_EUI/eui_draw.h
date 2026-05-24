#pragma once
#include "native_surface/ANativeWindowCreator.h"
#include "TouchHelperA.h"
namespace core::dsl { class Ui; struct Screen; }

extern bool permeate_record;
extern bool permeate_record_ini;
extern struct ANativeWindow*                        g_window;
extern android::ANativeWindowCreator::DisplayInfo   g_displayInfo;
extern int abs_ScreenX, abs_ScreenY;
extern int native_window_screen_x, native_window_screen_y;

void screen_config();
void EuiLayoutUI(core::dsl::Ui& ui, const core::dsl::Screen& screen);
bool EuiShouldRun();
