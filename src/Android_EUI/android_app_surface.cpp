#include "Android_EUI/eui_draw.h"
#include "app/dsl_app.h"
#include "components/components.h"

extern "C" const char *eui_android_text_font_path(void);
extern "C" const char *eui_android_icon_font_path(void);
extern "C" const char *eui_android_files_dir(void);

namespace app
{

    const DslAppConfig &dslAppConfig()
    {
        static DslAppConfig config = DslAppConfig{}
                                         .title("AndroidSurface EUI-NEO")
                                         .pageId("main")
                                         .clearColor({0.10f, 0.10f, 0.12f, 1.0f})
                                         .windowSize(0, 0)
                                         .fps(120.0)
                                         .textFont(eui_android_text_font_path() ? eui_android_text_font_path() : "")
                                         .iconFont(eui_android_icon_font_path() ? eui_android_icon_font_path() : "")
                                         .showFrameCountInTitle(false);
        return config;
    }

    void compose(core::dsl::Ui &ui, const core::dsl::Screen &screen)
    {
        EuiLayoutUI(ui, screen);
    }

} // namespace app