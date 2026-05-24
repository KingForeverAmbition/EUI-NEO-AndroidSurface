#include "TouchHelperA.h"
#include "native_surface/ANativeWindowCreator.h"

#include <android/log.h>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "EuiMain", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "EuiMain", __VA_ARGS__)

extern "C"
{
    void eui_android_set_native_window(ANativeWindow *window, int width, int height);
    void eui_android_inject_touch(float x, float y, int action);
    void eui_android_set_font_paths(const char *textFont, const char *iconFont, const char *filesDir);
    int eui_android_main(void);
}

bool permeate_record = false;
bool permeate_record_ini = false;
ANativeWindow *g_window = nullptr;
android::ANativeWindowCreator::DisplayInfo g_displayInfo = {};
int abs_ScreenX = 0, abs_ScreenY = 0;
int native_window_screen_x = 0, native_window_screen_y = 0;

static int s_screenW = 0;
static int s_screenH = 0;
static bool s_prevDown = false;

static void onTouchUpdate(std::vector<Touch::Device> *devices)
{
    for (auto &dev : *devices)
    {
        for (auto &finger : dev.Finger)
        {
            if (finger.isDown || (!finger.isDown && s_prevDown))
            {
                auto pos = Touch::Touch2Screen(finger.pos);
                // action: 0=抬起 1=按下 2=移动
                int action = finger.isDown ? (s_prevDown ? 2 : 1) : 0;
                eui_android_inject_touch(pos.x, pos.y, action);
                s_prevDown = finger.isDown;
                return;
            }
        }
    }
    // 所有手指都离开，补一个抬起
    if (s_prevDown)
    {
        eui_android_inject_touch(-1.f, -1.f, 0);
        s_prevDown = false;
    }
}

extern "C" void eui_android_set_touch_passthrough(bool enable)
{
    Touch::Close();
    Touch::Init({(float)abs_ScreenX, (float)abs_ScreenY}, enable);
    Touch::setOrientation(g_displayInfo.orientation);
    Touch::SetCallBack(onTouchUpdate);
}

int main(int /*argc*/, char * /*argv*/[])
{
    g_displayInfo = android::ANativeWindowCreator::GetDisplayInfo();

    s_screenW = g_displayInfo.width;
    s_screenH = g_displayInfo.height;

    abs_ScreenX = (s_screenW > s_screenH) ? s_screenW : s_screenH;
    abs_ScreenY = (s_screenW < s_screenH) ? s_screenW : s_screenH;

    native_window_screen_x = s_screenW;
    native_window_screen_y = s_screenH;

    LOGI("screen:%dx%d abs:%dx%d surface:%dx%d",
         s_screenW,
         s_screenH,
         abs_ScreenX,
         abs_ScreenY,
         native_window_screen_x,
         native_window_screen_y);

    g_window = android::ANativeWindowCreator::Create(
        "EuiAndroid",
        native_window_screen_x,
        native_window_screen_y,
        permeate_record);

    if (!g_window)
    {
        LOGE("Failed to create ANativeWindow");
        return -1;
    }

    eui_android_set_font_paths(
        "/data/local/tmp/JingNanJunJunTi-JinNanJunJunTi-Bold-2.ttf",
        "/data/local/tmp/Font Awesome 7 Free-Solid-900.otf",
        "/data/local/tmp");

    eui_android_set_native_window(g_window, native_window_screen_x, native_window_screen_y);

    // readOnly=false 触摸不穿透 true则是穿透(但是没法触控菜单了 -_-)
    Touch::Init({(float)abs_ScreenX, (float)abs_ScreenY}, false);
    Touch::setOrientation(g_displayInfo.orientation);
    Touch::SetCallBack(onTouchUpdate);

    LOGI("Entering eui_android_main");
    int ret = eui_android_main();

    Touch::Close();
    android::ANativeWindowCreator::Cleanup();
    android::ANativeWindowCreator::Destroy(g_window);
    return ret;
}