#include <glad/glad.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include "app/app.h"
#include "core/async.h"
#include "core/dsl_runtime.h"
#include "core/event.h"
#include "core/network.h"
#include "core/platform.h"
#include "core/text.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>
#include <vector>

#include <android/log.h>

extern "C" bool eui_android_exit_requested(void);
extern "C" bool eui_android_surface_lost(void);
extern "C" void eui_android_release_egl(GLFWwindow *w);
extern "C" bool eui_android_wait_for_surface(void);
extern "C" bool eui_android_promote_pending_window(void);
extern "C" void eui_android_poll_ime_frame(void);

struct WindowState
{
    bool needsRender = true;
    int renderedFrames = 0;
    double lastTitleUpdate = 0.0;
    double nextFrameTime = 0.0;
    double frameInterval = 1.0 / 60.0;
    double lastFrameRateLimit = 0.0;
    double lastRefreshRateUpdate = 0.0;
};

static float getDpiScale(GLFWwindow *window)
{
    float sx = 1.0f, sy = 1.0f;
    glfwGetWindowContentScale(window, &sx, &sy);
    return (sx + sy) * 0.5f;
}

static float getPointerScale(GLFWwindow *window)
{
    int ww = 0, wh = 0, fw = 0, fh = 0;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fw, &fh);
    if (ww <= 0 || wh <= 0) return 1.0f;
    return ((float)fw / (float)ww + (float)fh / (float)wh) * 0.5f;
}

static GLFWmonitor *getWindowMonitor(GLFWwindow *w)
{
    if (GLFWmonitor *m = glfwGetWindowMonitor(w)) return m;
    return glfwGetPrimaryMonitor();
}

static double getWindowRefreshRate(GLFWwindow *w)
{
    GLFWmonitor *m = getWindowMonitor(w);
    const GLFWvidmode *mode = m ? glfwGetVideoMode(m) : nullptr;
    return (mode && mode->refreshRate > 0) ? (double)mode->refreshRate : 60.0;
}

static void updateFrameInterval(GLFWwindow *w, WindowState &ws, double now, bool force = false)
{
    double limit = app::frameRateLimit();
    if (!force && limit == ws.lastFrameRateLimit && now - ws.lastRefreshRateUpdate < 0.5) return;
    double rr = std::clamp(getWindowRefreshRate(w), 30.0, 500.0);
    if (limit > 0.0) rr = std::min(rr, limit);
    ws.frameInterval = 1.0 / std::max(1.0, rr);
    ws.lastFrameRateLimit = limit;
    ws.lastRefreshRateUpdate = now;
}

// 返回 true 表示外层循环继续，false 表示直接退出
static bool runWindowSession()
{
    GLFWwindow *window = glfwCreateWindow(1080, 1920, "EUI-NEO", nullptr, nullptr);
    if (!window)
    {
        __android_log_print(ANDROID_LOG_ERROR, "EUI", "glfwCreateWindow failed");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return !eui_android_exit_requested();
    }

    app::initialize(window);

    WindowState ws;
    double lastFrameTime = glfwGetTime();
    ws.nextFrameTime = lastFrameTime;
    ws.needsRender = true;
    int fbW = 0, fbH = 0;

    // 内层循环
    while (!eui_android_exit_requested() && !eui_android_surface_lost())
    {
        glfwPollEvents();
        if (eui_android_surface_lost() || eui_android_exit_requested()) break;

        double currentTime = glfwGetTime();
        updateFrameInterval(window, ws, currentTime);
        float deltaSeconds = (float)(currentTime - lastFrameTime);
        lastFrameTime = currentTime;

        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW <= 0 || fbH <= 0)
        {
            ws.needsRender = true;
            glfwWaitEvents();
            ws.nextFrameTime = glfwGetTime();
            lastFrameTime = ws.nextFrameTime;
            continue;
        }

        float dpiScale = getDpiScale(window);
        float ptrScale = getPointerScale(window);
        bool externalReady = core::network::consumeAnyTextReady() || core::async::dispatchReady();

        if (app::update(window, deltaSeconds, fbW, fbH, dpiScale, ptrScale, externalReady, true))
        {
            ws.needsRender = true;
        }

        eui_android_poll_ime_frame();

        if (ws.needsRender)
        {
            app::render(fbW, fbH, dpiScale);
            glfwSwapBuffers(window);
            ws.needsRender = false;
        }

        if (app::isAnimating())
        {
            double now = glfwGetTime();
            ws.nextFrameTime += ws.frameInterval;
            // 如果目标帧时间已经落后或偏差超过两帧，直接重置到下一帧
            if (ws.nextFrameTime <= now || ws.nextFrameTime > now + ws.frameInterval * 2.0)
                ws.nextFrameTime = now + ws.frameInterval;
            glfwPollEvents();
        }
        else
        {
            glfwWaitEvents();
            ws.nextFrameTime = glfwGetTime();
        }
    }

    app::releaseGraphicsResources();
    core::releaseInputQueue(window);
    eui_android_release_egl(window);

    return !eui_android_exit_requested();
}

extern "C" int eui_android_main(void)
{

    while (!eui_android_exit_requested())
    {
        if (!runWindowSession()) break;

        if (!eui_android_wait_for_surface()) break;
        if (!eui_android_promote_pending_window())
        {
            continue;
        }
    }

    app::shutdown();
    glfwTerminate();
    return 0;
}