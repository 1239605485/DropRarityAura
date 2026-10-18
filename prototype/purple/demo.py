"""Texture-layer prototype. No world particles or native mod changes in this stage."""
from pathlib import Path
import argparse, json, math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT=Path(__file__).resolve().parent
ASSETS=ROOT/'assets'; ASSETS.mkdir(exist_ok=True)
OUT=ROOT/'output'; OUT.mkdir(exist_ok=True)
SIZE=192
Y,X=np.mgrid[-1:1:complex(SIZE),-1:1:complex(SIZE)]
R=np.hypot(X,Y)
edge=np.clip((1-R)/0.2,0,1)
masks={
 'bloom':np.exp(-R*R*4.8)*edge,
 'ring':(np.exp(-((R-0.65)/0.028)**2)*0.72+np.exp(-((R-0.65)/0.115)**2)*0.24)*edge,
 'star':np.clip(np.exp(-(R/0.12)**2)*0.95+np.exp(-np.abs(X)*4.3)*np.exp(-(Y/0.016)**2)*0.85+np.exp(-np.abs(Y)*7)*np.exp(-(X/0.018)**2)*0.5+np.exp(-(R/0.38)**2)*0.1,0,1)*edge,
 'spark':np.clip(np.exp(-(R/0.18)**2)+np.exp(-(X/0.035)**2)*np.exp(-np.abs(Y)*5)*0.6+np.exp(-(Y/0.035)**2)*np.exp(-np.abs(X)*5)*0.6,0,1)*edge,
}
# A smooth continuous taper: no stacked strips and no rectangular edges.
by,bx=np.mgrid[0:1:192j,-1:1:96j]
w=0.018+0.22*by**1.65
body=np.exp(-(bx/(w*2.6))**2)*0.16+np.exp(-(bx/w)**2)*0.48+np.exp(-(bx/(w*0.24+0.006))**2)*0.6
masks['beam']=np.clip(body*by**0.85*np.clip(by/0.12,0,1),0,1)
for name,m in masks.items():
 a=np.uint8(np.clip(m,0,1)*255)
 # White straight-alpha review PNG plus premultiplied RGBA32 for native upload.
 rgba=np.empty((*a.shape,4),dtype='uint8');rgba[:,:,:3]=255;rgba[:,:,3]=a
 Image.fromarray(rgba).save(ASSETS/(name+'.png'))
 pm=np.repeat(a[:,:,None],4,axis=2)
 (ASSETS/(name+'.rgba')).write_bytes(pm.tobytes())
manifest={'format':'RGBA32-premultiplied','textures':{n:{'width':m.shape[1],'height':m.shape[0]} for n,m in masks.items()},'layers':['ground bloom and rings','rear taper and lower energy tail','rear item bloom','original item sprite','centre flare and wisps','rising sparkles'],'blend':'additive RGB; keep world rendering batch state intact'}
(ASSETS/'layers.json').write_text(json.dumps(manifest,indent=2)+'\n')

class Scene:
 def __init__(self,image):self.a=np.asarray(image.convert('RGB'),dtype=np.float32)/255
 def add(self,mask,x,y,w,h,color,power=1,angle=0):
  src=Image.fromarray(np.uint8(np.clip(mask,0,1)*255)).resize((max(1,int(w)),max(1,int(h))),Image.Resampling.BICUBIC)
  if angle:src=src.rotate(angle,Image.Resampling.BICUBIC,expand=True)
  m=np.asarray(src,dtype=np.float32)/255
  left=round(x-src.width/2);top=round(y-src.height/2)
  x0=max(0,left);y0=max(0,top);x1=min(self.a.shape[1],left+src.width);y1=min(self.a.shape[0],top+src.height)
  if x1<=x0 or y1<=y0:return
  self.a[y0:y1,x0:x1]+=m[y0-top:y1-top,x0-left:x1-left,None]*np.array(color)*power
 def image(self):return Image.fromarray(np.uint8(np.clip(self.a,0,1)*255))

PURPLE=(0.68,0.16,1.0); PALE=(0.95,0.67,1.0); WHITE=(1.0,0.94,1.0)

