"""Extract a timestamped, contiguous source-video frame sequence (no interpolation)."""
import argparse
import json
import re
import subprocess
from pathlib import Path
from PIL import Image, ImageDraw


def main():
    p=argparse.ArgumentParser();p.add_argument('--video',type=Path,required=True)
    p.add_argument('--start',type=float,required=True);p.add_argument('--duration',type=float,default=1)
    p.add_argument('--crop',default='');p.add_argument('--output',type=Path,required=True)
    p.add_argument('--ffmpeg',required=True);args=p.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    info=subprocess.run([args.ffmpeg,'-hide_banner','-i',str(args.video)],capture_output=True,text=True).stderr
    fps=float(re.search(r'(\d+(?:\.\d+)?) fps',info)[1])
    command=[args.ffmpeg,'-hide_banner','-loglevel','error','-y','-ss',str(args.start),'-i',str(args.video),'-t',str(args.duration)]
    if args.crop:command+=['-vf','crop='+args.crop]
    subprocess.run(command+['-fps_mode','passthrough',str(args.output/'frame-%04d.png')],check=True)
    files=sorted(args.output.glob('frame-*.png'))
    for start in range(0,len(files),30):
        group=files[start:start+30];w,h=Image.open(group[0]).size
        ratio=min(1,300/w);w=int(w*ratio);h=int(h*ratio)
        sheet=Image.new('RGB',(w*6,(h+22)*((len(group)+5)//6)),(20,24,30));draw=ImageDraw.Draw(sheet)
        for i,path in enumerate(group):
            x=i%6*w;y=i//6*(h+22);sheet.paste(Image.open(path).resize((w,h)),(x,y))
            draw.text((x+4,y+h+3),f'{args.start+(start+i)/fps:.3f}s  #{start+i}',fill='white')
        sheet.save(args.output/f'sequence-{start//30:02d}.jpg')
    (args.output/'sequence.json').write_text(json.dumps({'source':str(args.video),'start':args.start,'fps':fps,
        'frames':len(files),'crop':args.crop,'note':'Video frame timing; in-game speed and pauses must be checked separately'},indent=2))
    print(args.output,len(files),'frames',fps,'fps')


if __name__=='__main__':main()
