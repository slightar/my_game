"""Generate a self-contained stage-select prototype page (horizontal rail).

Assets and geometry come from the real client bundle ui/[uc]stage.ab; only the
screen-level arrangement is inferred, because the bundle ships component
prefabs and the absolute positions are computed in game code.

  node tile          166 x 82      (bkg_normal / bkg_hilight / bkg_normal_branch)
  code label         +36.3, top 5.1, box 93.4 x 41.8
  rank stars         left -2.2, top 1.8, 40 x 45
  track segment      `line_spst` 4 x 96 (opaque run 4 x 52, RGB 253,70,0)
  track markers      `sprite_track_point_frame` 39x39 orange hollow diamond
                     `sprite_track_point_center` 23x23 orange disc
                     `img_dot_selected` 23x23 grey disc
  scrollbar          7 x 25 opaque white thumb + translucent white track
  details panel      371 x 478

Layout: the chapter selector is a tab strip along the top; stages run left to
right on a 222px pitch, alternating 80px up/down so the orange track folds
between them. The rail viewport is 585px wide, so longer chapters scroll
horizontally; the details panel is pinned to the right.

Output: docs/stage-select/index.html  (sprites inlined as data URIs)
"""
import io
import json
from base64 import b64encode
from pathlib import Path

from PIL import Image

SRC = Path("output/_tmp_new/stage-ui-sprites")
OUT = Path("docs/stage-select/index.html")

NEEDED = [
    "bg", "bkg_normal", "bkg_normal_branch", "bkg_hilight",
    "icon_stage_rank_3", "icon_stage_rank_0", "icon_lock",
    "sprite_track_point_frame", "sprite_track_point_center",
    "img_dot_selected", "details_bg", "dots", "scrollbar", "scrollbar_bg",
]


def uri(image: Image.Image) -> str:
    buf = io.BytesIO()
    image.save(buf, "PNG")
    return "data:image/png;base64," + b64encode(buf.getvalue()).decode()


def sprite(name: str) -> Image.Image:
    return Image.open(SRC / f"sprite__{name}.png").convert("RGBA")


def trimmed(name: str) -> Image.Image:
    im = sprite(name)
    return im.crop(im.getchannel("A").getbbox())


# `line_spst` is 4x96: 52 fully opaque rows then a long alpha fade-out. A
# stretched fade would make distant parts of the track vanish, so keep only the
# opaque run and use it as a uniform stroke in both orientations.
TRACK = sprite("line_spst")
TRACK = TRACK.crop(TRACK.getchannel("A").point(lambda v: 255 if v >= 250 else 0).getbbox())

ASSETS = {n: uri(sprite(n)) for n in NEEDED}
ASSETS["track_v"] = uri(TRACK)
ASSETS["track_h"] = uri(TRACK.transpose(Image.Transpose.ROTATE_90))
ASSETS["sbar"] = uri(trimmed("scrollbar").transpose(Image.Transpose.ROTATE_90))
ASSETS["sbar_bg"] = uri(trimmed("scrollbar_bg").transpose(Image.Transpose.ROTATE_90))

CHAPTERS = [
    ("Act initium", "序章", "黑暗时代 · 上", "0", 5),
    ("Act initium", "第一章", "黑暗时代 · 下", "1", 3),
    ("Act initium", "第二章", "异卵同生", "2", 2),
    ("Act I", "第三章", "二次呼吸", "3", 0),
    ("Act I", "第四章", "急性衰竭", "4", 0),
]

CHAPTER_JSON = json.dumps(
    [{"act": a, "title": t, "subtitle": s, "prefix": p, "cleared": c}
     for a, t, s, p, c in CHAPTERS],
    ensure_ascii=False,
)

