#include "character_creator.h"
#include "character_art.h"
#include "character_image.h"
#include "character_repository.h"
#include "file_path.h"
#include "terminal_ui.h"
#include "ui_font.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace {
using namespace TerminalUi;
constexpr Rectangle fields[] = {{190, 146, 530, 40}, {190, 207, 530, 40}, {190, 268, 530, 40},
    {190, 329, 530, 40}, {190, 390, 530, 40}};
constexpr Rectangle choose{58, 464, 204, 46}, generate{278, 464, 218, 46}, local{512, 464, 208, 46};
constexpr Rectangle keying{800, 432, 207, 42}, flip{1020, 432, 196, 42}, save{800, 574, 416, 52};
constexpr Rectangle smartToggle{514,112,230,25};
std::string Token() { return std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()); }
std::string Utf8(const std::filesystem::path& file) {
    const auto s = file.u8string(); return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}
void PopUtf8(std::string& s) {
    if (s.empty()) return;
    auto i = s.size()-1; while (i && (static_cast<unsigned char>(s[i]) & 0xc0) == 0x80) --i;
    s.resize(i);
}
void AppendText(std::string& target, const std::string& value, std::size_t limit) {
    for (std::size_t i = 0; i < value.size();) {
        const auto c = static_cast<unsigned char>(value[i]);
        const std::size_t count = c < 0x80 ? 1 : (c & 0xe0) == 0xc0 ? 2 : (c & 0xf0) == 0xe0 ? 3 : 4;
        if (i + count > value.size() || target.size() + count > limit) break;
        if (c >= 32 && c != 127) target.append(value, i, count);
        i += count;
    }
}
std::string Png(const Image& image) {
    int size = 0; auto* bytes = ExportImageToMemory(image, ".png", &size);
    if (!bytes || size <= 0) { if (bytes) MemFree(bytes); throw std::runtime_error("无法编码角色图片"); }
    std::string result(reinterpret_cast<char*>(bytes), size); MemFree(bytes); return result;
}
void Write(const std::filesystem::path& file, const std::string& data) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc); out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out.flush(); if (!out) throw std::runtime_error("无法写入角色素材，请检查目录权限");
}
void Checker(Rectangle r) {
    for (int y = 0; y < r.height; y += 16) for (int x = 0; x < r.width; x += 16)
        DrawRectangleRec({r.x+x, r.y+y, std::min(16.0F,r.width-x), std::min(16.0F,r.height-y)},
            ((x/16+y/16)%2) ? Color{69,77,82,255} : Color{57,64,69,255});
}
}

CharacterCreator::CharacterCreator(std::filesystem::path appDirectory) : app_(std::move(appDirectory)) {
    try { settings_ = CharacterGenerationSettings::Load(app_/"save"/"character_generation.json"); }
    catch (...) { status_ = "生成设置读取失败，已恢复默认 API Host 和模型"; }
    try {key_=LoadCharacterApiKey(app_/"save"/"character_api.local.json");localKey_=!key_.empty();}
    catch(const std::exception& e){status_=e.what();}
    stagingFile_ = app_/"save"/"character-drafts"/Token()/"character.json";
}
CharacterCreator::~CharacterCreator() {
    job_.Cancel();
    if (reference_.data) UnloadImage(reference_);
    if (generated_.data) UnloadImage(generated_);
    if (referenceTexture_.id) UnloadTexture(referenceTexture_);
    if (active_) SetCharacterTextInput(false);
    std::fill(key_.begin(), key_.end(), '\0');
}
void CharacterCreator::Open(CharacterArt& art) {
    active_ = true; focus_ = -1;
    if (generated_.data) try { MakePreview(art); } catch (const std::exception& e) { InvalidatePreview(); status_=e.what(); }
}
void CharacterCreator::Close() { active_ = false; focus_ = -1; SetCharacterTextInput(false); }
void CharacterCreator::InvalidatePreview() { hasPreview_ = false; animator_ = {}; previewTime_ = 0; }

