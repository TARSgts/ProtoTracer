#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// Seeed XIAO ESP32-S3 Sense (OV2640) pin mapping.
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      10
#define SIOD_GPIO_NUM      40
#define SIOC_GPIO_NUM      39

#define Y9_GPIO_NUM        48
#define Y8_GPIO_NUM        11
#define Y7_GPIO_NUM        12
#define Y6_GPIO_NUM        14
#define Y5_GPIO_NUM        16
#define Y4_GPIO_NUM        18
#define Y3_GPIO_NUM        17
#define Y2_GPIO_NUM        15
#define VSYNC_GPIO_NUM     38
#define HREF_GPIO_NUM      47
#define PCLK_GPIO_NUM      13

#ifdef LED_BUILTIN
#define FLASH_LED_PIN      LED_BUILTIN
#else
#define FLASH_LED_PIN      -1
#endif

static uint32_t frameCount = 0;
static uint32_t lastCaptureMs = 0;
static uint32_t lastPrintMs = 0;
static const uint32_t kCaptureIntervalMs = 70;   // ~14 FPS processing (higher per-frame quality)
static const uint32_t kPrintIntervalMs = 120;    // serial throttling

// Eye ROI in normalized frame coordinates; tune these for your mounting geometry.
static const float kRoiX0 = 0.20f;
static const float kRoiY0 = 0.20f;
static const float kRoiX1 = 0.80f;
static const float kRoiY1 = 0.65f;

struct EyeState {
    float rawMin = 999999.0f;
    float rawMax = 0.0f;
    bool isCalibrated = false;
    float blinkFiltered = 0.0f;
};

struct EyeFeature {
    bool tracked = false;
    int confidence = 0; // 0..100
    float centerX = 0.5f; // normalized full-frame coordinates
    float centerY = 0.5f;
    float contrast = 0.0f;
};

static EyeState leftEyeState;
static EyeState rightEyeState;
static EyeFeature latestFeatLeft;
static EyeFeature latestFeatRight;
static int latestBlinkLeft = 0;
static int latestBlinkRight = 0;
static int latestConfLeft = 0;
static int latestConfRight = 0;
static float latestRawLeft = 0.0f;
static float latestRawRight = 0.0f;

static httpd_handle_t streamHttpd = nullptr;
static httpd_handle_t controlHttpd = nullptr;
static const char* kApSsid = "XIAO-EYE-DEBUG";
static const char* kApPassword = "blinkdebug";

static inline float clamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float computeRoiRawDetail(const uint8_t* img, int width, int height, float nx0, float ny0, float nx1, float ny1) {
    int x0 = static_cast<int>(nx0 * width);
    int y0 = static_cast<int>(ny0 * height);
    int x1 = static_cast<int>(nx1 * width);
    int y1 = static_cast<int>(ny1 * height);

    if (x0 < 1) x0 = 1;
    if (y0 < 1) y0 = 1;
    if (x1 > width - 2) x1 = width - 2;
    if (y1 > height - 2) y1 = height - 2;

    uint32_t detailSum = 0;
    uint32_t sampleCount = 0;

    for (int y = y0; y < y1; y += 2) {
        int row = y * width;
        for (int x = x0; x < x1; x += 2) {
            uint8_t p = img[row + x];
            uint8_t l = img[row + x - 1];
            uint8_t r = img[row + x + 1];
            uint8_t u = img[row - width + x];
            uint8_t d = img[row + width + x];

            uint16_t gx = static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(l)))
                        + static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(r)));
            uint16_t gy = static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(u)))
                        + static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(d)));

            detailSum += static_cast<uint32_t>(gx + gy);
            sampleCount++;
        }
    }

    if (sampleCount == 0) {
        return 0.0f;
    }
    return static_cast<float>(detailSum) / static_cast<float>(sampleCount);
}

