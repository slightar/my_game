#include "ui_font.h"
#include "progress_glyphs.h"

#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

namespace {

constexpr const char* kUiGlyphs =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    " /+-.:?%<>=_[]()!#,;|&*'\"·（）↑↓←→；……，：、。《》？！"
    "按案败避标镖并部查朝车冲出弹当档岛德定动端段二发返方放飞"
    "锋干高格关换挥回或击继键交角接近进君卡开砍看可空量罗名命"
    "目能前枪确认任容入色闪尚射生失使始弑署术速锁天跳停统突退"
    "卫未务袭戏系匣向新行续选页一移已游员暂择斩战者中终重主住作"
    "扫模式过载冷却就绪效德克萨斯剑气阵雨连绵跳跃攻击技拖入角色导入刷新成功本地保存失败配置文件狙远程"
    "队数切敬期待战阵操未斗"
    // Map, archive, equipment and ending placeholder glyphs.
    "探索地图路线冷色暖色记忆线索完成可进入隐藏终局普通初始教学指引任务终点废弃终端密室真实"
    "档案关键剧情道具已收录条目尚未解锁记录来源装备任意放入栏当前拥有藏品回响协议卸下"
    "行动准备节点详情开发占位关卡战斗逻辑待接入拒绝遵循击败最终确认推进结局退出此战立即失效"
    "保持有效所有已装备原石信息特蕾西娅化为留下储存在游戏中的等待与你重逢"
    "损坏碎片遗失利首次重置痕迹火力维生增益显示附近死亡后保留一条"
    "系统建议既定路线强直接造成伤害提升获得额外生命照亮汇聚探索发现效率低语"
    "当前继续退出分类选择返回主页面左右上下切换按键确定指令归档"
    // Character lineup page.
    "编队预览仅影响输出擅长持续与幕压制范围查看详情位置";

constexpr int kFontAtlasSize = 128;

const char* FindFontPath() {
    static const std::string bundledPath =
        (std::filesystem::path(GetApplicationDirectory()) /
         "assets/fonts/ui.ttf").string();
    if (FileExists(bundledPath.c_str())) {
        return bundledPath.c_str();
    }
#ifdef _WIN32
    if (FileExists("C:/Windows/Fonts/msyh.ttf")) {
        return "C:/Windows/Fonts/msyh.ttf";
    }
    if (FileExists("C:/Windows/Fonts/simhei.ttf")) {
        return "C:/Windows/Fonts/simhei.ttf";
    }
    // stb_truetype uses a variable font's default axis, which can be too thin at HUD sizes.
    if (FileExists("C:/Windows/Fonts/NotoSansSC-VF.ttf")) {
        return "C:/Windows/Fonts/NotoSansSC-VF.ttf";
    }
#endif
    return nullptr;
}

}  // namespace

UiFont::UiFont() : font_(GetFontDefault()) {
    SetAdditionalText("");
}

void UiFont::SetAdditionalText(const std::string& text) {
    const char* fontPath = FindFontPath();
    if (fontPath == nullptr) {
        return;
    }

    int glyphCount = 0;
    const std::string glyphs = std::string(kUiGlyphs) + kProgressGlyphs + text;
    int* codepoints = LoadCodepoints(glyphs.c_str(), &glyphCount);
    std::vector<int> unique(codepoints, codepoints + glyphCount);
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    const Font loadedFont =
        LoadFontEx(fontPath, kFontAtlasSize, unique.data(), static_cast<int>(unique.size()));
    UnloadCodepoints(codepoints);

    if (loadedFont.texture.id != 0 &&
        loadedFont.texture.id != GetFontDefault().texture.id) {
        if (ownsFont_) UnloadFont(font_);
        font_ = loadedFont;
        ownsFont_ = true;
        GenTextureMipmaps(&font_.texture);
        SetTextureFilter(font_.texture, TEXTURE_FILTER_TRILINEAR);
    }
}

UiFont::~UiFont() {
    if (ownsFont_) {
        UnloadFont(font_);
    }
}

void UiFont::Draw(const char* text, float x, float y, float size, Color color) const {
    DrawTextEx(font_, text, {x, y}, size, 1.0F, color);
}

float UiFont::Measure(const char* text, float size) const {
    return MeasureTextEx(font_, text, size, 1.0F).x;
}

bool UiFont::SupportsText(const std::string& text) const {
    int count = 0;
    int* codepoints = LoadCodepoints(text.c_str(), &count);
    bool supported = true;
    for (int i = 0; i < count; ++i) {
        const int glyph = GetGlyphIndex(font_, codepoints[i]);
        if (glyph < 0 || glyph >= font_.glyphCount ||
            font_.glyphs[glyph].value != codepoints[i]) {
            supported = false;
            break;
        }
    }
    UnloadCodepoints(codepoints);
    return supported;
}
