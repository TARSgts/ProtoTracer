"""Run production startup/player/display checks with simulated display hardware.

Usage: python test_startup.py <scratch> <clang++ with wasm32> <node>
"""
import json, re, subprocess, sys
from pathlib import Path
scratch,clang,node=sys.argv[1:]
scratch=Path(scratch); scratch.mkdir(parents=True,exist_ok=True)
folder=Path(__file__).resolve().parent
repo=folder.parent.parent
display=repo/'lib/ProtoTracer/Controller'
stub='''#pragma once
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __SIZE_TYPE__ size_t;
#define PROGMEM
'''
(scratch/'Arduino.h').write_text(stub)
(scratch/'stdint.h').write_text(stub)
strip=lambda src: re.sub(r'^\s*#(?:include|pragma)[^\n]*\n','',src,flags=re.M)
src=(folder/'test_startup.cpp').read_text()
src=src.replace('// PRODUCTION_CONTROLLER_HEADER',strip((display/'HUB75Controller.h').read_text()))
src=src.replace('// PRODUCTION_CONTROLLER_BODY',strip((display/'HUB75Controller.cpp').read_text()))
(scratch/'test.cpp').write_text(src)
exports=['init','tick','frame_index','decoded_count','finished','failed','frame_ptr','palette_ptr','input_ptr','output_ptr','decode','display_frame','pixel','swaps','early_writes','accent_swaps']
subprocess.run([clang,'--target=wasm32','-std=c++17','-O2','-nostdlib','-nostdinc',
    '-I'+str(scratch),'-I'+str(repo/'lib/ProtoTracer/ExternalDevices/Displays'),str(scratch/'test.cpp'),
    '-Wl,--no-entry,--initial-memory=262144,--allow-undefined,'+','.join('--export='+n for n in exports),
    '-o',str(scratch/'test.wasm')],check=True)
report=json.loads(subprocess.check_output([node,str(folder/'test_startup.mjs'),str(scratch/'test.wasm'),str(folder/'startup-asset.json')],text=True))
(scratch/'startup-checks.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