static float computePixelBoxRawDetail(const uint8_t* img, int width, int height, int x0, int y0, int x1, int y1) {
    if (!img || width < 4 || height < 4) return 0.0f;

    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    if (x0 < 1) x0 = 1;
    if (y0 < 1) y0 = 1;
    if (x1 > width - 2) x1 = width - 2;
    if (y1 > height - 2) y1 = height - 2;
    if (x1 <= x0 || y1 <= y0) return 0.0f;

    uint32_t detailSum = 0;
    uint32_t sampleCount = 0;

    for (int y = y0; y < y1; y += 1) {
        int row = y * width;
        for (int x = x0; x < x1; x += 1) {
            uint8_t p = img[row + x];
            uint8_t l = img[row + x - 1];
            uint8_t r = img[row + x + 1];
            uint8_t u = img[row - width + x];
            uint8_t d = img[row + width + x];

            uint16_t gx = static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(l)))
                        + static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(r)));
            uint16_t gy = static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(u)))
                        + static_cast<uint16_t>(abs(static_cast<int>(p) - static_cast<int>(d)));

            detailSum += static_cast<uint32_t>(gx + gy);
            sampleCount++;
        }
    }

    if (sampleCount == 0) return 0.0f;
    return static_cast<float>(detailSum) / static_cast<float>(sampleCount);
}

static float computeTrackedEyeRawDetail(
    const uint8_t* img,
    int width,
    int height,
    const EyeFeature& feat,
    float roiX0,
    float roiY0,
    float roiX1,
    float roiY1
) {
    // Fallback: broad ROI if eye isn't currently tracked.
    if (!feat.tracked) {
        return computeRoiRawDetail(img, width, height, roiX0, roiY0, roiX1, roiY1);
    }

    int cx = static_cast<int>(feat.centerX * width + 0.5f);
    int cy = static_cast<int>(feat.centerY * height + 0.5f);

    // Small local patch around tracked center gives much cleaner blink separation.
    const int patchHalfW = max(5, width / 16);   // ~20 px at QVGA
    const int patchHalfH = max(4, height / 18);  // ~13 px at QVGA

    int x0 = cx - patchHalfW;
    int y0 = cy - patchHalfH;
    int x1 = cx + patchHalfW;
    int y1 = cy + patchHalfH;

    return computePixelBoxRawDetail(img, width, height, x0, y0, x1, y1);
}

static EyeFeature trackEyeFeature(const uint8_t* img, int width, int height, float nx0, float ny0, float nx1, float ny1) {
    EyeFeature result;

    int x0 = static_cast<int>(nx0 * width);
    int y0 = static_cast<int>(ny0 * height);
    int x1 = static_cast<int>(nx1 * width);
    int y1 = static_cast<int>(ny1 * height);

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > width) x1 = width;
    if (y1 > height) y1 = height;
    if (x1 <= x0 + 2 || y1 <= y0 + 2) {
        return result;
    }

    uint32_t sum = 0;
    uint8_t minPix = 255;
    uint32_t area = static_cast<uint32_t>((x1 - x0) * (y1 - y0));

    for (int y = y0; y < y1; ++y) {
        int row = y * width;
        for (int x = x0; x < x1; ++x) {
            uint8_t p = img[row + x];
            sum += p;
            if (p < minPix) minPix = p;
        }
    }

    float meanPix = static_cast<float>(sum) / static_cast<float>(area);
    float contrast = meanPix - static_cast<float>(minPix);
    result.contrast = contrast;

    // Adaptive dark-feature threshold in the eye ROI.
    float threshold = static_cast<float>(minPix) + contrast * 0.55f;
    if (threshold < 4.0f) threshold = 4.0f;

    float weightedX = 0.0f;
    float weightedY = 0.0f;
    float weightSum = 0.0f;
    uint32_t darkCount = 0;

    for (int y = y0; y < y1; ++y) {
        int row = y * width;
        for (int x = x0; x < x1; ++x) {
            float p = static_cast<float>(img[row + x]);
            if (p <= threshold) {
                float w = (threshold - p) + 1.0f;
                weightedX += static_cast<float>(x) * w;
                weightedY += static_cast<float>(y) * w;
                weightSum += w;
                darkCount++;
            }
        }
    }

    if (weightSum > 0.0f) {
        result.centerX = (weightedX / weightSum) / static_cast<float>(width);
        result.centerY = (weightedY / weightSum) / static_cast<float>(height);
    }

    float darkRatio = static_cast<float>(darkCount) / static_cast<float>(area);
    float confContrast = clamp01(contrast / 38.0f);
    float confRatio = 1.0f - clamp01(abs(darkRatio - 0.10f) / 0.10f); // best around ~10% dark pixels
    float confidence = confContrast * 0.70f + confRatio * 0.30f;
    result.confidence = static_cast<int>(confidence * 100.0f + 0.5f);
    result.tracked = (result.confidence >= 25);

    return result;
}

