#include "StartupAnimation.h"
static StartupAnimation player;
static uint8_t decodeOutput[2050];
static uint8_t decodeInput[8];
extern "C" {
void init() { player = StartupAnimation(); }
int tick(uint32_t ms) { return player.Update(ms); }
int frame_index() { return player.GetFrameIndex(); }
int decoded_count() { return player.GetDecodedFrames(); }
int finished() { return player.IsFinished(); }
int failed() { return player.HasFailed(); }
const uint8_t* frame_ptr() { return player.GetFrame(); }
const uint8_t* palette_ptr() { return StartupClip::kPalette; }
uint8_t* input_ptr() { return decodeInput; }
uint8_t* output_ptr() { return decodeOutput; }
int decode(int size,int pixels) { return StartupAnimation::DecodeRle(decodeInput,size,decodeOutput+1,pixels); }
}

// Minimal display hardware substitutes. The controller body below is the
// production implementation; this checks its actual writes and buffer handoff.
struct rgb24 { uint8_t red=0,green=0,blue=0; rgb24(){} rgb24(uint16_t r,uint16_t g,uint16_t b):red(r),green(g),blue(b){} };
struct Color { uint8_t R=13,G=29,B=71; };
struct IPixelGroup { Color color; Color* GetColor(int){return &color;} };
struct Camera { IPixelGroup pixels; IPixelGroup* GetPixelGroup(){return &pixels;} };
struct CameraManager { Camera c[3]; Camera* ptrs[3]={&c[0],&c[1],&c[2]}; Camera** GetCameras(){return ptrs;} };
struct Controller {
    CameraManager* cameras;
    uint8_t maxBrightness=0,maxAccentBrightness=0,brightness=20,accentBrightness=20;
    bool isOn=true;
    Controller(CameraManager* c,uint8_t,uint8_t):cameras(c){}
    virtual void Initialize(){} virtual void Display(){} virtual void SetBrightness(uint8_t){} virtual void SetAccentBrightness(uint8_t){}
    void UpdateBrightness(){}
};
struct Layer {
    rgb24 pixels[4096]; int swaps=0,pendingReads=0,writesBeforeHandoff=0;
    bool pending=false;
    void drawPixel(int x,int y,rgb24 c){ if(pending)++writesBeforeHandoff; pixels[y*64+x]=c; }
    void swapBuffers(bool=false){ ++swaps; }
    bool isSwapPending(){ if(pending){++pendingReads; if(pendingReads>1)pending=false;return true;} return false; }
};
struct Matrix {
    int brightness=0;
    void addLayer(Layer*){} void begin(){} void setRefreshRate(int){} void setBrightness(int b){brightness=b;}
    bool getdmaBufferUnderrunFlag(){return false;} bool getRefreshRateLoweredFlag(){return false;}
    int getRefreshRate(){return 240;}
};
struct SerialStub { template<class T>void print(T){} template<class T>void println(T){} } Serial;
uint32_t millis(){return 0;}
#define F(x) x
#define SMARTMATRIX_ALLOCATE_BUFFERS(name,...) Matrix name
#define SMARTMATRIX_APA_ALLOCATE_BUFFERS(name,...) Matrix name
#define SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(name,...) Layer name
static constexpr int kApaMatrixWidth=176;
// PRODUCTION_CONTROLLER_HEADER
// PRODUCTION_CONTROLLER_BODY
static CameraManager testCameras;
static HUB75Controller display(&testCameras,20,20);
extern "C" {
void display_frame(int external,int useFrame,int usePalette,int pending) {
    backgroundLayer=Layer(); apaBackgroundLayer=Layer();
    backgroundLayer.pending=pending; apaBackgroundLayer.pending=pending;
    display.SetExternalFrameProvider(external);
    display.SetStartupFrame(useFrame ? player.GetFrame() : nullptr,usePalette ? StartupClip::kPalette : nullptr);
    display.Display();
}
int pixel(int x,int y,int c){rgb24 p=backgroundLayer.pixels[y*64+x];return c==0?p.red:c==1?p.green:p.blue;}
int swaps(){return backgroundLayer.swaps;}
int early_writes(){return backgroundLayer.writesBeforeHandoff+apaBackgroundLayer.writesBeforeHandoff;}
int accent_swaps(){return apaBackgroundLayer.swaps;}
}