void CharacterCreator::SetReference(const std::filesystem::path& file) {
    if (job_.Busy()) throw std::runtime_error("请等待当前请求结束后更换参考图");
    Image image = LoadCharacterImage(file);
    if (!image.data) throw std::runtime_error("无法读取参考图：请选择 PNG、JPG 或 BMP，边长不超过 8192");
    const int largest = std::max(image.width, image.height);
    if (largest > 1536) ImageResize(&image, std::max(1,image.width*1536/largest), std::max(1,image.height*1536/largest));
    std::string png;
    try { png = Png(image); if (png.size() > 10*1024*1024) throw std::runtime_error("参考图压缩后仍超过 10 MiB"); }
    catch (...) { UnloadImage(image); throw; }
    const auto texture = LoadTextureFromImage(image);
    if (!texture.id) { UnloadImage(image); throw std::runtime_error("参考图预览加载失败"); }
    if (reference_.data) UnloadImage(reference_);
    if (referenceTexture_.id) UnloadTexture(referenceTexture_);
    reference_ = image; referenceTexture_ = texture;
    SetTextureFilter(referenceTexture_, TEXTURE_FILTER_BILINEAR);
    referenceFile_ = file; sourcePng_ = std::move(png);
    analysisDescription_.clear();
    if (name_ == "新角色" || name_.empty()) { name_.clear(); AppendText(name_, Utf8(file.stem()), 120); }
    if (generated_.data) { UnloadImage(generated_); generated_ = {}; }
    InvalidatePreview();
    status_ = "参考图已载入；点击生成会上传该图并按百炼账户规则计费";
}
void CharacterCreator::Start() {
    if (sourcePng_.empty()) throw std::runtime_error("请先选择或拖入参考图");
    settings_.Normalize();settings_.Save(app_/"save"/"character_generation.json");
    autoName_=name_.empty()||name_=="新角色"||name_==Utf8(referenceFile_.stem());
    generationTime_=0;
    job_.Start(settings_, key_, sourcePng_);
    status_ = "正在生成小人，请稍候；可返回菜单，完成后再进入查看";
}
void CharacterCreator::MakePreview(CharacterArt& art) {
    if (!generated_.data) throw std::runtime_error("请先生成小人或使用原图");
    Image sprite = PrepareCharacterSprite(generated_, removeGreen_);
    transparentPreview_ = false;
    auto* colors = LoadImageColors(sprite);
    // Ignore the padding added by PrepareCharacterSprite: an opaque rectangular
    // background must not be presented as a successfully extracted silhouette.
    if (colors) {
        for (int y=6;y<sprite.height-6 && !transparentPreview_;++y)
            for (int x=6;x<sprite.width-6;++x) if (colors[y*sprite.width+x].a<16) { transparentPreview_=true; break; }
        UnloadImageColors(colors);
    }
    std::string png;
    try { png = Png(sprite); } catch (...) { UnloadImage(sprite); throw; }
    UnloadImage(sprite);
    std::filesystem::create_directories(stagingFile_.parent_path());
    Write(stagingFile_.parent_path()/"reference.png", sourcePng_);
    Write(stagingFile_.parent_path()/"sprite.png", png);
    auto next = MakeGeneratedCharacter("_creator_preview", name_.empty() ? "新角色" : name_, combat_);
    next.assets.portrait = "reference.png"; next.assets.sprite = "sprite.png";
    next.assets.height = 190;
    art.RegisterCharacter(next, stagingFile_.parent_path());
    preview_ = std::move(next); hasPreview_ = true; animator_ = {}; previewTime_ = 0;
}
void CharacterCreator::Save() {
    if (!hasPreview_) throw std::runtime_error("请先完成小人预览");
    auto c = MakeGeneratedCharacter("hero_"+Token(), name_.empty() ? "新角色" : name_, combat_);
    if(!analysisDescription_.empty())c.description=analysisDescription_;
    c.assets.portrait = "reference.png"; c.assets.sprite = "sprite.png";
    Write(stagingFile_, c.ToJson());
    LocalCharacterRepository repository(app_/"characters");
    repository.Import(stagingFile_); // Repository validates and atomically commits the complete asset set.
    status_ = "角色已保存，可在角色编队中选择出战"; Close();
}