static void drawRectGray(uint8_t* img, int width, int height, int x0, int y0, int x1, int y1, uint8_t value) {
    if (!img || width <= 0 || height <= 0) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    x0 = constrain(x0, 0, width - 1);
    x1 = constrain(x1, 0, width - 1);
    y0 = constrain(y0, 0, height - 1);
    y1 = constrain(y1, 0, height - 1);

    for (int x = x0; x <= x1; ++x) {
        img[y0 * width + x] = value;
        img[y1 * width + x] = value;
    }
    for (int y = y0; y <= y1; ++y) {
        img[y * width + x0] = value;
        img[y * width + x1] = value;
    }
}

static void drawCrossGray(uint8_t* img, int width, int height, int cx, int cy, int radius, uint8_t value) {
    if (!img || width <= 0 || height <= 0) return;
    int x0 = constrain(cx - radius, 0, width - 1);
    int x1 = constrain(cx + radius, 0, width - 1);
    int y0 = constrain(cy - radius, 0, height - 1);
    int y1 = constrain(cy + radius, 0, height - 1);
    cy = constrain(cy, 0, height - 1);
    cx = constrain(cx, 0, width - 1);

    for (int x = x0; x <= x1; ++x) img[cy * width + x] = value;
    for (int y = y0; y <= y1; ++y) img[y * width + cx] = value;
}

static void drawVerticalBarGray(
    uint8_t* img,
    int width,
    int height,
    int x,
    int yTop,
    int barWidth,
    int barHeight,
    float fill01,
    uint8_t frameValue,
    uint8_t fillValue
) {
    if (!img || width <= 0 || height <= 0 || barWidth <= 1 || barHeight <= 1) return;
    fill01 = clamp01(fill01);

    int x0 = constrain(x, 0, width - 1);
    int y0 = constrain(yTop, 0, height - 1);
    int x1 = constrain(x + barWidth - 1, 0, width - 1);
    int y1 = constrain(yTop + barHeight - 1, 0, height - 1);

    drawRectGray(img, width, height, x0, y0, x1, y1, frameValue);

    int innerX0 = x0 + 1;
    int innerY0 = y0 + 1;
    int innerX1 = x1 - 1;
    int innerY1 = y1 - 1;
    if (innerX1 <= innerX0 || innerY1 <= innerY0) return;

    int innerH = innerY1 - innerY0 + 1;
    int fillH = static_cast<int>(fill01 * innerH + 0.5f);
    if (fillH < 0) fillH = 0;
    if (fillH > innerH) fillH = innerH;

    int fillStartY = innerY1 - fillH + 1;
    for (int yy = fillStartY; yy <= innerY1; ++yy) {
        if (yy < innerY0 || yy > innerY1) continue;
        int row = yy * width;
        for (int xx = innerX0; xx <= innerX1; ++xx) {
            img[row + xx] = fillValue;
        }
    }
}

