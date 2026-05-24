#pragma once
#include "native_surface/ANativeWindowCreator.h"
#include "TouchHelperA.h"
namespace core::dsl { class Ui; struct Screen; }

// 防录制开关
extern bool permeate_record;
extern bool permeate_record_ini;

// Android surface 句柄
extern struct ANativeWindow*                        g_window;
extern android::ANativeWindowCreator::DisplayInfo   g_displayInfo;

// 屏幕绝对分辨率
extern int abs_ScreenX, abs_ScreenY;
extern int native_window_screen_x, native_window_screen_y;

// 初始化
void screen_config();
void EuiLayoutUI(core::dsl::Ui& ui, const core::dsl::Screen& screen);
bool EuiShouldRun();
