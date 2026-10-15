"""Generate distinct, short keyboard-style clicks for Enter and Escape."""
from pathlib import Path
import math
import random
import struct
import wave
ROOT=Path(__file__).resolve().parents[2]

def make(name,frequency,duration,seed):
    rate=44100;rng=random.Random(seed);samples=[];low=0.0
    for i in range(int(rate*duration)):
        t=i/rate;noise=rng.uniform(-1,1);low=.72*low+.28*noise
        attack=min(1,t/.0015)
        sound=attack*(.6*noise*math.exp(-t/0.006)+.5*low*math.exp(-t/0.02)+.3*math.sin(2*math.pi*frequency*t)*math.exp(-t/.012))
        samples.append(round(max(-1,min(1,sound*.65))*32767))
    path=ROOT/'releases'/name
    with wave.open(str(path),'wb') as output:
        output.setparams((1,2,rate,0,'NONE','not compressed'));output.writeframes(struct.pack('<'+'h'*len(samples),*samples))
    return path
if __name__=='__main__':
    for args in [('controller_key_enter.wav',2300,.065,31),('controller_key_escape.wav',650,.085,47)]:print(make(*args).name)
