from pathlib import Path
import zipfile,struct,hashlib,json,re
from stock_assets import Archive
root=Path(__file__).resolve().parents[2]
source=Path('C:/Users/scegi/OneDrive/Documents/American Truck Simulator/mod/ATS-VisibleDriverBody-Mustang-0.1.0.scs')
arc=Archive('C:/Program Files (x86)/Steam/steamapps/common/American Truck Simulator/base_share.scs')
mesh=arc.read('asset/character/human/driver/man/driver.pmg');assert mesh==(root/'stock-reference/driver.pmg').read_bytes()
assert struct.unpack_from('<6I',mesh)==(0x506d6715,3,3,61,4,0)
destination=root/'releases/ATS-VisibleDriverBody-Mustang-0.7.0-exterior-head-test.scs'
with zipfile.ZipFile(source) as original,zipfile.ZipFile(destination,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as out:
 body=original.read('model/spf_stock_driver/driver.pmg');assert body[112:12312]==mesh[112:12312]
 unchanged=[]
 for name in original.namelist():
  data=original.read(name)
  if name=='model/spf_stock_driver/driver.pmg':data=mesh
  elif name=='manifest.sii':data=re.sub(rb'package_version:\s*"[^"]+"',b'package_version: "0.7.0"',data)
  else:unchanged.append(name)
  out.writestr(name,data)
with zipfile.ZipFile(source) as original,zipfile.ZipFile(destination) as output:
 for name in unchanged:assert original.read(name)==output.read(name),name
 assert output.read('model/spf_stock_driver/driver.pmg')==mesh
report={'full_original_driver_mesh_restored':True,'skeleton_byte_exact':True,'base_archive_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'head_mesh_sha256':hashlib.sha256(mesh).hexdigest(),'other_animation_and_attachment_files_byte_exact':True,'archive_version':'0.7.0','visual_test_pending':True}
(root/'development/evidence/head_visibility_validation.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
