from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math

OUT = Path(__file__).with_name('chernobog_cross_section.png')
W, H = 2200, 1450
img = Image.new('RGB', (W, H), '#101923')
d = ImageDraw.Draw(img)
font_path = 'C:/Windows/Fonts/msyh.ttc'
bold_path = 'C:/Windows/Fonts/msyhbd.ttc'
title = ImageFont.truetype(bold_path, 54)
heading = ImageFont.truetype(bold_path, 32)
body = ImageFont.truetype(font_path, 25)
small = ImageFont.truetype(font_path, 21)

WHITE = '#e8f0f3'
MUTED = '#a3b4bf'
MAIN = '#55cdd1'
HIDDEN = '#d4a86b'
BOSS = '#ed786e'
THERESA = '#e1a9c0'

d.text((85, 42), '切尔诺伯格 · 可探索城区横截面', font=title, fill=WHITE)
d.text((89, 112), '向右：主线推进    纵向：实际高度    青色实线：必经路线    金色虚线：探索', font=small, fill=MUTED)

bands = [
    ('高架层  +2', 185, 335),
    ('建筑上层  +1', 370, 520),
    ('地表  0', 555, 730),
    ('地下  -1', 765, 945),
    ('设施深层  -2', 980, 1160),
    ('系统空间', 1195, 1390),
]
for i, (name, y0, y1) in enumerate(bands):
    d.rectangle((42, y0, 2155, y1), fill='#17232e' if i % 2 == 0 else '#13202b')
    d.line((225, y1, 2150, y1), fill='#38505d', width=2)
    d.text((54, y0 + 20), name, font=heading, fill=WHITE)
    d.text((55, y0 + 70), ['钟楼／高架', '核心上部', '街区', '车站／管线', '隐藏根系', '非物理空间'][i], font=small, fill=MUTED)
d.line((225, 185, 225, 1390), fill='#42606d', width=3)

nodes = {
    'clinic': (325, 855, '地下诊疗所', '起点／重生', 'main'),
    'gray': (325, 640, '灰雪街区', 'PRTS 接管', 'main'),
    'clock': (325, 260, '停摆钟楼', '残响／隐藏', 'hidden'),
    'station': (620, 855, '旧车站', '地下通道', 'main'),
    'entry': (620, 640, '高架入口', '楼梯上行', 'main'),
    'bridge': (680, 260, '高架桥', '① 弑君者', 'boss'),
    'wtower': (1020, 260, '通讯塔上层', '② W', 'boss'),
    'comm': (1020, 640, '废弃通讯站', '升降机出口', 'main'),
    'well': (1020, 855, '维护井', 'W 的爆破缺口', 'hidden'),
    'ice': (1330, 640, '冰封居住区', '③ 霜星', 'boss'),
    'pipes': (1330, 855, '热能管线', '向下调查', 'main'),
    'shelter': (1330, 1065, '冰下避难所', '藏品／隐藏', 'hidden'),
    'industry': (1580, 855, '工业区关口', '④ 爱国者', 'boss'),
    'burn': (1815, 640, '燃烧街区', '地表主线', 'main'),
    'core': (1815, 445, '城市核心', '⑤ 塔露拉', 'boss'),
    'normal': (2040, 445, '任务结算', '普通结局', 'ending'),
    'ward': (620, 1065, '零号病房', '旧电梯回访', 'hidden'),
    'root': (1815, 1065, 'PRTS 数据根系', '塔露拉烧开的竖井', 'hidden'),
    'prts': (1560, 1285, 'PRTS 核心', '协议失效', 'boss'),
    'throne': (1850, 1285, '魔王王座', '特蕾西娅', 'theresa'),
}

def center(key):
    x, y, *_ = nodes[key]
    return x, y