static void annotateEyeTrackingOverlay(uint8_t* img, int width, int height) {
    if (!img || width <= 0 || height <= 0) return;

    int roiX0 = static_cast<int>(kRoiX0 * width);
    int roiY0 = static_cast<int>(kRoiY0 * height);
    int roiX1 = static_cast<int>(kRoiX1 * width);
    int roiY1 = static_cast<int>(kRoiY1 * height);
    int roiMidX = (roiX0 + roiX1) / 2;

    roiX0 = constrain(roiX0, 0, width - 1);
    roiX1 = constrain(roiX1, 0, width - 1);
    roiY0 = constrain(roiY0, 0, height - 1);
    roiY1 = constrain(roiY1, 0, height - 1);
    roiMidX = constrain(roiMidX, 0, width - 1);

    // Global eye band and split marker.
    drawRectGray(img, width, height, roiX0, roiY0, roiX1, roiY1, 150);
    for (int y = roiY0; y <= roiY1; ++y) img[y * width + roiMidX] = 120;

    EyeFeature featLeft = trackEyeFeature(img, width, height, kRoiX0, kRoiY0, (kRoiX0 + kRoiX1) * 0.5f, kRoiY1);
    EyeFeature featRight = trackEyeFeature(img, width, height, (kRoiX0 + kRoiX1) * 0.5f, kRoiY0, kRoiX1, kRoiY1);

    // Draw fixed half-ROI boxes as fallback.
    drawRectGray(img, width, height, roiX0, roiY0, roiMidX, roiY1, 105);
    drawRectGray(img, width, height, roiMidX, roiY0, roiX1, roiY1, 105);

    // Draw dynamic tracking boxes when feature lock is good.
    if (featLeft.tracked) {
        int cx = static_cast<int>(featLeft.centerX * width + 0.5f);
        int cy = static_cast<int>(featLeft.centerY * height + 0.5f);
        drawRectGray(img, width, height, cx - 10, cy - 7, cx + 10, cy + 7, 255);
        drawCrossGray(img, width, height, cx, cy, 3, 255);
    }

    if (featRight.tracked) {
        int cx = static_cast<int>(featRight.centerX * width + 0.5f);
        int cy = static_cast<int>(featRight.centerY * height + 0.5f);
        drawRectGray(img, width, height, cx - 10, cy - 7, cx + 10, cy + 7, 255);
        drawCrossGray(img, width, height, cx, cy, 3, 255);
    }

    // Right-side eye state bars:
    // L-open, L-closed, R-open, R-closed.
    const int barW = 5;
    const int barH = max(14, height - 8);
    const int yTop = max(1, (height - barH) / 2);
    const int gap = 2;
    const int totalW = barW * 4 + gap * 3;
    const int startX = max(1, width - totalW - 2);

    float leftClosed = clamp01(static_cast<float>(latestBlinkLeft) / 1023.0f);
    float rightClosed = clamp01(static_cast<float>(latestBlinkRight) / 1023.0f);
    float leftOpen = 1.0f - leftClosed;
    float rightOpen = 1.0f - rightClosed;

    drawVerticalBarGray(img, width, height, startX + (barW + gap) * 0, yTop, barW, barH, leftOpen, 120, 230);
    drawVerticalBarGray(img, width, height, startX + (barW + gap) * 1, yTop, barW, barH, leftClosed, 120, 255);
    drawVerticalBarGray(img, width, height, startX + (barW + gap) * 2, yTop, barW, barH, rightOpen, 120, 230);
    drawVerticalBarGray(img, width, height, startX + (barW + gap) * 3, yTop, barW, barH, rightClosed, 120, 255);
}

static int computeEyeBlinkAnalog(EyeState& eye, float rawDetail) {
    if (!eye.isCalibrated) {
        eye.rawMin = rawDetail;
        eye.rawMax = rawDetail;
        eye.isCalibrated = true;
    }

    // Fast update when expanding range, very slow drift when inside range.
    if (rawDetail < eye.rawMin) eye.rawMin = rawDetail;
    else eye.rawMin += (rawDetail - eye.rawMin) * 0.002f;

    if (rawDetail > eye.rawMax) eye.rawMax = rawDetail;
    else eye.rawMax += (rawDetail - eye.rawMax) * 0.002f;

    float range = eye.rawMax - eye.rawMin;
    if (range < 1.0f) range = 1.0f;

    // More detail => eye open. Less detail => eyelid closed.
    float openNorm = clamp01((rawDetail - eye.rawMin) / range);
    float blinkNorm = 1.0f - openNorm;

    // Smooth to reduce chatter.
    eye.blinkFiltered += (blinkNorm - eye.blinkFiltered) * 0.28f;
    eye.blinkFiltered = clamp01(eye.blinkFiltered);

    return static_cast<int>(eye.blinkFiltered * 1023.0f + 0.5f);
}

static esp_err_t statusHandler(httpd_req_t* req) {
    char buf[320];
    int len = snprintf(
        buf,
        sizeof(buf),
        "{\"blinkL\":%d,\"blinkR\":%d,\"confL\":%d,\"confR\":%d,\"trackedL\":%d,\"trackedR\":%d,"
        "\"xL\":%.4f,\"yL\":%.4f,\"xR\":%.4f,\"yR\":%.4f,\"rawL\":%.2f,\"rawR\":%.2f}",
        latestBlinkLeft,
        latestBlinkRight,
        latestConfLeft,
        latestConfRight,
        latestFeatLeft.tracked ? 1 : 0,
        latestFeatRight.tracked ? 1 : 0,
        latestFeatLeft.centerX,
        latestFeatLeft.centerY,
        latestFeatRight.centerX,
        latestFeatRight.centerY,
        latestRawLeft,
        latestRawRight
    );
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, len);
}