bool CharacterCreator::Update(float dt, CharacterArt& art, const std::function<void()>& redraw) {
    if (!active_) return false;
    pointer_ = ReadUiPointer(pointerState_);
    std::string bytes, error;
    if(job_.Busy())generationTime_+=std::clamp(dt,0.0F,.1F);
    CharacterAnalysis analysis;
    if (job_.Take(bytes, error,&analysis)) {
        try {
            if (!error.empty()) throw std::runtime_error(error);
            Image image = DecodeGeneratedCharacterImage(bytes);
            if(analysis.combat>=0) {
                if(!manualCombat_)combat_=analysis.combat;
                if(autoName_)name_=analysis.name;
                analysisDescription_=analysis.appearance;
            }
            if (generated_.data) UnloadImage(generated_);
            generated_ = image; InvalidatePreview(); MakePreview(art);
            status_ = analysis.fallback?"智能分析不可用，已按原始描述生成；请手动选择动作模板":
                analysis.combat>=0?"智能生成完成；已补全角色名称与动作建议，可调整后保存":"生成完成；检查轮廓和背景，预览动作后保存";
        } catch (const std::exception& e) { status_ = e.what(); }
    }
    if (IsKeyPressed(KEY_ESCAPE) || pointer_.Clicked(Back)) { Close(); return false; }
    try {
        if (IsFileDropped()) {
            const auto files = LoadDroppedFiles();
            std::string problem;
            try { if (files.count) SetReference(Utf8Path(files.paths[0])); } catch (const std::exception& e) { problem = e.what(); }
            UnloadDroppedFiles(files);
            if (!problem.empty()) throw std::runtime_error(problem);
        }
        if (!job_.Busy()) {
            if(pointer_.Clicked(smartToggle))settings_.smart=!settings_.smart;
            if (pointer_.pressed) {
                focus_ = -1; for (int i=0;i<5;++i) if (pointer_.Hit(fields[i])) focus_ = i;
            }
            if (IsKeyPressed(KEY_TAB)) focus_ = (focus_+1)%5;
            SetCharacterTextInput(focus_ >= 0);
            std::string* values[] = {&key_, &settings_.apiHost, &settings_.model, &name_, &settings_.instructions};
            const std::size_t limits[] = {4096, 512, 100, 120, 1800};
            if (focus_ >= 0) {
                auto& value = *values[focus_];
                const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
                if (ctrl && IsKeyPressed(KEY_A)) value.clear();
                if (ctrl && IsKeyPressed(KEY_V)) { if (const auto* s = GetClipboardText()) AppendText(value, s, limits[focus_]); }
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) PopUtf8(value);
                for (int codepoint; (codepoint=GetCharPressed()) > 0;) if (!ctrl && codepoint>=32) {
                    int count=0; const auto* text=CodepointToUTF8(codepoint,&count); AppendText(value,std::string(text,count),limits[focus_]);
                }
            }
            if (pointer_.Clicked(choose)) {
                const auto file = ChooseCharacterReference(redraw);
                pointerState_ = {};
                pointer_ = {};
                if (!file.empty()) SetReference(file);
                // Do not reuse input from the frame that opened the modal.
                return false;
            }
            if (pointer_.Clicked(generate)) { focus_=-1; SetCharacterTextInput(false); Start(); }
            if (pointer_.Clicked(local)) {
                if (!reference_.data) throw std::runtime_error("请先选择参考图");
                if (generated_.data) UnloadImage(generated_);
                generated_ = ImageCopy(reference_); removeGreen_ = false; InvalidatePreview(); MakePreview(art);
                status_ = "已套用动作模板；可预览攻击、闪避和受击，整图手脚不会独立运动";
            }
            for (int i=0;i<3;++i) if (pointer_.Clicked({58.0F+i*226,528,210,36})) {
                combat_=i; manualCombat_=true;if (hasPreview_) MakePreview(art);
            }
            if (pointer_.Clicked(keying)) { removeGreen_=!removeGreen_; if (generated_.data) { InvalidatePreview(); MakePreview(art); } }
            if (pointer_.Clicked(save)) { Save(); return true; }
        } else {
            focus_=-1; SetCharacterTextInput(false);
            if (pointer_.Clicked(generate)) { job_.Cancel(); status_="已取消等待；已提交的请求仍可能计费，请等待连接结束"; }
        }
        if (pointer_.Clicked(flip)) facing_=-facing_;
        for (int i=0;i<4;++i) if (pointer_.Clicked({800.0F+i*106,492,98,40})) { previewAction_=i; previewTime_=0; animator_={}; }
        for (int i=0;i<5;++i) if (pointer_.Clicked({800.0F+i*84,538,80,28})) { previewAction_=i+4; previewTime_=0; animator_={}; }
        const float previousPreviewTime = previewTime_;
        previewTime_ += std::clamp(dt,0.0F,.1F);
        CharacterMotion motion; motion.melee = combat_ == 0;
        motion.style = hasPreview_ ? ResolveCharacterMotionStyle(preview_) :
            combat_ == 0 ? CharacterMotionStyle::Melee : combat_ == 1 ? CharacterMotionStyle::Ranged : CharacterMotionStyle::Caster;
        const bool newCycle = previousPreviewTime == 0 ||
            std::floor(previousPreviewTime/1.2F) != std::floor(previewTime_/1.2F);
        if (previewAction_==1) motion.velocity.x=320;
        if (previewAction_==2) motion.attackTriggered = previousPreviewTime == 0 ||
            std::floor(previousPreviewTime/.9F) != std::floor(previewTime_/.9F);
        if (previewAction_==3) { motion.grounded=false; motion.velocity.y=std::fmod(previewTime_,1.4F)<.7F ? -200.0F : 200.0F; }
        if (previewAction_==4) { motion.grounded=false; motion.velocity.y=200; }
        if (previewAction_==5 && newCycle) {
            CharacterMotion airborne; airborne.grounded=false;
            animator_.Update(0,airborne);
        }
        if (previewAction_==6) motion.dodging=std::fmod(previewTime_,1.2F)<.45F;
        if (previewAction_==7 && newCycle) animator_.TriggerHurt();
        if (previewAction_==8) motion.dead=true;
        animator_.Update(dt,motion);
    } catch (const std::exception& e) { status_=e.what(); }
    return false;
}

