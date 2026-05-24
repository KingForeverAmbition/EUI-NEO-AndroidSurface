# EUI-NEO · AndroidSurface 移植说明

基于 [EUI-NEO](https://github.com/sudoevolve/EUI-NEO) 的 Android 纯 C++ 移植版本，不依赖 Java/Kotlin，以 `ANativeWindow` 直接创建悬浮窗，通过 `glfw_shim` 桥接 EGL，让 EUI-NEO 的整套 DSL / Components 跑在 Android 上。

---

## 与原版的主要差异

### 新增文件

| 文件 | 说明 |
|------|------|
| `main.cpp` | 程序入口，替代原版的 `main.cpp`（桌面 GLFW 版） |
| `Android_EUI/eui_draw_gui.cpp` | UI 层实现，替代原 ImGui 版的 `draw_Gui.cpp` |
| `Android_EUI/android_app_surface.cpp` | `app::` 接口的 Android 端实现 |
| `glfw_shim/` | GLFW → EGL/ANativeWindow 的桥接层，让 EUI-NEO 的 GLFW 调用在 Android 上透明运行 |
| `native_surface/ANativeWindowCreator` | 纯 C++ 创建 Android 悬浮窗，无需 Java |
| `TouchHelperA` | `/dev/input` 触摸事件读取与坐标注入 |

### 替换/修改的文件

原版的 `android_app.cpp`（gallery demo）被替换为我们自己的 `android_app_surface.cpp`，入口函数 `compose()` 对接 `EuiLayoutUI()`。

Shader 预处理宏 `EUI_SHADER_PRELUDE` 由桌面的 `#version 330 core` 改为 GLES 兼容的：

```glsl
#version 300 es
precision highp float;
```

### 渲染循环

原版桌面版直接用 `glfwCreateWindow` 创建系统窗口。Android 版在此基础上封装了一层 surface 会话：

```
eui_android_main()
  └─ while (!exit)
       ├─ runWindowSession()   // 创建 GLFWwindow，持续渲染直到 surface 丢失
       ├─ wait_for_surface()   // 等待新 surface（如锁屏唤醒后）
       └─ promote_pending_window()  // 切换到新 surface，重新进入渲染循环
```

surface 丢失时会完整释放 GL 资源再重建，避免 `GLuint` 泄漏。

### 设置持久化

原版 demo 没有持久化。本版本将所有用户状态以 `key=value` 格式写入 `<files-dir>/settings.txt`，每次状态变更即写入，依赖内核页缓存，实际 I/O 频率远低于帧率。

### 触摸注入

触摸事件通过 `TouchHelperA` 从 `/dev/input` 读取，经坐标变换后通过 `eui_android_inject_touch(x, y, action)` 注入 `glfw_shim`，最终触发 EUI-NEO 的事件系统。

---

## 已知问题

### ⚠ 触摸穿透目前实现有误

当前代码中 `main.cpp` 里：

```cpp
// readOnly=false → 不 GRAB 设备 → 触摸穿透到下层 app
Touch::Init({(float)abs_ScreenX, (float)abs_ScreenY}, false);
```

注释与参数含义写反了。`readOnly=false` 实际上会 GRAB 设备，底层 app **无法**收到触摸，并非真正穿透。

正确的穿透应传 `readOnly=true`：

```cpp
Touch::Init({(float)abs_ScreenX, (float)abs_ScreenY}, true);
```

但 `readOnly=true` 时 EUI-NEO 界面是否能正常响应触摸，取决于 `glfw_shim` 侧的注入路径，目前尚未完整验证。欢迎有能力的开发者提 PR。

### 其他待修复问题

- **软键盘遮挡**：弹出软键盘时没有推起布局，输入框可能被遮住
- **横竖屏切换**：`surface` 销毁/重建流程在部分设备上有竞态，偶发黑屏
- **字体路径硬编码**：当前字体路径写死在 `main.cpp`，未做自动探测，换设备需手动改
- **帧率在动画停止后无法降频**：`isAnimating()` 返回 false 时理论上应进入 `glfwWaitEvents`，但部分场景下定时器会持续触发 recompose，导致始终以目标帧率空转
- **颜色选择器主题色更新延迟**：`g_pickedColor` 更新后需下一帧才反映到 `dslAppConfig().clearColor`，首帧有闪烁

---

## 项目结构

```
.
├── main.cpp                        # 程序入口：初始化 surface、触摸、字体，启动主循环
├── Android_EUI/
│   ├── eui_android_config.h        # GLES shader 预处理宏、全局变量声明
│   ├── eui_draw_gui.cpp            # 简版 UI（悬浮窗主界面）
│   └── android_app_surface.cpp     # 完整 demo UI（控件/图表/时钟/选择器等）
├── glfw_shim/                      # GLFW → EGL 桥接
├── native_surface/                 # ANativeWindow 悬浮窗创建
├── TouchHelperA/                   # /dev/input 触摸读取
├── app/                            # 原版 app:: 接口（dsl_app.h 等）
├── components/                     # 原版组件层（保持不动）
└── core/                           # 原版 DSL / Runtime（保持不动）
```

---

## 编译

### 环境要求

- Android NDK r25+
- CMake 3.14+
- C++17

### CMake 配置

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build -j8
```

产物为单个可执行文件，通过 `adb push` 推送到设备运行。

### 字体文件

目前字体路径硬编码，推送到设备时需要对应：

```bash
adb push JingNanJunJunTi.ttf /data/local/tmp/
adb push "Font Awesome 7 Free-Solid-900.otf" /data/local/tmp/
```

或修改 `main.cpp` 中 `eui_android_set_font_paths()` 的参数指向实际路径。

---

## 运行

```bash
adb push build/AndroidSurfaceEUI /data/local/tmp/
adb shell chmod +x /data/local/tmp/AndroidSurfaceEUI
adb shell /data/local/tmp/AndroidSurfaceEUI
```

需要 root 权限（`/dev/input` 读取依赖 root）。

---

## 自定义 UI

修改 `Android_EUI/eui_draw_gui.cpp` 中的 `EuiLayoutUI()` 函数，或直接替换 `android_app_surface.cpp`。组件用法与原版一致，参考 [原版组件文档](https://github.com/sudoevolve/EUI-NEO/blob/main/docs/组件.md)。

简单示例：

```cpp
void EuiLayoutUI(core::dsl::Ui& ui, const core::dsl::Screen& screen) {
    ui.stack("root")
        .size(screen.width, screen.height)
        .content([&] {
            components::button(ui, "btn.test")
                .size(200.f, 50.f)
                .text("点我")
                .onClick([] { /* 你的逻辑 */ })
                .build();
        })
        .build();
}
```

---

## 贡献

欢迎提 issue 或 PR，尤其是以下方向：

- 触摸穿透的正确实现
- 软键盘推起布局
- 横竖屏切换稳定性
- 字体路径自动探测

原版 EUI-NEO 的 core / components 层我们尽量不动，以便跟进原版更新。

---

## License

遵循原版 [Apache 2.0](https://github.com/sudoevolve/EUI-NEO/blob/main/LICENSE)。