static esp_err_t jpgHandler(httpd_req_t* req) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (fb->format == PIXFORMAT_GRAYSCALE) {
        annotateEyeTrackingOverlay(fb->buf, fb->width, fb->height);
    }

    uint8_t* jpgBuf = nullptr;
    size_t jpgLen = 0;
    bool jpegReady = frame2jpg(fb, 80, &jpgBuf, &jpgLen);

    esp_camera_fb_return(fb);

    if (!jpegReady || !jpgBuf || jpgLen == 0) {
        if (jpgBuf) free(jpgBuf);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    esp_err_t res = httpd_resp_send(req, reinterpret_cast<const char*>(jpgBuf), jpgLen);
    free(jpgBuf);
    return res;
}

static esp_err_t streamHandler(httpd_req_t* req) {
    static const char* kStreamContentType = "multipart/x-mixed-replace;boundary=frame";
    static const char* kStreamBoundary = "\r\n--frame\r\n";
    static const char* kStreamPart = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    httpd_resp_set_type(req, kStreamContentType);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    char partHeader[64];

    while (true) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) return ESP_FAIL;

        if (fb->format == PIXFORMAT_GRAYSCALE) {
            annotateEyeTrackingOverlay(fb->buf, fb->width, fb->height);
        }

        uint8_t* jpgBuf = nullptr;
        size_t jpgLen = 0;
        bool jpegReady = frame2jpg(fb, 75, &jpgBuf, &jpgLen);
        esp_camera_fb_return(fb);

        if (!jpegReady || !jpgBuf || jpgLen == 0) {
            if (jpgBuf) free(jpgBuf);
            return ESP_FAIL;
        }

        esp_err_t res = httpd_resp_send_chunk(req, kStreamBoundary, strlen(kStreamBoundary));
        if (res == ESP_OK) {
            size_t hlen = snprintf(partHeader, sizeof(partHeader), kStreamPart, static_cast<unsigned>(jpgLen));
            res = httpd_resp_send_chunk(req, partHeader, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, reinterpret_cast<const char*>(jpgBuf), jpgLen);
        }
        free(jpgBuf);

        if (res != ESP_OK) return res;
    }
}

static void startCameraWebServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 8;

    httpd_uri_t statusUri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = statusHandler,
        .user_ctx = nullptr
    };

    httpd_uri_t jpgUri = {
        .uri = "/jpg",
        .method = HTTP_GET,
        .handler = jpgHandler,
        .user_ctx = nullptr
    };

    httpd_uri_t streamUri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = streamHandler,
        .user_ctx = nullptr
    };

    if (httpd_start(&controlHttpd, &config) == ESP_OK) {
        httpd_register_uri_handler(controlHttpd, &statusUri);
        httpd_register_uri_handler(controlHttpd, &jpgUri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    if (httpd_start(&streamHttpd, &config) == ESP_OK) {
        httpd_register_uri_handler(streamHttpd, &streamUri);
    }
}

static bool initCamera() {
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_GRAYSCALE;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_QQVGA;
        config.jpeg_quality = 15;
        config.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed, error=0x%X\n", err);
        return false;
    }

    sensor_t* sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_brightness(sensor, 0);
        sensor->set_saturation(sensor, 0);
        sensor->set_framesize(sensor, config.frame_size);
    }

    return true;
}

