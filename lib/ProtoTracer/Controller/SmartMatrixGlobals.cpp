#include "SmartMatrixHUB75.h"

// Allocate shared matrix buffers.
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_APA_ALLOCATE_BUFFERS(apamatrix, kApaMatrixWidth, kApaMatrixHeight, kApaRefreshDepth, kApaDmaBufferRows, kApaPanelType, kApaMatrixOptions);

#if defined(ESP32)
  #ifdef USE_ADAFRUIT_GFX_LAYERS
SMLayerBackgroundGFX<RGB_TYPE(COLOR_DEPTH), kBackgroundLayerOptions> backgroundLayer(kMatrixWidth, kMatrixHeight);
SMLayerBackgroundGFX<RGB_TYPE(COLOR_DEPTH), kApaBackgroundLayerOptions> apaBackgroundLayer(kApaMatrixWidth, kApaMatrixHeight);
  #else
SMLayerBackground<RGB_TYPE(COLOR_DEPTH), kBackgroundLayerOptions> backgroundLayer(kMatrixWidth, kMatrixHeight);
SMLayerBackground<RGB_TYPE(COLOR_DEPTH), kApaBackgroundLayerOptions> apaBackgroundLayer(kApaMatrixWidth, kApaMatrixHeight);
  #endif
#else
  #ifdef USE_ADAFRUIT_GFX_LAYERS
static BACKGROUND_MEMSECTION RGB_TYPE(COLOR_DEPTH) backgroundLayerBitmap[2 * kMatrixWidth * kMatrixHeight];
static color_chan_t backgroundLayerColorCorrectionLUT[sizeof(RGB_TYPE(COLOR_DEPTH)) <= 3 ? 256 : 4096];
SMLayerBackgroundGFX<RGB_TYPE(COLOR_DEPTH), kBackgroundLayerOptions> backgroundLayer(
    backgroundLayerBitmap, kMatrixWidth, kMatrixHeight, backgroundLayerColorCorrectionLUT);

static BACKGROUND_MEMSECTION RGB_TYPE(COLOR_DEPTH) apaBackgroundLayerBitmap[2 * kApaMatrixWidth * kApaMatrixHeight];
static color_chan_t apaBackgroundLayerColorCorrectionLUT[sizeof(RGB_TYPE(COLOR_DEPTH)) <= 3 ? 256 : 4096];
SMLayerBackgroundGFX<RGB_TYPE(COLOR_DEPTH), kApaBackgroundLayerOptions> apaBackgroundLayer(
    apaBackgroundLayerBitmap, kApaMatrixWidth, kApaMatrixHeight, apaBackgroundLayerColorCorrectionLUT);
  #else
static BACKGROUND_MEMSECTION RGB_TYPE(COLOR_DEPTH) backgroundLayerBitmap[2 * kMatrixWidth * kMatrixHeight];
static color_chan_t backgroundLayerColorCorrectionLUT[sizeof(RGB_TYPE(COLOR_DEPTH)) <= 3 ? 256 : 4096];
SMLayerBackground<RGB_TYPE(COLOR_DEPTH), kBackgroundLayerOptions> backgroundLayer(
    backgroundLayerBitmap, kMatrixWidth, kMatrixHeight, backgroundLayerColorCorrectionLUT);

static BACKGROUND_MEMSECTION RGB_TYPE(COLOR_DEPTH) apaBackgroundLayerBitmap[2 * kApaMatrixWidth * kApaMatrixHeight];
static color_chan_t apaBackgroundLayerColorCorrectionLUT[sizeof(RGB_TYPE(COLOR_DEPTH)) <= 3 ? 256 : 4096];
SMLayerBackground<RGB_TYPE(COLOR_DEPTH), kApaBackgroundLayerOptions> apaBackgroundLayer(
    apaBackgroundLayerBitmap, kApaMatrixWidth, kApaMatrixHeight, apaBackgroundLayerColorCorrectionLUT);
  #endif
#endif