std::string CharacterCreator::Glyphs() const {
    return name_+settings_.instructions+status_+
        "角色工坊参考图选择拖入生成小人百炼本次运行密钥不会保存可用环境变量模型补充描述创建名称即可棋盘格表示透明区域"
        "北京新加坡业务空间官方域名仅填写不含路径使用原图近战射击法术去绿幕已开关朝向翻转"
        "待机跑步攻击跳跃保存并返回编队未生成动作预览完整全身纯色背景计费取消等待连接结束"
        "账户余额不足权限失败网络检查设置恢复默认参考图片上传后按规则计费手脚独立运动"
        "生成完成轮廓透明区域重试请求支持编辑内容审核服务目录文件写入无法读取过长有效未检测到背景建议重新"
        "按切换输入框粘贴清空字段关闭页面继续后台工作只保存角色素材出战"
        "动作模板近战挥斩后坐力施放下落落地闪避受击倒地套用整图"
        "智能分析识别外观生成小人下载图片耗时秒本机密钥已载入输入替换自动补全角色名称动作建议可调整原始描述手动选择";
}
void CharacterCreator::Draw(const UiFont& font, const CharacterArt& art) const {
    SetPointerState(pointer_);
    Background(font,"角色工坊 / 图片创建","CHARACTER WORKSHOP / ALIBABA CLOUD BAILIAN");
    Surface({36,112,708,528},Paper); Surface({776,112,464,528},Ink);
    DrawRectangle(36,112,708,25,Ink);
    font.Draw("01 / CHARACTER PROFILE",58,119,11,Paper);
    Button(font,smartToggle,settings_.smart?"智能分析 / 开":"智能分析 / 关",settings_.smart,Ink,Paper);
    font.Draw("02 / PREVIEW",800,119,10,Muted);
    const char* labels[]={"API Key","API Host","模型","角色名称","补充描述"};
    const std::string values[]={key_.empty()?"":std::string(std::min<std::size_t>(key_.size(),36),'*'),settings_.apiHost,settings_.model,name_,settings_.instructions};
    for(int i=0;i<5;++i) {
        font.Draw(labels[i],58,fields[i].y+10,18,Ink);
        Surface(fields[i],focus_==i?WHITE:SoftPaper);
        DrawLineEx({fields[i].x,fields[i].y+fields[i].height},
                   {fields[i].x+fields[i].width,fields[i].y+fields[i].height},focus_==i?3.0F:1.0F,focus_==i?Blue:OnPaper);
        if(focus_==i)DrawRectangleLinesEx(fields[i],1,Blue);
        Fit(font,values[i],{fields[i].x+9,fields[i].y+5,fields[i].width-18,30},i==1?15.0F:18.0F,Ink);
    }
    font.Draw(localKey_?"本机密钥已载入；输入可临时替换":"密钥仅本次运行使用；也可设置 DASHSCOPE_API_KEY",190,188,12,Color{80,90,96,255});
    font.Draw("填写官方域名；业务空间域名和地域须与密钥一致",190,249,12,Color{80,90,96,255});
    font.Draw("补充人物特征即可；全身小人和绿幕背景由系统统一设置",58,439,13,Ink);
    Button(font,choose,"选择参考图",pointer_.Hit(choose),Ink,Paper);
    Button(font,generate,job_.Busy()?"取消等待":"生成小人",pointer_.Hit(generate),Blue,Paper);
    Button(font,local,"使用原图",pointer_.Hit(local),Ink,Paper);
    const char* types[]={"近战 / 挥斩","射击 / 后坐力","法术 / 施放"};
    for(int i=0;i<3;++i) Button(font,{58.0F+i*226,528,210,36},types[i],combat_==i,combat_==i?Blue:Color{206,213,214,255},combat_==i?Paper:Ink);
    DrawRectangle(58,580,662,1,Fade(Ink,.2F));
    const char* stages[]={"连接服务","1/3 识别人物外观","2/3 生成小人","3/3 下载图片"};
    const auto status=job_.Busy()?std::string(stages[std::clamp(job_.Stage(),0,3)])+TextFormat(" · %.0f 秒 · 可返回菜单等待",generationTime_):status_;
    Wrap(font,status,{58,590,662,45},16,Ink);
    font.Draw("REFERENCE",800,137,10,Muted); font.Draw("ACTION PREVIEW",994,137,10,Paper);
    Checker({800,155,158,236}); Checker({978,155,238,236});
    if(referenceTexture_.id) {
        const float scale=std::min(146.0F/referenceTexture_.width,224.0F/referenceTexture_.height);
        const float w=referenceTexture_.width*scale,h=referenceTexture_.height*scale;
        DrawTexturePro(referenceTexture_,{0,0,float(referenceTexture_.width),float(referenceTexture_.height)},
            {879-w/2,273-h/2,w,h},{},0,WHITE);
    } else Wrap(font,"选择或拖入参考图",{817,240,124,60},18,Paper);
    if(hasPreview_) art.DrawAnimatedCharacter(preview_,{1097,379},facing_,animator_);
    else font.Draw("未生成",1060,267,20,Muted);
    font.Draw(hasPreview_&&!transparentPreview_?"未检测到透明背景，建议重新生成":"棋盘格表示透明区域",800,403,14,
        hasPreview_&&!transparentPreview_?Orange:Muted);
    Button(font,keying,removeGreen_?"去绿幕 / 开":"去绿幕 / 关",removeGreen_,Paper,Ink);
    Button(font,flip,"翻转朝向",pointer_.Hit(flip),Paper,Ink);
    const char* actions[]={"待机","跑步","攻击","跳跃"};
    for(int i=0;i<4;++i) Button(font,{800.0F+i*106,492,98,40},actions[i],previewAction_==i,previewAction_==i?Blue:Paper,previewAction_==i?Paper:Ink);
    const char* moreActions[]={"下落","落地","闪避","受击","倒地"};
    for(int i=0;i<5;++i) Button(font,{800.0F+i*84,538,80,28},moreActions[i],previewAction_==i+4,
        previewAction_==i+4?Blue:Paper,previewAction_==i+4?Paper:Ink);
    Button(font,save,"保存并返回编队",hasPreview_&&!job_.Busy(),hasPreview_?Blue:Color{70,80,86,255},Paper);
    Footer(font,"Tab 切换输入框 · Ctrl+V 粘贴 · Ctrl+A 清空字段 · Esc 返回 · 点击生成会上传参考图并可能计费");
}
