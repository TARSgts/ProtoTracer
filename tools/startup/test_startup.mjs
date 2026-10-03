import fs from 'node:fs';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
const [wasmPath,manifestPath]=process.argv.slice(2);
const manifest=JSON.parse(fs.readFileSync(manifestPath,'utf8'));
let memory;
const env={memset:(p,v,n)=>{new Uint8Array(memory.buffer).fill(v,p,p+n);return p;},memcpy:(d,s,n)=>{new Uint8Array(memory.buffer).copyWithin(d,s,s+n);return d;}};
const {instance}=await WebAssembly.instantiate(fs.readFileSync(wasmPath),{env});
const e=instance.exports; memory=e.memory;
let checks=0;
function check(condition,message){assert.ok(condition,message);++checks;}
function frame(){return new Uint8Array(memory.buffer,e.frame_ptr(),2048);}
function palette(){return new Uint8Array(memory.buffer,e.palette_ptr(),768);}
e.init();
check(e.tick(100)===1&&e.frame_index()===0,'First call starts at frame zero');
for(let i=0;i<96;++i){
    check(e.tick(100+Math.ceil(i*1000/24))===1&&e.frame_index()===i,`Frame ${i} exact timestamp`);
    const rgb=Buffer.alloc(6144),p=palette();
    frame().forEach((index,n)=>{rgb[n*3]=p[index*3];rgb[n*3+1]=p[index*3+1];rgb[n*3+2]=p[index*3+2];});
    check(createHash('sha256').update(rgb).digest('hex')===manifest.decoded_sha256[i],`Frame ${i} exactly matches converted asset`);
}
check(e.decoded_count()===96&&!e.failed(),'All frames decoded successfully');
check(e.tick(4099)===1&&e.frame_index()===95,'Last frame held until clip end');
check(e.tick(4100)===0&&e.finished(),'Completion at four seconds');
for(const ms of [4200,10000,600000,0,0xffffffff])check(e.tick(ms)===0&&e.decoded_count()===96,'Completed clip never restarts');
e.init(); e.tick(0); e.tick(2300);
check(e.frame_index()===55&&e.decoded_count()===2,'Delayed calls skip directly to current frame');
e.tick(2301);check(e.decoded_count()===2,'Repeated frame does not decode again');
check(e.tick(10000)===0,'Long stall releases normal display without catch-up');
e.init();e.tick(0xffffff00);check(e.tick(0x2e8)===1&&e.frame_index()===24,'millis rollover keeps elapsed time');
check(e.tick(0xea0)===0,'Rollover completes at four seconds');
const bad=[[0,9],[3,9],[1,9,1],[1,9]];
for(const data of bad){
    const out=new Uint8Array(memory.buffer,e.output_ptr(),2050);out.fill(173);
    new Uint8Array(memory.buffer,e.input_ptr(),8).set(data);
    check(e.decode(data.length,2)===0,'Malformed, zero, overflow, odd or incomplete RLE rejected');
    check(out[0]===173&&out[3]===173,'Decoder never writes beyond output bounds');
}
new Uint8Array(memory.buffer,e.input_ptr(),8).set([2,7]);
check(e.decode(2,2)===1,'Exact bounded run accepted');
e.init();e.tick(0);e.tick(2000);
e.display_frame(0,1,1,1);
const p=palette(),f=frame();
let matching=true;
for(let y=0;y<32;++y)for(let x=0;x<64;++x)for(let c=0;c<3;++c){
    const value=p[f[y*64+x]*3+c];
    matching&&=e.pixel(x,y,c)===value&&e.pixel(63-x,y+32,c)===value;
}
check(matching,'Actual controller writes top-down RGB and existing mirrored second panel');
check(e.swaps()===1&&e.accent_swaps()===1&&!e.early_writes(),'Pending buffers handed off before startup/strip writes');
e.display_frame(1,1,1,0);check(e.swaps()===1,'Boot frame takes display priority over external face');
for(const pair of [[0,0],[1,0],[0,1]]){
    e.display_frame(0,...pair,0);
    check(e.pixel(0,0,0)===13&&e.pixel(0,0,1)===29&&e.pixel(0,0,2)===71,'Null startup input restores normal camera RGB');
}
e.display_frame(1,0,0,0);check(e.swaps()===0,'External provider still owns normal display after startup');
console.log(JSON.stringify({checks,passed:true,frames_verified:96,duration_ms:4000,scope:'Compiled production player/decoder and HUB75 display writes with hardware stubs; not physical appearance qualification'},null,2));
