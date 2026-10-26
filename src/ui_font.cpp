#include "ui_font.h"
#include "progress_glyphs.h"
#include "world_layout.h"
#include "prts_narrator.h"

#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>
#include <cstring>

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
    "编队预览仅影响输出擅长持续与幕压制范围查看详情位置"
    "红门敌方入口增援场上待入场结束"
    "离开交叉路线射击拆雷或离开标记可拆除击碎冰晶或离开路线连续跳跃或远离跳跃或绕后"
    "冰晶锁定散射击碎或换位冰棘沿地面推进三次震地范围逐次扩大跳跃避开或远离标记水平贯穿"
    "镜影交刃烟幕伏杀定时空爆遥控雷网棱镜回廊大地战鼓贯阵长枪两侧幻影提前锁定交叉路线换位或拦截飞刀"
    "同时投刀残影消退留下反击时间三个落点先攻击中央再攻击两侧地面幻影逐个出刀离开红色标记即可避开"
    "高低两层爆点已锁定留意各自倒计时高位爆炸留在地面低位跳跃雷网每枚两点伤害可拆除地雷依次引爆射击拆雷或移出范围"
    "空中冰晶蓄能三条蓝线锁定散射方向依次发射三路冰刃从霜星前方向玩家逐段推进升起每次范围更大只标下一次对应震地退到标记外"
    "红色光带锁定水平攻击路线长枪沿固定方向贯穿出枪和身体动作同步"
    "系统配置音量与操作设置图片创建角色干员档案战术配置行动准备记录检索征"
    "删除自定义角色只能本地创建或导入删除后将退出编队文件保留在本地回收区失败请检查目录权限取消";

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

UiFont::UiFont(bool deferFontLoad) : font_(GetFontDefault()) {
    IndexGlyphs();
    if (!deferFontLoad) SetAdditionalText("");
}

void UiFont::SetAdditionalText(const std::string& text) {
    const char* fontPath = FindFontPath();
    if (fontPath == nullptr) {
        return;
    }

    int glyphCount = 0;
    const std::string glyphs = std::string(kUiGlyphs) + kProgressGlyphs + WorldLayout::Glyphs() + PrtsNarrator::GlyphText() + text;
    int* codepoints = LoadCodepoints(glyphs.c_str(), &glyphCount);
    std::vector<int> unique(codepoints, codepoints + glyphCount);
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    UnloadCodepoints(codepoints);
    if (ownsFont_ && unique == loadedCodepoints_) return;
    const Font loadedFont =
        LoadFontEx(fontPath, kFontAtlasSize, unique.data(), static_cast<int>(unique.size()));

    if (loadedFont.texture.id != 0 &&
        loadedFont.texture.id != GetFontDefault().texture.id) {
        if (ownsFont_) UnloadFont(font_);
        font_ = loadedFont;
        ownsFont_ = true;
        GenTextureMipmaps(&font_.texture);
        SetTextureFilter(font_.texture, TEXTURE_FILTER_TRILINEAR);
        loadedCodepoints_ = std::move(unique);
        IndexGlyphs();
    }
}

UiFont::~UiFont() {
    if (ownsFont_) {
        UnloadFont(font_);
    }
}

void UiFont::Draw(const char* text, float x, float y, float size, Color color) const {
    if (text == nullptr || font_.glyphCount == 0) return;
    // raylib's line spacing is global and has no getter. Delegate multiline
    // text to it so SetTextLineSpacing continues to work exactly as before.
    if (std::strchr(text, '\n')) {
        DrawTextEx(font_, text, {x, y}, size, 1.0F, color);
        return;
    }
    const float scale = size / font_.baseSize;
    float offset = 0;
    for (int i = 0, length = TextLength(text); i < length;) {
        int bytes = 0;
        const int codepoint = GetCodepointNext(text + i, &bytes);
        const int index = GlyphIndex(codepoint);
        const auto& glyph = font_.glyphs[index];
        const auto& rec = font_.recs[index];
        if (codepoint != ' ' && codepoint != '\t') {
            // Same rectangles and arithmetic as raylib 5.5 DrawTextCodepoint;
            // replace only its two linear glyph searches per character.
            const Rectangle destination{
                x + offset + glyph.offsetX * scale - float(font_.glyphPadding) * scale,
                y + glyph.offsetY * scale - float(font_.glyphPadding) * scale,
                (rec.width + 2.0F * font_.glyphPadding) * scale,
                (rec.height + 2.0F * font_.glyphPadding) * scale};
            const Rectangle source{rec.x - float(font_.glyphPadding), rec.y - float(font_.glyphPadding),
                rec.width + 2.0F * font_.glyphPadding, rec.height + 2.0F * font_.glyphPadding};
            DrawTexturePro(font_.texture, source, destination, {}, 0, color);
        }
        if (glyph.advanceX == 0) offset += rec.width * scale + 1.0F;
        else offset += float(glyph.advanceX) * scale + 1.0F;
        i += bytes;
    }
}

float UiFont::Measure(const char* text, float size) const {
    if (text == nullptr || text[0] == '\0' || font_.texture.id == 0) return 0;
    int count = 0, longestCount = 0;
    float width = 0, longestWidth = 0;
    for (int i = 0, length = TextLength(text); i < length;) {
        ++count;
        int bytes = 0;
        const int codepoint = GetCodepointNext(text + i, &bytes);
        const int index = GlyphIndex(codepoint);
        i += bytes;
        if (codepoint != '\n') {
            const auto& glyph = font_.glyphs[index];
            if (glyph.advanceX > 0) width += glyph.advanceX;
            else width += font_.recs[index].width + glyph.offsetX;
        } else {
            longestWidth = std::max(longestWidth, width);
            count = 0;
            width = 0;
        }
        longestCount = std::max(longestCount, count);
    }
    longestWidth = std::max(longestWidth, width);
    return longestWidth * (size / float(font_.baseSize)) + float(longestCount - 1);
}

bool UiFont::SupportsText(const std::string& text) const {
    for (int i = 0, length = TextLength(text.c_str()); i < length;) {
        int bytes = 0;
        const int codepoint = GetCodepointNext(text.c_str() + i, &bytes);
        if (!glyphIndices_.contains(codepoint)) return false;
        i += bytes;
    }
    return true;
}

void UiFont::IndexGlyphs() {
    glyphIndices_.clear();
    glyphIndices_.reserve(font_.glyphCount);
    fallbackGlyph_ = 0;
    for (int i = 0; i < font_.glyphCount; ++i) {
        glyphIndices_.emplace(font_.glyphs[i].value, i);
        if (font_.glyphs[i].value == '?') fallbackGlyph_ = i;
    }
}

int UiFont::GlyphIndex(int codepoint) const {
    const auto found = glyphIndices_.find(codepoint);
    return found != glyphIndices_.end() ? found->second : fallbackGlyph_;
}
