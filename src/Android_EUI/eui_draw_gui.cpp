/*
 * eui_draw_gui.cpp
 *
 * 用户 UI 层：替代原 draw_Gui.cpp（ImGui 版）
 * 使用 EUI-NEO 的正式 DSL / Components API
 *
 * 所有组件用法参考 EUI-NEO-Android 的 android_app.cpp
 */

#include "components/components.h"
#include "core/dsl.h"
#include "core/text.h"

#include "native_surface/ANativeWindowCreator.h"

#include <cstdio>
#include <string>

// ── 全局状态 ─────────────────────────────────────────────────────
bool permeate_record     = false;
bool permeate_record_ini = false;

ANativeWindow*                             g_window        = nullptr;
android::ANativeWindowCreator::DisplayInfo g_displayInfo   = {};
int abs_ScreenX = 0, abs_ScreenY = 0;
int native_window_screen_x = 0, native_window_screen_y = 0;

static bool  s_showAnotherWindow = false;
static bool  s_showDemoWindow    = false;
static float s_sliderVal         = 0.44f;
static int   s_counter           = 0;
static int   s_themeIdx          = 0;   // 0=深色 1=浅色
static bool  s_switchVal         = false;

static bool  s_mainFlag          = true;

static components::theme::ThemeColorTokens getTheme() {
    return s_themeIdx == 1
        ? components::theme::LightThemeColors()
        : components::theme::DarkThemeColors();
}