HTML = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>主线关卡选择 · 像素级还原原型</title>
<style>
  :root{ --ink:#e8eef4; --dim:#8b9aa8; --accent:#ef9f27; --track:#fd4600; }
  *{ box-sizing:border-box; margin:0; padding:0; }
  html,body{ height:100%; }
  body{
    background:#05070a; overflow:hidden;
    font-family:"Source Han Sans SC","Noto Sans SC","PingFang SC","Microsoft YaHei",-apple-system,"Segoe UI",sans-serif;
    color:var(--ink); -webkit-font-smoothing:antialiased;
  }
  #viewport{ position:absolute; inset:0; display:block; }
  #screen{
    position:absolute; left:0; top:0; width:1024px; height:576px;
    transform-origin:top left; overflow:hidden; background:#0a1118;
    user-select:none;
  }
  #screen img{ position:absolute; display:block; image-rendering:auto; }
  .abs{ position:absolute; }
  .seg{ background-repeat:no-repeat; background-size:100% 100%; pointer-events:none; }

  #bg{ left:0; top:0; width:1024px; height:576px; }
  #scrim{ left:0; top:0; width:1024px; height:576px; background:rgba(4,8,12,.45); }

  #tabs{ left:25px; top:20px; width:975px; height:46px; display:flex; gap:10px; }
  .tab{ flex:1; display:flex; flex-direction:column; justify-content:center; padding:0 12px;
        cursor:pointer; border-bottom:2px solid transparent; background:rgba(232,238,244,.04); }
  .tab:hover{ background:rgba(232,238,244,.09); }
  .tab.on{ background:rgba(239,159,39,.10); border-bottom-color:var(--accent); }
  .tab .t-num{ font-size:14px; font-weight:500; letter-spacing:.06em; }
  .tab .t-sub{ font-size:11px; color:var(--dim); margin-top:3px; letter-spacing:.04em; }
  .tab.on .t-sub{ color:#c8b48c; }

  .eyebrow{ left:25px; top:86px; font-size:12px; letter-spacing:.28em; color:var(--dim); }
  #chapter-title{ left:23px; top:100px; font-size:26px; font-weight:500; letter-spacing:.06em; }
  #chapter-sub{ left:25px; top:140px; font-size:13px; color:var(--dim); letter-spacing:.12em; }
  #act-label{ left:25px; top:166px; font-size:11px; letter-spacing:.3em; color:#5d6b7a; }
  #hint{ left:25px; top:452px; font-size:11px; color:#5d6b7a; letter-spacing:.06em; }

  #rail{ left:25px; top:224px; width:585px; height:192px; overflow:hidden; }
  #rail-inner{ position:absolute; left:0; top:0; height:192px; }

  .tile{ width:166px; height:82px; cursor:pointer; }
  .tile .plate{ left:0; top:0; width:166px; height:82px; }
  .tile .rank{ left:-2px; top:2px; width:40px; height:45px; }
  .tile .code{ left:36px; top:5px; width:94px; height:42px; line-height:42px;
               font-size:30px; font-weight:500; letter-spacing:.02em; }
  .tile .lock{ left:112px; top:12px; width:20px; height:26px; opacity:.85; }
  .tile.done .code{ color:#f2f6fa; }
  .tile.current .code{ color:#12191f; }
  .tile.locked .code{ color:#6b7a88; }
  .tile.locked .plate{ opacity:.5; }
  .tile:hover .plate{ filter:brightness(1.12); }
  .tile.sel{ outline:1px solid rgba(239,159,39,.75); outline-offset:2px; }

  #sbar{ left:25px; top:430px; width:585px; height:7px; cursor:pointer;
         background-repeat:no-repeat; background-size:100% 100%; }
  #sbar-thumb{ left:0; top:0; height:7px; background-repeat:no-repeat; background-size:100% 100%; }

  #panel{ left:629px; top:74px; width:371px; height:478px; }
  #panel-bg{ left:0; top:0; width:371px; height:478px; }
  #p-code{ left:43px; top:90px; width:258px; height:75px; line-height:75px; text-align:center;
           font-size:44px; font-weight:500; letter-spacing:.04em; }
  #p-name{ left:60px; top:168px; width:250px; text-align:center; font-size:12px;
           color:var(--dim); letter-spacing:.16em; }
  #p-div{ left:76px; top:196px; width:219px; height:1px; background:rgba(180,196,210,.18); }
  .row{ left:76px; width:219px; height:26px; display:flex; align-items:center;
        justify-content:space-between; font-size:12px; }
  .row .k{ color:var(--dim); letter-spacing:.08em; }
  .row .v{ color:#d6e0ea; letter-spacing:.04em; }
  #p-state{ left:76px; top:322px; width:219px; font-size:12px; color:var(--accent); letter-spacing:.1em; }
  #p-dots{ left:96px; top:308px; width:60px; height:9px; opacity:.5; }
  #start{ left:110px; top:398px; width:151px; height:44px; display:flex; align-items:center;
          justify-content:center; font-size:14px; letter-spacing:.18em; cursor:pointer;
          color:#e8eef4; border:1px solid rgba(210,222,232,.55); background:rgba(232,238,244,.04);
          clip-path:polygon(8px 0,100% 0,100% calc(100% - 8px),calc(100% - 8px) 100%,0 100%,0 8px); }
  #start:hover{ background:rgba(232,238,244,.12); }
  #start.off{ color:#5d6b7a; border-color:rgba(120,134,148,.35); background:transparent;
              cursor:not-allowed; }
  #start.off:hover{ background:transparent; }
