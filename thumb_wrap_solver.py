"""Fit thumb joints using a conservative finger capsule proxy (NumPy only)."""
import math
import numpy as np

def solve_thumb_wrap(pose,flat,world,palm,rotation,quaternion):
 hand=world[7];hand_r=hand[:3,:3]
 root=hand[:3,3]+hand_r@pose[20][8:11]
 target=(world[9][:3,3]+world[10][:3,3])/2+palm*.022
 finger_a=[];finger_b=[]
 for chain in ((8,9,10),(11,12,13),(14,15,16),(17,18,19)):
  points=[world[i][:3,3] for i in chain]
  points.append(points[-1]+world[chain[-1]][:3,:3]@np.array([.018,0.,0.]))
  for a,b in zip(points[:-1],points[1:]):finger_a.append(a);finger_b.append(b)
 finger_a=np.array(finger_a);finger_b=np.array(finger_b);vectors=finger_b-finger_a
 lengths=(vectors*vectors).sum(axis=1)
 def axis(degrees,index):
  a=math.radians(degrees);v=np.zeros(3);v[index]=math.sin(a/2)
  return rotation((math.cos(a/2),*v))
 def evaluate(angles,details=False):
  q20=quaternion(rotation(flat[20][4:8])@axis(angles[0],0)@axis(angles[1],1)@axis(angles[2],2),flat[20][4:8])
  q21=quaternion(rotation(flat[21][4:8])@axis(angles[3],2),flat[21][4:8])
  q22=quaternion(rotation(flat[22][4:8])@axis(angles[4],2),flat[22][4:8])
  r20=hand_r@rotation(q20);p21=root+r20@pose[21][8:11]
  r21=r20@rotation(q21);p22=p21+r21@pose[22][8:11]
  r22=r21@rotation(q22);tip=p22+r22@np.array([.018,0.,0.])
  points=np.concatenate([np.linspace(root+(p21-root)*.35,p21,15),np.linspace(p21,p22,15),np.linspace(p22,tip,15)])
  delta=points[:,None,:]-finger_a[None,:,:]
  fraction=np.clip((delta*vectors[None,:,:]).sum(axis=2)/lengths[None,:],0,1)
  residual=delta-fraction[:,:,None]*vectors[None,:,:]
  distances=np.linalg.norm(residual,axis=2)
  penetration=np.maximum(0,.018-distances)
  score=float(np.dot(tip-target,tip-target)*5+(penetration*penetration).sum()*30)
  # Keep distal thumb on the exposed palm side of the folded fingers.
  score+=max(0,.010-np.dot(p22-(world[9][:3,3]+world[10][:3,3])/2,palm))**2*5
  score+=float(np.dot(angles,angles))*1e-8
  return (score,(q20,q21,q22),tip,float(distances.min())) if details else score
 bounds=np.array([[-40,40],[-60,60],[-35,70],[0,100],[0,90]],float)
 best=None
 for seed in ((0,0,10,15,65),(0,25,20,45,35),(20,-25,0,60,30),(-20,40,35,30,55)):
  x=np.array(seed,dtype=float);score=evaluate(x)
  for step in (30,15,7.5,3,1):
   for sweep in range(16):
    changed=False
    for i in range(5):
     for sign in (-1,1):
      trial=x.copy();trial[i]=np.clip(trial[i]+step*sign,*bounds[i]);value=evaluate(trial)
      if value<score:score=value;x=trial;changed=True
    if not changed:break
  result=evaluate(x,True)
  if best is None or result[0]<best[0]:best=(*result,x)
 score,quats,tip,clearance,angles=best
 assert clearance>.014,'Thumb proxy still intersects finger proxy'
 assert np.linalg.norm(tip-target)<.035,'Thumb wrap misses folded index middle segment'
 for i,q in zip((20,21,22),quats):pose[i][4:8]=q
 return {'joint_adjustment_degrees':angles.tolist(),'finger_proxy_clearance_m':clearance,'contact_target_m':target.tolist(),'tip_m':tip.tolist(),'tip_target_error_m':float(np.linalg.norm(tip-target)),'mesh_clearance_visual_test_pending':True}
