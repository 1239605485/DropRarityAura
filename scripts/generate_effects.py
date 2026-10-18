"""Approved prototype masks, premultiplied RGBA32, bottom-up for Unity."""
import math, sys
from pathlib import Path
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
for name in ('bloom','ring','star','spark','beam'):
 w,h=(96,192) if name=='beam' else (192,192)
 data=bytearray()
 for row in reversed(range(h)):
  y=row/(h-1)*2-1
  for col in range(w):
   x=col/(w-1)*2-1;r=math.hypot(x,y);edge=max(0,min(1,(1-r)/.2))
   if name=='bloom':v=math.exp(-r*r*4.8)*edge
   elif name=='ring':v=(math.exp(-((r-.65)/.028)**2)*.72+math.exp(-((r-.65)/.115)**2)*.24)*edge
   elif name=='star':v=min(1,math.exp(-(r/.12)**2)*.95+math.exp(-abs(x)*4.3)*math.exp(-(y/.016)**2)*.85+math.exp(-abs(y)*7)*math.exp(-(x/.018)**2)*.5+math.exp(-(r/.38)**2)*.1)*edge
   elif name=='spark':v=min(1,math.exp(-(r/.18)**2)+math.exp(-(x/.035)**2)*math.exp(-abs(y)*5)*.6+math.exp(-(y/.035)**2)*math.exp(-abs(x)*5)*.6)*edge
   else:
    q=row/(h-1);a=.018+.22*q**1.65
    v=min(1,(math.exp(-(x/(a*2.6))**2)*.16+math.exp(-(x/a)**2)*.48+math.exp(-(x/(a*.24+.006))**2)*.6)*q**.85*min(1,q/.12))
   a=int(max(0,min(1,v))*255);data.extend((a,a,a,a))
 (out/(name+'.rgba')).write_bytes(data)
 if name=='beam':
  stride=w*4
  (out/'tail.rgba').write_bytes(b''.join(data[i:i+stride] for i in reversed(range(0,len(data),stride))))