</style>
</head>
<body>
<div id="viewport"><div id="screen">
  <img id="bg" src="__BG__" alt="">
  <div class="abs" id="scrim"></div>

  <div class="abs" id="tabs"></div>

  <div class="abs eyebrow">MAIN THEME</div>
  <div class="abs" id="chapter-title"></div>
  <div class="abs" id="chapter-sub"></div>
  <div class="abs" id="act-label"></div>
  <div class="abs" id="hint">滚轮或拖动滚动条浏览关卡 · 点击关卡查看详情</div>

  <div class="abs" id="rail"><div id="rail-inner"></div></div>
  <div class="abs" id="sbar"><div class="abs" id="sbar-thumb"></div></div>

  <div class="abs" id="panel">
    <img id="panel-bg" src="__PANEL__" alt="">
    <div class="abs" id="p-code"></div>
    <div class="abs" id="p-name">关卡名称待补充</div>
    <div class="abs" id="p-div"></div>
    <img class="abs" id="p-dots" src="__DOTS__" alt="">
    <div class="abs row" style="top:232px"><span class="k">推荐等级</span><span class="v">—</span></div>
    <div class="abs row" style="top:262px"><span class="k">理智消耗</span><span class="v">—</span></div>
    <div class="abs row" style="top:292px"><span class="k">首次掉落</span><span class="v">—</span></div>
    <div class="abs" id="p-state"></div>
    <div class="abs" id="start">开始行动</div>
  </div>
</div></div>

<script>
const A = __ASSETS__;
const CHAPTERS = __CHAPTERS__;

const TILE_W = 166, TILE_H = 82, COUNT = 5;
const PITCH = 222, GAP = PITCH - TILE_W;
const Y_UP = 10, Y_DOWN = 90, JOG = Y_DOWN - Y_UP;
const RAIL_W = 585, RAIL_H = 192;
const CONTENT_W = (COUNT - 1) * PITCH + TILE_W;
const MAX_SC = CONTENT_W - RAIL_W;
const THUMB_W = Math.round(RAIL_W * RAIL_W / CONTENT_W);

const tileY = (i) => (i % 2 === 0 ? Y_UP : Y_DOWN);
const midY  = (i) => tileY(i) + TILE_H / 2;
const tileX = (i) => i * PITCH;
const nodeX = (i) => (i === 0 ? TILE_W + GAP / 2 : i * PITCH - GAP / 2);

let ci = 1, si = 3, sc = 0;

function stateOf(ch, i){
  if (i < ch.cleared) return "done";
  if (i === ch.cleared) return "current";
  return "locked";
}

function addSeg(host, left, top, w, h, image){
  const d = document.createElement("div");
  d.className = "abs seg";
  d.style.left = left + "px";
  d.style.top = top + "px";
  d.style.width = w + "px";
  d.style.height = h + "px";
  d.style.backgroundImage = "url(" + image + ")";
  host.appendChild(d);
  return d;
}

function addMarker(host, image, size, x, y){
  const m = document.createElement("img");
  m.src = image;
  m.className = "abs";
  m.style.left = (x - size / 2) + "px";
  m.style.top = (y - size / 2) + "px";
  m.style.width = size + "px";
  m.style.height = size + "px";
  host.appendChild(m);
  return m;
}

function clampSc(v){ return Math.max(0, Math.min(MAX_SC, v)); }

function applyScroll(){
  sc = clampSc(sc);
  document.getElementById("rail-inner").style.transform = "translateX(" + (-sc) + "px)";
  const travel = RAIL_W - THUMB_W;
  document.getElementById("sbar-thumb").style.left =
    (MAX_SC ? (sc / MAX_SC) * travel : 0) + "px";
}

function revealStage(i){
  const x0 = tileX(i), x1 = x0 + TILE_W;
  if (x0 - 8 < sc) sc = x0 - 8;
  else if (x1 + 8 > sc + RAIL_W) sc = x1 + 8 - RAIL_W;
}

function renderTabs(){
  const host = document.getElementById("tabs");
  host.innerHTML = "";
  CHAPTERS.forEach((ch, i) => {
    const d = document.createElement("div");
    d.className = "tab" + (i === ci ? " on" : "");
    d.innerHTML = `<div class="t-num">${ch.title}</div><div class="t-sub">${ch.subtitle}</div>`;
    d.onclick = () => { ci = i; si = Math.min(ch.cleared, COUNT - 1); sc = 0; render(); };
    host.appendChild(d);
  });
}

