# EUI-NEO · AndroidSurface 移植说明

基于 [EUI-NEO](https://github.com/sudoevolve/EUI-NEO) 的 Android 纯 C++ 移植版本，不依赖 Java/Kotlin，以 `ANativeWindow` 直接创建悬浮窗，通过 `glfw_shim` 桥接 EGL，让 EUI-NEO 的整套 DSL / Components 跑在 Android 上。

## 预览

| 选择器页 | 图表页 |
|:---:|:---:|
| ![选择器](docs/pic/preview_pickers.jpg) | ![图表](docs/pic/preview_charts.jpg) |

---

---

## 与原版的主要差异

### 新增文件

| 文件 | 说明 |
|------|------|
| `src/main.cpp` | 程序入口，替代原版桌面 GLFW 的 `main.cpp` |
| `src/Android_EUI/android_main.cpp` | EUI-NEO 主循环适配，管理 surface 会话生命周期 |
| `src/Android_EUI/android_app.cpp` | UI 层实现，替代原 ImGui 版的 `draw_Gui.cpp` |
| `src/glfw_shim/glfw_shim.cpp` | GLFW → EGL/ANativeWindow 桥接，让 EUI-NEO 的 GLFW 调用在 Android 上透明运行 |
| `src/Android_touch/TouchHelperA.cpp` | `/dev/input` 触摸事件读取与坐标注入 |
| `include/native_surface/` | 纯 C++ 创建 Android 悬浮窗，无需 Java |

### 替换/修改的内容

原版的 gallery demo 入口被替换为我们自己的 `android_app.cpp`，`compose()` 对接 `EuiLayoutUI()`。

Shader 预处理宏 `EUI_SHADER_PRELUDE` 由桌面的 `#version 330 core` 改为 GLES 兼容的：

```glsl
#version 300 es
precision highp float;
```

### 渲染循环

原版直接用 `glfwCreateWindow` 创建系统窗口。Android 版封装了一层 surface 会话，处理锁屏、后台切换等场景下 surface 反复销毁重建的问题：

```
eui_android_main()
  └─ while (!exit)
       ├─ runWindowSession()         // 创建 GLFWwindow，持续渲染直到 surface 丢失
       ├─ wait_for_surface()         // 等待新 surface（锁屏唤醒等）
       └─ promote_pending_window()   // 接管新 surface，重新进入渲染循环
```

surface 丢失时会完整释放 GL 资源再重建，避免 `GLuint` 泄漏。

### 设置持久化

原版 demo 没有状态保存。本版本将所有用户设置以 `key=value` 格式写入 `<files-dir>/settings.txt`，每次状态变更即写入，依赖内核页缓存，实际不会每帧落盘。

### 触摸注入

触摸事件通过 `TouchHelperA` 从 `/dev/input` 读取，经坐标变换后通过 `eui_android_inject_touch(x, y, action)` 注入 `glfw_shim`，最终触发 EUI-NEO 的事件系统。

---

## 已知问题

### ⚠ 触摸穿透与菜单交互互斥

`TouchHelperA` 的 `readOnly` 参数目前两种取值都不完整：

| 模式 | 穿透到底层 app | EUI-NEO 菜单可交互 |
|------|:-:|:-:|
| `readOnly=true` | ✅ | ❌ |
| `readOnly=false` | ❌ | ✅ |

`readOnly=true` 时 TouchHelperA 不 GRAB 设备，事件由系统直接派发给底层 app，但 `glfw_shim` 的注入路径同时失效，EUI-NEO 菜单收不到触摸。

理想方案是在不 GRAB 设备的前提下，把触摸坐标通过独立通道注入 `glfw_shim`，两条路并行。目前没有完整实现，欢迎提 PR。

### ⚠ 软键盘无法弹出

`glfw_shim` 层没有实现 Android IME 的调用路径，`eui_android_poll_ime_frame()` 目前是空壳，点击输入框不会唤起系统软键盘。中文输入等依赖 IME 的功能完全不可用。

### 其他待修复

- **横竖屏切换**：surface 销毁/重建在部分设备上有竞态，偶发黑屏
- **字体路径硬编码**：路径写死在 `main.cpp`，换设备需手动改
- **动画停止后帧率不降**：定时器持续触发 recompose，`isAnimating()` 返回 false 后仍以目标帧率空转
- **颜色选择器首帧闪烁**：`g_pickedColor` 更新后要下一帧才反映到 `dslAppConfig().clearColor`

---

## 项目结构

```
.
├── Android.mk                          # NDK 构建配置
├── src/
│   ├── main.cpp                        # 入口：初始化 surface、触摸、字体，启动主循环
│   ├── glfw_shim/
│   │   └── glfw_shim.cpp               # GLFW → EGL 桥接
│   ├── Android_EUI/
│   │   ├── android_main.cpp            # EUI-NEO 主循环 / surface 会话管理
│   │   └── android_app.cpp             # UI 层（控件/图表/时钟/选择器等）
│   └── Android_touch/
│       └── TouchHelperA.cpp            # /dev/input 触摸读取
├── include/
│   ├── Android_EUI/
│   │   └── eui_android_config.h        # GLES shader 宏、全局变量声明
│   ├── Android_touch/                  # TouchHelperA 头文件
│   └── native_surface/                 # ANativeWindow 悬浮窗创建
├── core/                               # 原版 DSL / Runtime（尽量不动）
├── 3rd/                                # 第三方单文件依赖
└── include/                            # glad_gles 等
```

---

## 编译

### 环境要求

- Android NDK r21+
- `ndk-build` 工具链

### 构建

```bash
# 在项目根目录下
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk

# 指定 ABI（默认构建所有 ABI）
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk APP_ABI=arm64-v8a
```

产物输出到 `libs/arm64-v8a/AndroidSurfaceEUI`。

### 字体文件

字体路径目前写死，推送到设备时路径需对应：

```bash
adb push JingNanJunJunTi.ttf /data/local/tmp/
adb push "Font Awesome 7 Free-Solid-900.otf" /data/local/tmp/
```

或修改 `src/main.cpp` 中 `eui_android_set_font_paths()` 的参数。

---

## 运行

```bash
adb push libs/arm64-v8a/AndroidSurfaceEUI /data/local/tmp/
adb shell chmod +x /data/local/tmp/AndroidSurfaceEUI
adb shell /data/local/tmp/AndroidSurfaceEUI
```

> 需要 root 权限，`/dev/input` 读取依赖 root。

---

## 自定义 UI

修改 `src/Android_EUI/android_app.cpp` 中的 `EuiLayoutUI()` 函数即可。组件用法与原版完全一致，参考 [原版组件文档](https://github.com/sudoevolve/EUI-NEO/blob/main/docs/组件.md)。

```cpp
void EuiLayoutUI(core::dsl::Ui& ui, const core::dsl::Screen& screen)
{
    ui.stack("root")
        .size(screen.width, screen.height)
        .content([&] {
            components::button(ui, "btn.hello")
                .size(200.f, 50.f)
                .text("点我")
                .onClick([] { /* 业务逻辑 */ })
                .build();
        })
        .build();
}
```

---

## 贡献

欢迎提 issue 或 PR，当前最需要解决的：

- 触摸穿透与菜单交互并存（`readOnly=true` 时独立注入通道）
- Android IME 调用路径，让软键盘能弹出
- 横竖屏切换稳定性
- 字体路径自动探测

`core/` 和原版 `components/` 层尽量不动，方便后续跟进原版更新。

---

## License

遵循原版 [Apache 2.0](https://github.com/sudoevolve/EUI-NEO/blob/main/LICENSE)。