def render(bg,t,item_mask=None,anchors=(116,160,202),enabled=None):
 s=Scene(bg);x,y,ground=anchors
 p=0.93+0.07*math.sin(t*2.2)
 enabled=set(range(6)) if enabled is None else set(enabled)
 if 0 in enabled:
  s.add(masks['bloom'],x,ground,106,22,PURPLE,.57*p)
  s.add(masks['bloom'],x,ground,65,10,PALE,.39*p)
  s.add(masks['ring'],x,ground-1,93,17,PURPLE,.75*p)
  s.add(masks['ring'],x,ground-2,67,11,PALE,.6*p)
  q=(t/3.8)%1
  s.add(masks['ring'],x,ground-2-q*4,62+q*49,12+q*9,PURPLE,math.sin(q*math.pi)*.5,math.sin(t*.6)*2)
 if 1 in enabled:
  # Beam's high-energy foot coincides with the ITEM centre, never the logical hitbox.
  s.add(masks['beam'],x,y-50,50,100,PURPLE,.98*p)
  s.add(masks['beam'],x,y-48,18,96,PALE,.72*p)
  # Reflected/streaming energy below the suspended object.
  s.add(masks['beam'],x,y+(ground-y)/2,23,ground-y, PURPLE,.34,180)
 if 2 in enabled:
  s.add(masks['bloom'],x,y,100,100,PURPLE,.65*p)
  s.add(masks['bloom'],x,y,48,48,PALE,.42*p)
 # Restore only the actual game item, not a replacement sphere.
 if item_mask is not None and enabled==set(range(6)):
  halo=np.asarray(item_mask.filter(ImageFilter.GaussianBlur(5)),dtype=np.float32)[:,:,None]/255
  s.a+=halo*np.array(PURPLE)*(.35*p)
  m=np.asarray(item_mask,dtype=np.float32)[:,:,None]/255
  raw=np.asarray(bg,dtype=np.float32)/255
  s.a=s.a*(1-m)+raw*m
 if 3 in enabled:
  # Minimal front fill catches edges while retaining the item silhouette.
  s.add(masks['bloom'],x,y,54,54,PURPLE,.21*p)
 if 4 in enabled:
  s.add(masks['star'],x,y,190,34,PURPLE,.68*p)
  s.add(masks['star'],x,y,58,22,PALE,.43*p)
  s.add(masks['bloom'],x+1,y+2,12,12,WHITE,.28*p)
  q=(t/4.2)%1
  s.add(masks['ring'],x,ground-8-q*28,57+q*13,10,PURPLE,.26*math.sin(q*math.pi),-8+q*16)
 if 5 in enabled:
  rng=np.random.default_rng(12)
  for i in range(20):
   off,speed,phase,amp,size=rng.uniform(0,1),rng.uniform(.14,.25),rng.uniform(0,6.28),rng.uniform(8,25),rng.uniform(2.5,6.5)
   q=(t*speed+off)%1
   px=x+math.sin(phase+q*5)*amp*(.45+.55*q)
   py=y+12-q*131
   alpha=math.sin(q*math.pi)**1.5*(.24+.5*size/6.5)
   s.add(masks['spark'],px,py,size,size,PALE,alpha)
  for i in range(3):
   a=t*(.5+i*.12)+i*2.094
   s.add(masks['spark'],x+math.cos(a)*22,y+math.sin(a)*13,5,5,PALE,.62)
 return s.image()

if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--background',required=True);args=ap.parse_args()
 source=Image.open(args.background).convert('RGB')
 # This earlier supplied frame has no mod effects. Crop, don't paint out prior effects.
 box=(800,190,1070,440);bg=source.crop(box)
 mask=Image.new('L',bg.size,0);d=ImageDraw.Draw(mask)
 poly=[(909,313),(933,310),(948,328),(946,354),(937,367),(932,375),(916,380),(910,375),(916,362),(908,356),(902,365),(876,388),(865,389),(869,378),(891,351),(900,345),(895,337),(904,323)]
 d.polygon([(x-box[0],y-box[1]) for x,y in poly],fill=255)
 mask=mask.filter(ImageFilter.GaussianBlur(.3))
 frames=[]
 for i in range(96):
  im=render(bg,i/24,mask)
  frames.append(im.resize((648,600),Image.Resampling.LANCZOS))
 frames[0].save(OUT/'purple-layered-demo.gif',save_all=True,append_images=frames[1:],duration=42,loop=0,optimize=True)
 frames[32].save(OUT/'purple-layered-still.png')
 # Exactly the same masks and transforms on a dark stage, for inspecting the glow.
 dark=Image.new('RGB',bg.size,(16,12,22))
 render(dark,32/24).resize((648,600),Image.Resampling.LANCZOS).save(OUT/'purple-dark-stage.png')
 # Individual passes for visual QA, not a separate proposed effect style.
 board=Image.new('RGB',(810,500),(12,13,20))
 for j in range(6):
  layer=render(dark,32/24,enabled=[j]);board.paste(layer,((j%3)*270,(j//3)*250))
 board.save(OUT/'purple-layer-passes.png')
 print(OUT/'purple-layered-demo.gif')