function renderStages(){
  const host = document.getElementById("rail-inner");
  host.innerHTML = "";
  host.style.width = CONTENT_W + "px";
  const ch = CHAPTERS[ci];

  for (let i = 0; i < COUNT; i++) {
    const st = stateOf(ch, i);
    const x = tileX(i), y = tileY(i);

    if (i < COUNT - 1) {
      addSeg(host, x + TILE_W, midY(i) - 2, GAP / 2, 4, A.track_h);
      addSeg(host, x + TILE_W + GAP / 2 - 2, Math.min(midY(i), midY(i + 1)), 4, JOG, A.track_v);
      addSeg(host, x + TILE_W + GAP / 2, midY(i + 1) - 2, GAP / 2, 4, A.track_h);
    }

    const marker = st === "current" ? [A.sprite_track_point_frame, 39]
                 : st === "done" ? [A.sprite_track_point_center, 23]
                 : [A.img_dot_selected, 23];
    addMarker(host, marker[0], marker[1], nodeX(i), midY(i));

    const t = document.createElement("div");
    t.className = "tile abs " + st + (i === si ? " sel" : "");
    t.style.left = x + "px";
    t.style.top = y + "px";
    const plate = st === "current" ? A.bkg_hilight
                : st === "done" ? (i === COUNT - 1 ? A.bkg_normal_branch : A.bkg_normal)
                : A.bkg_normal;
    let inner = `<img class="abs plate" src="${plate}" alt="">`;
    if (st === "locked") {
      inner += `<img class="abs lock" src="${A.icon_lock}" alt="">`;
    } else {
      const rank = st === "current" ? A.icon_stage_rank_0 : A.icon_stage_rank_3;
      inner += `<img class="abs rank" src="${rank}" alt="">`;
    }
    inner += `<div class="abs code">${ch.prefix}-${i + 1}</div>`;
    t.innerHTML = inner;
    t.onclick = () => { si = i; render(); };
    host.appendChild(t);
  }
  revealStage(si);
  applyScroll();
}

function renderPanel(){
  const ch = CHAPTERS[ci];
  const st = stateOf(ch, si);
  document.getElementById("p-code").textContent = `${ch.prefix}-${si + 1}`;
  document.getElementById("p-name").textContent = "关卡名称待补充";
  const state = document.getElementById("p-state");
  const btn = document.getElementById("start");
  state.textContent = st === "done" ? "已通关" : st === "current" ? "可挑战" : "未解锁";
  state.style.color = st === "done" ? "#9fd6a8" : st === "current" ? "#ef9f27" : "#6b7a88";
  btn.className = st === "locked" ? "abs off" : "abs";
  btn.id = "start";
}

function render(){
  const ch = CHAPTERS[ci];
  document.getElementById("chapter-title").textContent = ch.title;
  document.getElementById("chapter-sub").textContent = ch.subtitle;
  document.getElementById("act-label").textContent = ch.act;
  renderTabs(); renderStages(); renderPanel();
}

document.getElementById("rail").addEventListener("wheel", (e) => {
  sc = clampSc(sc + e.deltaY);
  applyScroll();
});

let dragging = false, dragX = 0, dragSc = 0;
const sbar = document.getElementById("sbar");
const thumb = document.getElementById("sbar-thumb");
sbar.style.backgroundImage = "url(" + A.sbar_bg + ")";
thumb.style.backgroundImage = "url(" + A.sbar + ")";
thumb.style.width = THUMB_W + "px";

sbar.addEventListener("mousedown", (e) => {
  if (e.target === thumb) { dragging = true; dragX = e.clientX; dragSc = sc; return; }
  sc = clampSc((e.offsetX - THUMB_W / 2) / (RAIL_W - THUMB_W) * MAX_SC);
  applyScroll();
});
addEventListener("mousemove", (e) => {
  if (!dragging) return;
  sc = clampSc(dragSc + (e.clientX - dragX) / (RAIL_W - THUMB_W) * MAX_SC);
  applyScroll();
});
addEventListener("mouseup", () => { dragging = false; });

function fit(){
  const s = Math.min(innerWidth / 1024, innerHeight / 576);
  const el = document.getElementById("screen");
  el.style.transform = `scale(${s})`;
  el.style.left = Math.round((innerWidth - 1024 * s) / 2) + "px";
  el.style.top = Math.round((innerHeight - 576 * s) / 2) + "px";
}
addEventListener("resize", fit);
document.getElementById("start").onclick = () => {};
fit(); render();
</script>
</body>
</html>
"""

html = (HTML
        .replace("__BG__", ASSETS["bg"])
        .replace("__PANEL__", ASSETS["details_bg"])
        .replace("__DOTS__", ASSETS["dots"])
        .replace("__ASSETS__", json.dumps(ASSETS))
        .replace("__CHAPTERS__", CHAPTER_JSON))

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(html, encoding="utf-8")
print(f"wrote {OUT}  {OUT.stat().st_size // 1024} KB")
print(f"chapters={len(CHAPTERS)}  stages/chapter=5  sprites={len(ASSETS)}")
print("rail 585x192  pitch 222  tile 166x82  jog 80  content 1054  scroll range 469")
print(f"track stroke = {TRACK.size[0]}x{TRACK.size[1]} (opaque run of line_spst 4x96)")
