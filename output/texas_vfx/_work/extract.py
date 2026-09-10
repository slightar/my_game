from pathlib import Path
import cv2, numpy as np, json
from PIL import Image, ImageDraw
ROOT=Path(r'D:\my_game\output\texas_vfx')
SRC=r'C:\Users\34844\Videos\屏幕录制 2026-09-10 112514.mp4'
c=cv2.VideoCapture(SRC); frames=[]
while True:
    ok,f=c.read()
    if not ok: break
    frames.append(cv2.cvtColor(f,cv2.COLOR_BGR2RGB))
ref=frames[9]; h,w=ref.shape[:2]
gray=lambda f:cv2.GaussianBlur(cv2.cvtColor(f,cv2.COLOR_RGB2GRAY),(5,5),0)
mask=np.ones((h,w),np.uint8)*255; mask[70:480,160:640]=0; mask[480:]=0; mask[:,:25]=0
aligned=[]; warps=[]
for i,f in enumerate(frames):
    warp=np.eye(2,3,dtype=np.float32)
    try: _,warp=cv2.findTransformECC(gray(ref),gray(f),warp,cv2.MOTION_AFFINE,(cv2.TERM_CRITERIA_COUNT|cv2.TERM_CRITERIA_EPS,70,1e-4),mask,5)
    except cv2.error: pass
    aligned.append(cv2.warpAffine(f,warp,(w,h),flags=cv2.INTER_LINEAR|cv2.WARP_INVERSE_MAP,borderMode=cv2.BORDER_REFLECT))
    warps.append(warp.tolist())
arr=np.stack(aligned)
bg=np.median(arr[6:280:2],axis=0).astype(np.float32)
Image.fromarray(bg.astype('uint8')).save(ROOT/'_work/background.png')
np.save(ROOT/'_work/aligned.npy',arr)
np.save(ROOT/'_work/background.npy',bg)
(ROOT/'_work/warps.json').write_text(json.dumps(warps))
print('Aligned',len(frames),flush=True)
