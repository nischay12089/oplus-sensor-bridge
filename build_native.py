"""Build the vendor daemon only; optionally rebuild its SystemUI JNI companion."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

root=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--with-aod-hook', action='store_true')
parser.add_argument('--no-panel-writes', action='store_true', help='Disable direct AOD brightness control to isolate FOD interference')
args=parser.parse_args()
sdk=Path(os.environ.get('ANDROID_HOME',Path(os.environ['LOCALAPPDATA'])/'Android/Sdk'))
cxx=sdk/'ndk/26.3.11579264/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe'
out=root/'out'
out.mkdir(exist_ok=True)
command=[cxx,'--target=aarch64-linux-android34','-std=c++17','-O2','-fPIE','-pie',
 '-static-libstdc++','-Wall','-Wno-unused-parameter','-DBRIDGE_PANEL_FEATURE',
 '-I'+str(root/'common'),'-I'+str(root/'aidl'),'-I'+str(root/'aidl/compat'),
 '-L'+str(root/'sysroot/vendor_lib64'),'-Wl,--allow-shlib-undefined','-Wl,--as-needed',
 root/'daemon/oplus_sensor_bridged.cpp',
 root/'aidl/vendor/oplus/hardware/displaypanelfeature/IDisplayPanelFeature.cpp',
 '-l:libsensorndkbridge.so','-l:libbinder_ndk.so','-o',out/'oplus-sensor-bridged.vendor']
if args.no_panel_writes:
    command.insert(1,'-DBRIDGE_NO_PANEL_WRITES')
subprocess.run([str(value) for value in command],check=True)
if args.with_aod_hook:
    project=root/'aodfade'
    if not (project/'build.py').exists():
        project=root.parent/'aodfade'
    if not (project/'build.py').exists():
        raise SystemExit('Extract the JNI companion source.zip into bridge/aodfade first.')
    subprocess.run([sys.executable,str(project/'build.py')],check=True)
    shutil.copy2(project/'build-release/libaodfade.so',out/'libaodfade.so')
print(out/'oplus-sensor-bridged.vendor')