def route(points, color=MAIN, width=6, dashed=False, arrow=True):
    if dashed:
        for a, b in zip(points, points[1:]):
            dx, dy = b[0]-a[0], b[1]-a[1]
            length = math.hypot(dx,dy)
            if length == 0: continue
            ux, uy = dx/length, dy/length
            n = 0
            while n < length:
                end = min(n+15,length)
                d.line((a[0]+ux*n,a[1]+uy*n,a[0]+ux*end,a[1]+uy*end), fill=color, width=width)
                n += 27
    else:
        d.line(points, fill=color, width=width, joint='curve')
    if arrow:
        a,b=points[-2],points[-1]
        ang=math.atan2(b[1]-a[1],b[0]-a[0])
        q=14
        p1=(b[0]-q*math.cos(ang-0.52), b[1]-q*math.sin(ang-0.52))
        p2=(b[0]-q*math.cos(ang+0.52), b[1]-q*math.sin(ang+0.52))
        d.polygon([b,p1,p2], fill=color)

# Main route: solid cyan. Lines stop before cards so direction stays readable.
route([(325,810),(325,690)])
route([(435,640),(510,640)])
route([(620,595),(620,307)])
route([(790,260),(910,260)])
route([(1020,307),(1020,593)])
route([(1130,640),(1220,640)])
route([(1330,687),(1330,808)])
route([(1440,855),(1470,855)])
route([(1690,855),(1740,855),(1740,687)])
route([(1815,593),(1815,492)])
route([(1925,445),(1930,445)], color=MAIN)

# Hidden and backtracking connections.
route([(325,593),(325,307)], HIDDEN, 4, True)
route([(435,260),(570,260)], HIDDEN, 4, True)
route([(510,855),(510,690)], HIDDEN, 4, True)
route([(620,903),(620,1018)], HIDDEN, 4, True)
route([(510,1065),(400,1065),(400,903)], HIDDEN, 4, True)
route([(730,855),(910,855)], HIDDEN, 4, True)
route([(1020,687),(1020,808)], HIDDEN, 4, True)
route([(1130,855),(1220,855)], HIDDEN, 4, True)
route([(1330,903),(1330,1018)], HIDDEN, 4, True)
route([(1440,1065),(1705,1065)], HIDDEN, 4, True)
route([(1815,492),(1815,1018)], THERESA, 5, True)
route([(1710,1095),(1560,1095),(1560,1238)], THERESA, 5)
route([(1670,1285),(1740,1285)], THERESA, 5)

def card(key):
    x,y,name,subtitle,kind=nodes[key]
    w,h=220,94
    colors={'main':MAIN,'boss':BOSS,'hidden':HIDDEN,'ending':MAIN,'theresa':THERESA}
    accent=colors[kind]
    bg={'main':'#223743','boss':'#432c34','hidden':'#3d352d','ending':'#263d42','theresa':'#40303e'}[kind]
    box=(x-w//2,y-h//2,x+w//2,y+h//2)
    d.rounded_rectangle(box, radius=13, fill=bg, outline=accent, width=3)
    d.rectangle((x-w//2,y-h//2,x-w//2+7,y+h//2), fill=accent)
    name_box=d.textbbox((0,0),name,font=heading)
    d.text((x-(name_box[2]-name_box[0])/2,y-35),name,font=heading,fill=WHITE)
    sub_box=d.textbbox((0,0),subtitle,font=small)
    d.text((x-(sub_box[2]-sub_box[0])/2,y+11),subtitle,font=small,fill=accent)

for key in nodes:
    card(key)

# Prologue belongs to the connection layer, not the physical city section.
d.rounded_rectangle((930, 36, 2128, 153), radius=15, fill='#302a39', outline=THERESA, width=3)
d.text((957, 53), '序章异常接入：诊疗所 → 魔王王座 → 茧笼断线 → 诊疗所重生', font=body, fill=WHITE)
d.text((957, 99), '王座不是切城的物理房间；终章从数据根系进入系统空间才真正抵达。', font=small, fill=THERESA)

img.save(OUT)
print(OUT)