void screen_config() {
    g_displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

// ── 主 UI 构建函数 ────────────────────────────────────────────────
void EuiLayoutUI(core::dsl::Ui& ui, const core::dsl::Screen& screen) {
    using namespace components;
    const auto tokens = getTheme();
    const float W = screen.width;
    const float H = screen.height;

    ui.stack("root")
        .size(W, H)
        .color(tokens.background)
        .content([&] {

            // ── 主面板背景 ────────────────────────────────────────
            panel(ui, "main.bg", tokens)
                .size(W - 24.f, H - 24.f)
                .x(12.f).y(12.f)
                .radius(16.f)
                .build();

            // ── 内容列 ───────────────────────────────────────────
            ui.column("main.col")
                .size(W - 24.f, H - 24.f)
                .x(12.f).y(12.f)
                .gap(8.f)
                .padding(16.f, 16.f)
                .content([&] {

                    // 标题行
                    ui.row("hdr")
                        .width(W - 56.f).height(44.f)
                        .gap(8.f)
                        .alignItems(core::Align::CENTER)
                        .content([&] {
                            text(ui, "hdr.title", tokens)
                                .size(W - 140.f, 36.f)
                                .text("AndroidSurface EUI-NEO")
                                .fontSize(19.f)
                                .build();

                            button(ui, "btn.exit")
                                .theme(tokens, false)
                                .size(76.f, 36.f)
                                .text("退出")
                                .fontSize(14.f)
                                .radius(8.f)
                                .onClick([] { s_mainFlag = false; })
                                .build();
                        })
                        .build();

                    // 渲染信息
                    text(ui, "info", tokens)
                        .size(W - 56.f, 22.f)
                        .text("OpenGL ES 3.0  |  EUI-NEO  |  glfw_shim")
                        .fontSize(12.f)
                        .color({0.4f, 0.85f, 0.4f, 1.f})
                        .build();

                    // 主题切换
                    ui.row("theme.row")
                        .width(W - 56.f).height(40.f)
                        .gap(10.f)
                        .alignItems(core::Align::CENTER)
                        .content([&] {
                            text(ui, "theme.lbl", tokens)
                                .size(44.f, 28.f).text("主题").fontSize(14.f).build();
                            segmented(ui, "theme.seg")
                                .theme(tokens)
                                .size(W - 124.f, 36.f)
                                .items({"深色", "浅色"})
                                .selected(s_themeIdx)
                                .onChange([](int i) { s_themeIdx = i; })
                                .build();
                        })
                        .build();

                    // 防录屏
                    toggleSwitch(ui, "sw.record")
                        .theme(tokens)
                        .size(W - 56.f, 36.f)
                        .checked(permeate_record)
                        .label("防录屏")
                        .fontSize(14.f)
                        .onChange([](bool v) {
                            permeate_record = v;
                            permeate_record_ini = true;
                        })
                        .build();

                    // 坤坤页面
                    toggleSwitch(ui, "sw.kunkun")
                        .theme(tokens)
                        .size(W - 56.f, 36.f)
                        .checked(s_showAnotherWindow)
                        .label("坤坤页面")
                        .fontSize(14.f)
                        .onChange([](bool v) { s_showAnotherWindow = v; })
                        .build();

                    // 演示页面
                    toggleSwitch(ui, "sw.demo")
                        .theme(tokens)
                        .size(W - 56.f, 36.f)
                        .checked(s_showDemoWindow)
                        .label("演示页面")
                        .fontSize(14.f)
                        .onChange([](bool v) { s_showDemoWindow = v; })
                        .build();

                    // 滑动条
                    ui.row("slider.row")
                        .width(W - 56.f).height(36.f)
                        .gap(10.f)
                        .alignItems(core::Align::CENTER)
                        .content([&] {
                            text(ui, "sl.lbl", tokens)
                                .size(40.f, 28.f).text("强度").fontSize(14.f).build();
                            slider(ui, "main.sl")
                                .theme(tokens)
                                .size(W - 120.f, 32.f)
                                .value(s_sliderVal)
                                .onChange([](float v) { s_sliderVal = v; })
                                .build();
                        })
                        .build();

                    // 按钮 + 计数
                    ui.row("btn.row")
                        .width(W - 56.f).height(46.f)
                        .gap(14.f)
                        .alignItems(core::Align::CENTER)
                        .content([&] {
                            button(ui, "btn.cnt")
                                .theme(tokens, true)
                                .size(100.f, 40.f)
                                .text("点我")
                                .fontSize(15.f)
                                .radius(10.f)
                                .onClick([] { ++s_counter; })
                                .build();

                            char buf[32];
                            snprintf(buf, sizeof(buf), "计数 = %d", s_counter);
                            text(ui, "cnt.txt", tokens)
                                .size(W - 194.f, 28.f)
                                .text(buf)
                                .fontSize(15.f)
                                .build();
                        })
                        .build();
                })
                .build();

            // ── 坤坤浮层 ─────────────────────────────────────────
            if (s_showAnotherWindow) {
                const float pw = 220.f, ph = 160.f;
                panel(ui, "kk.bg", tokens)
                    .size(pw, ph)
                    .x(W * 0.5f - pw * 0.5f).y(H * 0.5f - ph * 0.5f)
                    .radius(14.f).build();
                ui.column("kk.col")
                    .size(pw, ph)
                    .x(W * 0.5f - pw * 0.5f).y(H * 0.5f - ph * 0.5f)
                    .gap(12.f)
                    .justifyContent(core::Align::CENTER)
                    .alignItems(core::Align::CENTER)
                    .content([&] {
                        text(ui, "kk.t", tokens)
                            .size(pw - 24.f, 30.f)
                            .text("另一个页面 爱坤!")
                            .fontSize(16.f)
                            .horizontalAlign(core::HorizontalAlign::Center)
                            .build();
                        button(ui, "kk.close")
                            .theme(tokens, false)
                            .size(100.f, 36.f).text("关闭").radius(8.f)
                            .onClick([] { s_showAnotherWindow = false; })
                            .build();
                    })
                    .build();
            }

            // ── 演示浮层 ─────────────────────────────────────────
            if (s_showDemoWindow) {
                const float pw = W - 40.f, ph = H - 40.f;
                panel(ui, "demo.bg", tokens)
                    .size(pw, ph).x(20.f).y(20.f).radius(14.f).build();
                ui.column("demo.col")
                    .size(pw, ph).x(20.f).y(20.f)
                    .gap(14.f)
                    .justifyContent(core::Align::CENTER)
                    .alignItems(core::Align::CENTER)
                    .content([&] {
                        text(ui, "demo.t", tokens)
                            .size(pw - 32.f, 36.f)
                            .text("EUI-NEO 演示")
                            .fontSize(22.f)
                            .horizontalAlign(core::HorizontalAlign::Center)
                            .build();

                        slider(ui, "demo.sl")
                            .theme(tokens)
                            .size(pw - 64.f, 32.f)
                            .value(s_sliderVal)
                            .onChange([](float v) { s_sliderVal = v; })
                            .build();

                        toggleSwitch(ui, "demo.sw")
                            .theme(tokens)
                            .size(pw - 64.f, 34.f)
                            .checked(s_switchVal)
                            .label("示例开关")
                            .onChange([](bool v) { s_switchVal = v; })
                            .build();

                        button(ui, "demo.close")
                            .theme(tokens, false)
                            .size(120.f, 40.f).text("关闭").radius(10.f)
                            .onClick([] { s_showDemoWindow = false; })
                            .build();
                    })
                    .build();
            }
        })
        .build();
}

bool EuiShouldRun() { return s_mainFlag; }