void setup() {
    if (FLASH_LED_PIN >= 0) {
        pinMode(FLASH_LED_PIN, OUTPUT);
        digitalWrite(FLASH_LED_PIN, LOW);
    }

    Serial.begin(115200);
    delay(1200);

    Serial.println();
    Serial.println("XIAO ESP32-S3 Sense camera blink test firmware");
    Serial.printf("PSRAM detected: %s\n", psramFound() ? "yes" : "no");

    if (!initCamera()) {
        Serial.println("HALT: camera setup failed. Check module wiring/board profile.");
        while (true) {
            if (FLASH_LED_PIN >= 0) digitalWrite(FLASH_LED_PIN, HIGH);
            delay(100);
            if (FLASH_LED_PIN >= 0) digitalWrite(FLASH_LED_PIN, LOW);
            delay(900);
        }
    }

    Serial.println("Camera initialized OK.");
    Serial.println("Blink analog output enabled.");
    Serial.println("Output: blinkL/blinkR=0(open) ... 1023(closed), confL/confR=0..100%");
    Serial.println("Tracking flags/centroids: trackedL/trackedR + xL/yL/xR/yR");
    Serial.println("Use the first ~5 seconds to keep your eye open for auto-calibration.");

    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP(kApSsid, kApPassword);
    if (apOk) {
        startCameraWebServer();
        IPAddress ip = WiFi.softAPIP();
        Serial.printf("WiFi AP started: SSID=%s PASS=%s\n", kApSsid, kApPassword);
        Serial.printf("Live stream: http://%s:81/stream\n", ip.toString().c_str());
        Serial.printf("Snapshot:    http://%s/jpg\n", ip.toString().c_str());
        Serial.printf("Status JSON: http://%s/status\n", ip.toString().c_str());
    } else {
        Serial.println("[WARN] AP start failed; preview endpoints unavailable.");
    }
}

void loop() {
    uint32_t now = millis();
    if ((now - lastCaptureMs) < kCaptureIntervalMs) {
        delay(10);
        return;
    }
    lastCaptureMs = now;

    if (FLASH_LED_PIN >= 0) digitalWrite(FLASH_LED_PIN, HIGH);
    delay(20);
    camera_fb_t* fb = esp_camera_fb_get();
    if (FLASH_LED_PIN >= 0) digitalWrite(FLASH_LED_PIN, LOW);

    if (!fb) {
        Serial.println("[ERR] Capture failed (fb == nullptr)");
        return;
    }

    const float roiMidX = (kRoiX0 + kRoiX1) * 0.5f;
    EyeFeature featLeft = trackEyeFeature(fb->buf, fb->width, fb->height, kRoiX0, kRoiY0, roiMidX, kRoiY1);
    EyeFeature featRight = trackEyeFeature(fb->buf, fb->width, fb->height, roiMidX, kRoiY0, kRoiX1, kRoiY1);
    float rawLeft = computeTrackedEyeRawDetail(fb->buf, fb->width, fb->height, featLeft, kRoiX0, kRoiY0, roiMidX, kRoiY1);
    float rawRight = computeTrackedEyeRawDetail(fb->buf, fb->width, fb->height, featRight, roiMidX, kRoiY0, kRoiX1, kRoiY1);

    int blinkLeft = computeEyeBlinkAnalog(leftEyeState, rawLeft);
    int blinkRight = computeEyeBlinkAnalog(rightEyeState, rawRight);
    int confLeft = featLeft.confidence;
    int confRight = featRight.confidence;

    latestFeatLeft = featLeft;
    latestFeatRight = featRight;
    latestBlinkLeft = blinkLeft;
    latestBlinkRight = blinkRight;
    latestConfLeft = confLeft;
    latestConfRight = confRight;
    latestRawLeft = rawLeft;
    latestRawRight = rawRight;

    frameCount++;

    uint32_t nowPrint = millis();
    if ((nowPrint - lastPrintMs) >= kPrintIntervalMs) {
        lastPrintMs = nowPrint;
        Serial.printf(
            "frame=%lu blinkL=%d blinkR=%d confL=%d%% confR=%d%% trackedL=%d trackedR=%d xL=%.3f yL=%.3f xR=%.3f yR=%.3f rawL=%.2f rawR=%.2f minL=%.2f maxL=%.2f minR=%.2f maxR=%.2f size=%ux%u\n",
            static_cast<unsigned long>(frameCount),
            blinkLeft,
            blinkRight,
            confLeft,
            confRight,
            featLeft.tracked ? 1 : 0,
            featRight.tracked ? 1 : 0,
            featLeft.centerX,
            featLeft.centerY,
            featRight.centerX,
            featRight.centerY,
            rawLeft,
            rawRight,
            leftEyeState.rawMin,
            leftEyeState.rawMax,
            rightEyeState.rawMin,
            rightEyeState.rawMax,
            static_cast<unsigned>(fb->width),
            static_cast<unsigned>(fb->height)
        );
    }

    esp_camera_fb_return(fb);
}
