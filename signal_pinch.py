"""Fit index/middle fingertips near the stock thumb, preserving the remaining grip."""
import math
import numpy as np

def pinch_fingers(neutral,mesh,rotation,quaternion):
 flat={i:np.array(__import__('struct').unpack_from('<14f',mesh,112+i*200+136)[4:8]) for i in (8,9,10,11,12,13)}
 def axis(angle,index):
  a=math.radians(angle);v=np.zeros(3);v[index]=math.sin(a/2);return rotation((math.cos(a/2),*v))
 # Work entirely in the hand's local frame.
 r20=rotation(neutral[20][4:8]);p20=neutral[20][8:11]
 r21=r20@rotation(neutral[21][4:8]);p21=p20+r20@neutral[21][8:11]
 r22=r21@rotation(neutral[22][4:8]);p22=p21+r21@neutral[22][8:11]
 thumb_tip=p22+r22@np.array([.018,0,0])
 span=neutral[8][8:11]-neutral[11][8:11];span/=np.linalg.norm(span)
 result={};report={};finger_tips=[]
 for chain,sign in [((8,9,10),1),((11,12,13),-1)]:
  target=thumb_tip+span*(sign*.008)
  def evaluate(x,details=False):
   qs=[quaternion(rotation(flat[chain[0]])@axis(x[0],1)@axis(x[1],2),flat[chain[0]]),quaternion(rotation(flat[chain[1]])@axis(x[2],2),flat[chain[1]]),quaternion(rotation(flat[chain[2]])@axis(x[3],2),flat[chain[2]])]
   r=rotation(qs[0]);p=neutral[chain[0]][8:11].copy()
   for i,q in zip(chain[1:],qs[1:]):p+=r@neutral[i][8:11];r=r@rotation(q)
   tip=p+r@np.array([.018,0,0]);score=float(np.dot(tip-target,tip-target)+np.dot(x,x)*1e-9)
   return (score,qs,tip) if details else score
  bounds=[(-30,30),(0,110),(0,110),(0,90)];best=None
  for seed in [(0,45,45,15),(0,75,65,25),(15,30,65,25),(-15,75,40,10)]:
   x=np.array(seed,dtype=float);score=evaluate(x)
   for step in (20,10,5,2,1):
    for sweep in range(12):
     changed=False
     for i in range(4):
      for sign2 in (-1,1):
       trial=x.copy();trial[i]=np.clip(trial[i]+step*sign2,*bounds[i]);value=evaluate(trial)
       if value<score:x=trial;score=value;changed=True
     if not changed:break
   details=evaluate(x,True)
   if best is None or details[0]<best[0]:best=(*details,x)
  score,qs,tip,angles=best
  angles=angles.copy();angles[2]=max(0,angles[2]-15);angles[3]=max(0,angles[3]-10)
  score,qs,tip=evaluate(angles,True)
  finger_tips.append(tip.copy())
  for i,q in zip(chain,qs):result[i]=q
  report[str(chain[0])]={'angles_degrees':angles.tolist(),'thumb_tip_gap_m':float(np.linalg.norm(tip-thumb_tip)),'target_error_m':float(np.linalg.norm(tip-target))}
 # Bring the thumb toward the shared fingertip contact area.
 contact=(finger_tips[0]+finger_tips[1])/2
 def thumb_eval(x,details=False):
  qs=[quaternion(rotation(neutral[20][4:8])@axis(x[0],0)@axis(x[1],1)@axis(x[2],2),neutral[20][4:8]),quaternion(rotation(neutral[21][4:8])@axis(x[3],2),neutral[21][4:8]),quaternion(rotation(neutral[22][4:8])@axis(x[4],2),neutral[22][4:8])]
  r=rotation(qs[0]);pos=neutral[20][8:11].copy()
  for i,q in zip((21,22),qs[1:]):pos+=r@neutral[i][8:11];r=r@rotation(q)
  tip=pos+r@np.array([.018,0,0]);score=float(np.dot(tip-contact,tip-contact)*5+np.dot(x,x)*1e-9)
  return (score,qs,tip) if details else score
 x=np.zeros(5);score=thumb_eval(x);bounds=[(-25,25),(-60,60),(-60,60),(-45,45),(-45,45)]
 for step in (20,10,5,2,1):
  for sweep in range(16):
   changed=False
   for i in range(5):
    for sign in (-1,1):
     trial=x.copy();trial[i]=np.clip(trial[i]+step*sign,*bounds[i]);value=thumb_eval(trial)
     if value<score:x=trial;score=value;changed=True
   if not changed:break
 x=np.array([-17.,0.,0.,20.,6.]) # Preserve the last visually tested thumb pose.
 _,qs,tip=thumb_eval(x,True)
 for i,q in zip((20,21,22),qs):result[i]=q
 report['thumb']={'adjustment_degrees':x.tolist(),'index_gap_m':float(np.linalg.norm(tip-finger_tips[0])),'middle_gap_m':float(np.linalg.norm(tip-finger_tips[1])),'mesh_contact_visual_test_pending':True}
 return result,report
