from pathlib import Path
import cv2,numpy as np,subprocess,imageio_ffmpeg,json
from PIL import Image,ImageDraw
R=Path(r'D:\my_game\output\texas_vfx'); arr=np.load(R/'_work/aligned.npy'); bg=np.load(R/'_work/background.npy')
H,W=bg.shape[:2]; yy,xx=np.mgrid[:H,:W]
def polygon(points):
    m=np.zeros((H,W),np.uint8);cv2.fillPoly(m,[np.array(points,np.int32)],255);return cv2.GaussianBlur(m.astype(np.float32)/255,(7,7),0)
body=polygon([(329,251),(360,214),(421,205),(454,233),(512,238),(495,290),(544,307),(479,319),(485,370),(361,374),(342,332)])
enemy=polygon([(197,202),(254,180),(296,199),(312,266),(300,291),(312,342),(275,348),(242,324),(217,347),(199,328),(218,285),(192,255)])
ui=np.ones((H,W),np.float32)
ui[96:138,240:284]=0;ui[346:365,197:304]=0;ui[378:399,353:477]=0
ui[:65]=0;ui[480:]=0
roi=polygon([(177,95),(301,91),(358,141),(615,132),(649,293),(615,477),(160,477),(151,244)])
def extract(i,kind):
    f=arr[i].astype(np.float32);d=f-bg
    change=np.max(np.abs(d),axis=2)
    pos=np.maximum(d,0)/np.maximum(255-bg,18)
    neg=np.maximum(-d,0)/np.maximum(bg,18)
    a=np.maximum(pos.max(2),neg.max(2));a=np.clip(a,0,1)
    raw_a=a.copy()
    a*=np.clip((change-18)/28,0,1)
    # Within moving sprites only retain distinctive bright effect light.
    white=np.clip((f.min(2)-160)/70,0,1)*np.clip((d.mean(2)-18)/35,0,1)
    blue=np.clip((f[:,:,2]-np.maximum(f[:,:,0],f[:,:,1])+8)/45,0,1)*np.clip((d[:,:,2]-25)/65,0,1)
    red=np.clip((f[:,:,0]-f[:,:,1]*1.55)/65,0,1)*np.clip((d[:,:,0]-12)/60,0,1)
    recovery=np.clip((f.min(2)-210)/40,0,1)*np.clip((d.mean(2)-40)/60,0,1)
    if kind=='attack':
        keep=(1-body)*(1-enemy)+np.maximum(body,enemy)*recovery
        # Incoming enemy flash at the operator is unrelated to her outgoing slash.
        keep*=1-body
        keep*=np.clip((xx-290)/24,0,1)+(1-np.clip((xx-290)/24,0,1))*np.clip((220-yy)/25,0,1)
    elif kind=='hit':
        keep=(1-enemy)+enemy*np.maximum(recovery,red*.65)
        keep*=polygon([(186,83),(285,85),(318,206),(315,344),(213,352),(169,231)])
    else:
        keep=(1-body)*(1-enemy)
    # Reject amber floor markings and weak background motion.
    rgb=np.clip(bg+d/np.maximum(raw_a[:,:,None],.035),0,255)
    warm=(rgb[:,:,0]>rgb[:,:,2]*1.45)&(rgb[:,:,1]>rgb[:,:,2]*1.35)&(rgb[:,:,1]>55)
    a*=np.where(warm,0.,1.)
    effective_ui=np.maximum(ui,recovery*.95)
    a*=np.clip(keep,0,1)*roi*effective_ui
    a[a<.11]=0
    # Recover unassociated foreground colour from the estimated plate.
    alpha=(a*255).astype(np.uint8);rgb[alpha==0]=0
    return np.dstack([rgb.astype(np.uint8),alpha])[64:480,140:652]
spec=[('01_activation_candidate','opening',290,311),('02_attack','attack',178,199),('03_hit','hit',181,200)]
ff=imageio_ffmpeg.get_ffmpeg_exe()
def encode(cmd):
    p=subprocess.run([ff,'-hide_banner','-loglevel','error','-y']+cmd,capture_output=True)
    if p.returncode:raise RuntimeError(p.stderr.decode(errors='replace'))
samples=[]
for name,kind,start,end in spec:
    folder=R/name;seq=folder/'PNG_RGBA';seq.mkdir(parents=True,exist_ok=True)
    imgs=[]
    for j,i in enumerate(range(start,end)):
        im=extract(i,kind);Image.fromarray(im).save(seq/f'{j:04d}.png');imgs.append(im)
    encode(['-framerate','30','-i',str(seq/'%04d.png'),'-c:v','prores_ks','-profile:v','4','-pix_fmt','yuva444p10le','-alpha_bits','16',str(folder/(name+'_alpha.mov'))])
    encode(['-framerate','30','-i',str(seq/'%04d.png'),'-f','lavfi','-i','color=c=0x00ff00:s=512x416:r=30','-filter_complex','[1:v][0:v]overlay=shortest=1:format=auto,format=yuv420p','-c:v','libx264','-crf','10','-preset','slow','-pix_fmt','yuv420p','-movflags','+faststart',str(folder/(name+'_green.mp4'))])
    # Exact unmodified source excerpt, to check what was visible in the recording.
    encode(['-i',r'C:\Users\34844\Videos\屏幕录制 2026-09-10 112514.mp4','-vf',f'trim=start_frame={start}:end_frame={end},setpts=PTS-STARTPTS','-an','-c:v','libx264','-crf','12',str(folder/(name+'_source.mp4'))])
    idxs=np.linspace(0,len(imgs)-1,5).astype(int)
    for j in idxs:
        im=imgs[j];a=im[:,:,3:]/255.;grid=np.indices(im.shape[:2]).sum(axis=0)//16%2
        # Checkerboard by separate x/y blocks.
        y,x=np.indices(im.shape[:2]);grid=(x//16+y//16)%2
        check=np.where(grid[:,:,None]==0,45,70)*np.ones((1,1,3))
        samples.append((name,j,Image.fromarray((im[:,:,:3]*a+check*(1-a)).astype('uint8'))))
    print(name,'exported',len(imgs),'frames',flush=True)
sheet=Image.new('RGB',(1280,3*232),(24,24,24));draw=ImageDraw.Draw(sheet)
for k,(name,j,im) in enumerate(samples):
    x=(k%5)*256;y=(k//5)*232;sheet.paste(im.resize((256,208)),(x,y));draw.text((x+3,y+210),name+' '+str(j),fill='white')
sheet.save(R/'preview.jpg')
