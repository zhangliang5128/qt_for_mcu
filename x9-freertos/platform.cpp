/******************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Ultralite module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/
#include <platforminterface/rect.h>
#include <platforminterface/screen.h>

#include <platforminterface/drawingdevice.h>
#include <platforminterface/drawingengine.h>
#include <platforminterface/keys.h>
#include <platforminterface/platforminterface.h>

#include <platform/platform.h>
#include <platform/alloc.h>
#include <platform/singlepointtoucheventdispatcher.h>

#include <qul/eventqueue.h> // for input handling
#include <qul/singleton.h>  // for input handling (temperature)

//#include <ctime> //
 #include <cstring> // for memcpy
#include <cstdint>

// add by semidrive
#include <config.h>
#include <lk_wrapper.h>
#include "sdm_display.h"
#include "disp_data_type.h"
#include <g2dlite_api.h>

#define USE_HW_ACC 1


#define RES_G2D_G2D2 0x4362200A

#define DISPLAY_QT_TEMPLATE { \
    1,/*layer*/\
    0,/*layer_dirty*/\
    1,/*layer_en*/\
    COLOR_ARGB8888,/*fmt*/\
    {0,0,1920,720},/*x,y,w,h src */ \
    {0x0,0x0,0x0,0x0},/*y,u,v,a*/ \
    {7680,0,0,0},/*stride*/ \
    {0,0,1920,720},/*start*/ \
    {0,0,1920,720},/*dst*/ \
    0,/*ckey_en*/\
    0,/*ckey*/\
    0,/*alpha_en*/\
    0x0,/*alpha*/\
    1,/*z-order*/ \
    0/*security*/\
}

namespace Qul {
namespace Platform {

extern void *qul_malloc(std::size_t size);

// //![fastAlloc]
// static char qul_scratch_buffer[16 * 1024];

// char *StackAllocator::m_buffer = qul_scratch_buffer;
// char *StackAllocator::m_top = StackAllocator::m_buffer;
// int32_t StackAllocator::m_size = sizeof(qul_scratch_buffer);
// // ![fastAlloc]

//! [waitForBufferFlip]
static volatile bool waitingForBufferFlip = false;
static uint32_t idleTimeWaitingForDisplay = 0;

static int QT_DISPLAY_ID = SCREEN_1;

struct g2dlite {
    int index;
    addr_t reg_addr;
    uint32_t irq_num;
};

static sdm_display_t * m_sdm;
static struct sdm_buffer sdm_buf = DISPLAY_QT_TEMPLATE;
static struct sdm_post_config post_data;
static void *G2D = NULL;

static void waitForBufferFlip()
{
    // Has there already been a buffer flip?
    if (!waitingForBufferFlip)
        return;
    const uint64_t startTime = currentTimestamp();
    while (waitingForBufferFlip) {
        //waitForInterrupt(BUFFER_FLIP_INTERRUPT, 1);
    }

    idleTimeWaitingForDisplay = currentTimestamp() - startTime;
}

// Note: This line clearing waitingForBufferFlip will need to be moved to the
// actual interrupt handler for the display available on the target platform.
// It's needed to inform about when the buffer address used to scan out pixels
// to the display has been updated, making the buffer free in order to start
// drawing the next frame.
void LCD_BufferFlipInterruptHandler()
{
    waitingForBufferFlip = false;
}
//! [waitForBufferFlip]

//! [refreshInterrupt]
static volatile int refreshCount = 1;

// Note: This line incrementing the refreshCount will need to be moved to the
// actual interrupt handler for the display available on the target platform. It
// needs to be called once per vertical refresh of the display, to keep track of
// how many refreshes have happened between calls to presentFrame in order to
// support custom refresh intervals. On some implementations this can be done
// using a so called "line event".
void LCD_RefreshInterruptHandler()
{
    ++refreshCount;
}
//! [refreshInterrupt]

//! [framebuffer]
// Assuming we use 32bpp framebuffers
static const int BytesPerPixel = 4;
static unsigned char* framebuffer[2];//[BytesPerPixel * ScreenWidth * ScreenHeight];
static int backBufferIndex = 0;
//! [framebuffer]
static const int ScreenWidth = QUL_DEFAULT_SCREEN_WIDTH;
static const int ScreenHeight = QUL_DEFAULT_SCREEN_HEIGHT;

static unsigned char * temp;
static unsigned char * bgtemp;

//! [initializeDisplay]
void initializeDisplay(const PlatformInterface::Screen *)
{
    //initLcd(screen->size().width(), screen->size().height());
    struct list_node *head = sdm_get_display_list();
    list_for_every_entry(head, m_sdm, sdm_display_t, node) {
        if (m_sdm->handle->display_id == QT_DISPLAY_ID)
            break;
    }
    printf("QT display_id %d\n", m_sdm->handle->display_id);

    framebuffer[0] = (unsigned char*) qul_malloc(BytesPerPixel * ScreenWidth * ScreenHeight);
    framebuffer[1] = (unsigned char*) qul_malloc(BytesPerPixel * ScreenWidth * ScreenHeight);
    temp = (unsigned char*) qul_malloc(BytesPerPixel * 288 * 288);
    bgtemp = (unsigned char*) qul_malloc(BytesPerPixel * 1920 * 720);
    memset(bgtemp, 0xff, 1920 * 720*4);
}
//! [initializeDisplay]

//! [initializeHardware]
void initializeHardware()
{
    //printf("kyle initializeHardware begin\n");
    //init g2dlite()
    int ret;
    ret = hal_g2dlite_creat_handle(&G2D, RES_G2D_G2D2);
    if (!ret) {
        printf("g2dlite creat handle failed\n");
    }

    hal_g2dlite_init(G2D);
    printf("g2d->index 0x%x\n", ((struct g2dlite *)G2D)->index);

    Qul::PlatformInterface::init32bppRendering();
#if 0
    Replace the following code with actual code for your device.

    setup_system_clocks();
    setup_flash_memory();
    setup_sd_ram();
    setup_usart();
    setup_rnd_generator();
#endif
    //! [initializeHardware]
    // input tp
    // g2dlite
    // decoder

#if defined(__ICCARM__)
    //! [preloadingAssetsIAR]
#pragma section = "AssetDataPreload"
#pragma section = "AssetDataPreload_init"
    char *_preloadable_assetdata_src = (char *) (__section_begin("AssetDataPreload_init"));
    char *_preloadable_assetdata_dst_begin = (char *) (__section_begin("AssetDataPreload"));
    char *_preloadable_assetdata_dst_end = (char *) (__section_end("AssetDataPreload"));

    memcpy(_preloadable_assetdata_dst_begin,
           _preloadable_assetdata_src,
           (unsigned) _preloadable_assetdata_dst_end - (unsigned) _preloadable_assetdata_dst_begin);
    //! [preloadingAssetsIAR]
#else
    //! [preloadingAssets]
    // Preloading assets
    // extern unsigned char _preloadable_assetdata_src;
    // extern unsigned char _preloadable_assetdata_dst_begin;
    // extern unsigned char _preloadable_assetdata_dst_end;

    // memcpy(&_preloadable_assetdata_dst_begin,
    //        &_preloadable_assetdata_src,
    //        &_preloadable_assetdata_dst_end - &_preloadable_assetdata_dst_begin);
//! [preloadingAssets]
#endif
    //printf("kyle initializeHardware end\n");
}

#if defined(__ICCARM__)
int HAL_GetTick();
uint64_t stm32_timestamp();

/*
 * Note from the IAR C/C++ Development Guide:
 * To make the time and date functions work, you must implement the three functions
 * clock, time, and __getzone.
 */

// The number of times an internal timing event occurs per second
int const CLOCKS_PER_SECOND = 1000;

clock_t clock(void)
{
    QUL_UNUSED(CLOCKS_PER_SECOND);

    // This function should return a value, which after division by CLOCKS_PER_SECOND,
    // is the processor time in seconds.
    return (clock_t) HAL_GetTick();
}

__time32_t __time32(__time32_t *t)
{
    uint64_t timestamp = stm32_timestamp();
    // same timestamp as _gettimeofday
    __time32_t curtime = (__time32_t)(60 * (60 * 13 + 33) + timestamp / 1000);

    if (t)
        *t = curtime;

    return curtime;
}

char const *__getzone()
{
    // See <IAR>/src/lib/time/getzone.c for documentation
    // For Germany as a default timezone
    return ":GMT+1:GMT+2:0100:032502+0:102502+0";
}

__ATTRIBUTES char *_DstMalloc(size_t);
__ATTRIBUTES void _DstFree(char *);

char *_DstMalloc(size_t s)
{
    // Return a buffer that can hold the maximum number of DST entries of
    // of any timezone available on the device.
    // Each DST entry takes up a structure of 5 bytes plus regular alignment.
    // Instead of a static buffer a dynamically allocated memory can be used as well.

    // With the two entries shown above the required buffer size would be
    // 2 * (5 bytes size + 3 bytes alignment) = 16 bytes

    static char buffert[8 * 4];
    return buffert;
}

void _DstFree(char *p)
{
    // Nothing required here because of static buffer in _DstMalloc
    QUL_UNUSED(p);
}

#endif

//! [rand]
double rand()
{
    // Replace std::rand() by the proper call to the random number generator on your device, if available.
    const uint32_t number = std::rand();
    return number / (std::numeric_limits<uint32_t>::max() + 1.0);
}
//! [rand]

//! [currentTimestamp]
uint64_t currentTimestamp()
{
    uint32_t time = current_time();
    //printf("kyle currentTimestamp %d, %llu\n", time, (uint64_t)time);
    return time;
}
//! [currentTimestamp]

//! [nextUpdate]
static uint64_t nextUpdate = 0llu;

void scheduleEngineUpdate(uint64_t time)
{
    //printf("kyle scheduleEngineUpdate %llu\n", time);
    nextUpdate = time;
}
//! [nextUpdate]

//! [exec]
static uint64_t last_time = 0llu;
static  int frame = 0;
static int lastframe = 0;
static bool already_copy_source = false;
static bool already_copy_bg = false;
void exec()
{
    //printf("kyle exec start\n");
    last_time = currentTimestamp();
    while (true) {
        //printf("kyle exec while start\n");
        const uint64_t timestamp = currentTimestamp();

        //printf("kyle exec while %lld, %llu\n",timestamp, nextUpdate);
        if (timestamp >= nextUpdate) {
            // Handle deadline or pending events
            //printf("kyle exec while updateEngine start\n");
            Qul::PlatformInterface::updateEngine(timestamp);
        } else {
            // The core library has no pending actions.
            // The device may go to a sleep mode.
            //printf("kyle exec while sleep %llu\n", nextUpdate - timestamp);
            // wait_for_interrupt(nextUpdate - timestamp);
            thread_sleep(nextUpdate - timestamp);
        }
        frame++;
        frame = frame%3600;

        if (frame%360 == 0) {
            uint64_t cur = currentTimestamp();
            uint64_t gap = cur - last_time;
            if (gap == 0) {
                gap = 36;
            }
            float fps = 360.0f*1000/gap;
            printf("fps:%f\n", fps);
            last_time = cur;
        }
        //printf("kyle exec while end\n");
    }
}
//! [exec]

//! [frameBufferingType]
FrameBufferingType frameBufferingType(const PlatformInterface::Screen *)
{
    return FlippedDoubleBuffering;
}
//! [frameBufferingType]

//! [availableScreens]

Qul::PlatformInterface::Screen *availableScreens(size_t *screenCount)
{
    //printf("kyle availableScreens start %d, %d\n", ScreenWidth, ScreenHeight);
    *screenCount = 1;
    static PlatformInterface::Screen screen(PlatformInterface::Size(ScreenWidth, ScreenHeight));
    //printf("kyle availableScreens end\n");
    return &screen;
}
//! [availableScreens]

//! [asyncFunctions]
void waitUntilAsyncReadFinished(const void * /*begin*/, const void * /*end*/)
{
    // HW_SyncFramebufferForCpuAccess();
}

void flushCachesForAsyncRead(const void * /*addr*/, size_t /*length*/)
{
    // CleanInvalidateDCache_by_Addr(const_cast<void *>(addr), length);
}
//! [asyncFunctions]

//! [synchronizeAfterCpuAccess]
static bool framebufferAccessedByCpu = false;

#if USE_HW_ACC

//! [drawingEngine]
class SDRVDrawingEngine : public PlatformInterface::DrawingEngine
{
public:
    void blendTransformedImage(PlatformInterface::DrawingDevice *drawingDevice,
                               const PlatformInterface::Transform &transform,
                               const PlatformInterface::RectF &destinationRect,
                               const PlatformInterface::Texture &source,
                               const PlatformInterface::RectF &sourceRect,
                               const PlatformInterface::Rect &clipRect,
                               int sourceOpacity,
                               BlendMode blendMode) override
    {
        // if (frame%360 == 0) {
        //     printf("kyle destinationRect %f,%f,%f,%f\n", destinationRect.x(), destinationRect.y(),destinationRect.width(),destinationRect.height());
        //     printf("kyle sourceRect %f,%f,%f,%f\n", sourceRect.x(), sourceRect.y(),sourceRect.width(),sourceRect.height());
        //     printf("kyle clipRect %d,%d,%d,%d\n", clipRect.x(), clipRect.y(),clipRect.width(),clipRect.height());
        // }

        // if (!already_copy_bg) {
        //     memcpy(bgtemp, source.data(), 480*272*4);
        //     arch_clean_cache_range((addr_t)bgtemp, 480*272*4);
        //     already_copy_bg = true;
        // }

        fallbackDrawingEngine()->blendTransformedImage(drawingDevice,
                                                    transform,
                                                    destinationRect,
                                                    source,
                                                    sourceRect,
                                                    clipRect,
                                                    sourceOpacity,
                                                    blendMode);

        // hal_g2dlite_fastcopy(G2D, (addr_t)(bgtemp), 1920, 720, 1920 *4, (addr_t)(drawingDevice->bits()), 1920 * 4);
    }

    void blendImage(PlatformInterface::DrawingDevice *drawingDevice,
                                const PlatformInterface::Point &pos,
                                const PlatformInterface::Texture &source,
                                const PlatformInterface::Rect &sourceRect,
                                int sourceOpacity,
                                BlendMode blendMode) override
    {
        // if (frame == lastframe) {
        //     return;
        // }
        // lastframe = frame;

        if (frame%360 == 0) {
            //printf("kyle bg=%p, fg=%p\n", drawingDevice->bits(), source.data());
        }

        // printf("kyle pos %d,%d\n", pos.x(), pos.y());
        // printf("kyle sourceRect %d,%d,%d,%d\n", sourceRect.x(), sourceRect.y(),sourceRect.width(),sourceRect.height());
        if (!already_copy_source) {
            memcpy(temp, source.data(), 288*288*4);
            arch_clean_cache_range((addr_t)temp, 288*288*4);
            already_copy_source = true;
        }


        int x = sourceRect.x();
        int y = sourceRect.y();
        int w =  sourceRect.width();
        int h =  sourceRect.height();
        // fallbackDrawingEngine()->blendImage(drawingDevice, pos, source, sourceRect, sourceOpacity, blendMode);

        static struct g2dlite_input input;
        input.layer_num = 2;

        for (int i = 0; i < input.layer_num; i++) {
            struct g2dlite_input_cfg  *l = &input.layer[i];
            l->layer_en = 1;
            l->layer = i;
            l->fmt = COLOR_ARGB8888;
            l->zorder = i;

            l->ckey_en = 0;

            l->alpha = 255;

            if (i == 0) {
                l->blend = BLEND_PIXEL_NONE;
                l->addr[0] = (unsigned long) (drawingDevice->bits());
                l->src_x = pos.x();
                l->src_y = pos.y();
                l->src_w = w;
                l->src_h = h;
                l->src_stride[0] = drawingDevice->width()*4;

                l->dst_x = 0;// canvas :output x y
                l->dst_y = 0;
                l->dst_w = w;
                l->dst_h = h;
            } else {
                l->blend = BLEND_PIXEL_COVERAGE;
                l->addr[0] = (unsigned long) (temp);
                l->src_x = x;
                l->src_y = y;
                l->src_w = w;
                l->src_h = h;
                l->src_stride[0] = source.width()*4;

                l->dst_x = 0;// canvas :output x y
                l->dst_y = 0;
                l->dst_w = w;
                l->dst_h = h;
            }
        }

        input.output.width = w;
        input.output.height = h;
        input.output.fmt = COLOR_ARGB8888;
        input.output.addr[0] = (unsigned long)(drawingDevice->bits() + pos.y()*drawingDevice->width()*4 + pos.x()*4);
        //input.output.addr[0] = (unsigned long)(drawingDevice->bits());
        input.output.stride[0] = drawingDevice->width()*4;
        input.output.rotation = 0;
        hal_g2dlite_blend(G2D, &input);

    }

    void synchronizeForCpuAccess(PlatformInterface::DrawingDevice *drawingDevice,
                                 const PlatformInterface::Rect &rect) override
    {
        // HW_SyncFramebufferForCpuAccess();
        // if (frame%360 == 0) {
        //     printf("kyle synchronizeForCpuAccess %d,%d,%d,%d\n", rect.x(), rect.y(),rect.width(),rect.height());
        // }
        // framebufferAccessedByCpu = true;

        // unsigned char *backBuffer = framebuffer[backBufferIndex];
        // for (int i = 0; i < rect.height(); ++i) {
        //     unsigned char *pixels = backBuffer + (ScreenWidth * (rect.y() + i) + rect.x()) * BytesPerPixel;
        //     // CleanInvalidateDCache_by_Addr(pixels, rect.width() * BytesPerPixel);
        //     // add by semidrive
        //     arch_clean_cache_range((addr_t)pixels, rect.width() * BytesPerPixel);
        // }
    }
};
//! [drawingEngine]

#endif //USE_HW_ACC

static void synchronizeAfterCpuAccess(const PlatformInterface::Rect &rect)
{
    if (frame%360 == 0) {
        //printf("kyle synchronizeAfterCpuAccess %d,%d,%d,%d\n", rect.x(), rect.y(),rect.width(),rect.height());
    }
    if (framebufferAccessedByCpu) {
        unsigned char *backBuffer = framebuffer[backBufferIndex];
        for (int i = 0; i < rect.height(); ++i) {
            unsigned char *pixels = backBuffer + (ScreenWidth * (rect.y() + i) + rect.x()) * BytesPerPixel;
            // CleanInvalidateDCache_by_Addr(pixels, rect.width() * BytesPerPixel);
            // add by semidrive
            arch_clean_cache_range((addr_t)pixels, rect.width() * BytesPerPixel);
        }
    }
}
//! [synchronizeAfterCpuAccess]

//! [beginFrame]
static int requestedRefreshInterval = 1;
PlatformInterface::DrawingDevice *beginFrame(const PlatformInterface::Screen *,
                                             int /*layer*/,
                                             const PlatformInterface::Rect &,
                                             int refreshInterval)
{
#if USE_HW_ACC
    static SDRVDrawingEngine drawingEngine;
#else
    static PlatformInterface::DrawingEngine drawingEngine;
#endif
    //printf("kyle beginFrame start %d\n", backBufferIndex);
    requestedRefreshInterval = refreshInterval;

    // Wait until the back buffer is free, i.e. no longer held by the display
    //waitForBufferFlip();

    // A pointer to the back buffer
    uchar *bits = framebuffer[backBufferIndex];
    static PlatformInterface::DrawingDevice buffer = {Qul::PixelFormat_ARGB32,
                                                      PlatformInterface::Size(ScreenWidth, ScreenHeight),
                                                      bits,
                                                      ScreenWidth * BytesPerPixel,
                                                      &drawingEngine};

    buffer.setBits(bits);
    //printf("kyle beginFrame end\n");
    return &buffer;
}
//! [beginFrame]

//! [endFrame]
void endFrame(const PlatformInterface::Screen *) {}
//! [endFrame]

//! [waitForRefreshInterval]
static void waitForRefreshInterval()
{
    //printf("kyle waitForRefreshInterval start\n");
    if (refreshCount < requestedRefreshInterval) {
        uint64_t startTime = currentTimestamp();
        while (refreshCount < requestedRefreshInterval) {
            //waitForInterrupt(REFRESH_INTERRUPT, 1);
        }
        idleTimeWaitingForDisplay += currentTimestamp() - startTime;
    }
    //printf("kyle waitForRefreshInterval end\n");
    refreshCount = 0;
}
//! [waitForRefreshInterval]

//! [presentFrame]
FrameStatistics presentFrame(const PlatformInterface::Screen *, const PlatformInterface::Rect &rect)
{
    // HW_SyncFramebufferForCpuAccess();
    //printf("kyle presentFrame start\n");
    synchronizeAfterCpuAccess(rect);
    framebufferAccessedByCpu = false;
    //printf("kyle presentFrame 111\n");
    //! [frameSkipCompensation]
    FrameStatistics stats;
    stats.refreshDelta = refreshCount - requestedRefreshInterval;
    //waitForRefreshInterval();
    static const int RefreshRate = 60; // screen refresh rate in Hz
    stats.remainingBudget = idleTimeWaitingForDisplay + stats.refreshDelta * int(1000.0f / RefreshRate);
    //! [frameSkipCompensation]
    //printf("kyle presentFrame sdm_post start, %d\n", backBufferIndex);
    waitingForBufferFlip = true;
    idleTimeWaitingForDisplay = 0;

    //arch_clean_cache_range((addr_t)framebuffer[backBufferIndex], ScreenWidth*ScreenHeight*BytesPerPixel);

    // Now we can update the framebuffer address
    // LCD_SetBufferAddr(framebuffer[backBufferIndex]);
    //printf("kyle sdm post bits 0x%x, 0x%x, 0x%x, 0x%x\n", framebuffer[backBufferIndex][0], framebuffer[backBufferIndex][1], framebuffer[backBufferIndex][2], framebuffer[backBufferIndex][3]);
    sdm_buf.addr[0] = (unsigned long)framebuffer[backBufferIndex];
    post_data.bufs             = &sdm_buf;
    post_data.n_bufs           = 1;
    sdm_post(m_sdm->handle, &post_data);
    //printf("kyle presentFrame sdm_post end\n");
    // Now the front and back buffers are swapped
    if (backBufferIndex == 0)
        backBufferIndex = 1;
    else
        backBufferIndex = 0;
    //printf("kyle presentFrame end\n");
    waitingForBufferFlip = false;
    return stats;
}
//! [presentFrame]
#if 0
namespace Private {

//! [singleTouchInputDispatcherEventQueue]
class SinglePointTouchEventQueue : public Qul::EventQueue<Qul::Platform::SinglePointTouchEvent>
{
public:
    void onEvent(const Qul::Platform::SinglePointTouchEvent &event) override { touchEventDispatcher.dispatch(event); }

private:
    Qul::Platform::SinglePointTouchEventDispatcher touchEventDispatcher;
};

static SinglePointTouchEventQueue touchEventQueue;
//! [singleTouchInputDispatcherEventQueue]

//! [singleTouchInputDispatcherEventGetter]
Qul::Platform::SinglePointTouchEvent getTouchEvent()
{
    static Qul::Platform::SinglePointTouchEvent event;
    int x = 0, y = 0;
    bool pressed = false;

    // Here would be platform specific code to fetch touch data and store it to above x, y and
    // pressed variables or directly to the event variable.

    event.x = x;
    event.y = y;
    event.pressed = pressed;
    event.timestamp = Qul::Platform::currentTimestamp();

    return event;
}
//! [singleTouchInputDispatcherEventGetter]

//! [singleTouchInputDispatcherISR]
void touchISR()
{
    touchEventQueue.postEvent(getTouchEvent());
}
//! [singleTouchInputDispatcherISR]

//! [singleTouchInputEventQueue]
class TouchPointEventQueue : public Qul::EventQueue<Qul::PlatformInterface::TouchPoint>
{
public:
    void onEvent(const Qul::PlatformInterface::TouchPoint &point) override
    {
        Qul::PlatformInterface::handleTouchEvent(nullptr, /* Primary screen will be used */
                                                 Qul::Platform::currentTimestamp(),
                                                 &point,
                                                 1); /* only one touch point */
    }
};

static TouchPointEventQueue touchPointQueue;

void touchEventISR()
{
    static Qul::PlatformInterface::TouchPoint point;

    // Here would be code to fetch platform specific touch data and store it into point variable.
    // To keep the example simple we just send constant data.
    point.id = 0;
    point.state = Qul::PlatformInterface::TouchPoint::Stationary;
    point.positionX = 100;
    point.positionY = 100;
    point.areaX = 1.f;
    point.areaY = 1.f;
    point.pressure = 0.1f;
    point.rotation = 1.f;
    touchPointQueue.postEvent(point);
}
//! [singleTouchInputEventQueue]

//! [multiTouchInput]
// These are just dummy handles for the example
Qul::PlatformInterface::Screen primaryScreenHandle;
Qul::PlatformInterface::Screen secondaryScreenHandle;

// multi-touch with 2 touch points
const int numTouchPoints = 2;
struct TouchPoints
{
    Qul::PlatformInterface::TouchPoint points[numTouchPoints];
};

class PlatformTouchEventQueue : public Qul::EventQueue<TouchPoints>
{
public:
    PlatformTouchEventQueue(Qul::PlatformInterface::Screen *s)
        : screen(s)
    {}

    void onEvent(const TouchPoints &p) override
    {
        Qul::PlatformInterface::handleTouchEvent(screen, Qul::Platform::currentTimestamp(), p.points, numTouchPoints);
    }

private:
    Qul::PlatformInterface::Screen *screen;
};

static PlatformTouchEventQueue primaryScreenTouchEventQueue(&primaryScreenHandle);
static PlatformTouchEventQueue secondaryScreenTouchEventQueue(&secondaryScreenHandle);

void primaryTouchISR()
{
    static TouchPoints p;

    // Here would be platform specific code to fetch touch data for primary screen. To keep the
    // example simple we just send constant data.

    for (int i = 0; i < numTouchPoints; i++) {
        p.points[i].id = i;
        p.points[i].state = Qul::PlatformInterface::TouchPoint::Stationary;
        p.points[i].positionX = 100;
        p.points[i].positionY = 100;
        p.points[i].areaX = 1.f;
        p.points[i].areaY = 1.f;
        p.points[i].pressure = 0.1f;
        p.points[i].rotation = 1.f;
    }
    primaryScreenTouchEventQueue.postEvent(p);
}

void secondaryTouchISR()
{
    static TouchPoints p;

    // Here would be platform specific code to fetch touch data for secondary screen. To keep the
    // example simple we just send constant data.

    for (int i = 0; i < numTouchPoints; i++) {
        p.points[i].id = i;
        p.points[i].state = Qul::PlatformInterface::TouchPoint::Stationary;
        p.points[i].positionX = 150;
        p.points[i].positionY = 150;
        p.points[i].areaX = 0.5f;
        p.points[i].areaY = 0.5f;
        p.points[i].pressure = 0.5f;
        p.points[i].rotation = 2.f;
    }
    secondaryScreenTouchEventQueue.postEvent(p);
}
//! [multiTouchInput]

//! [keyInput]
struct KeyboardEvent
{
    uint64_t timestamp;
    Qul::PlatformInterface::KeyEventType type;
    int key;
    unsigned int nativeScanCode;
    unsigned int modifiers;
    char *textUTF;
    bool autorepeat;
};

class PlatformKeyboardEventQueue : public Qul::EventQueue<KeyboardEvent>
{
    void onEvent(const KeyboardEvent &event) override
    {
        Qul::PlatformInterface::handleKeyEvent(event.timestamp,
                                               event.type,
                                               event.key,
                                               event.nativeScanCode,
                                               event.modifiers,
                                               event.textUTF,
                                               event.autorepeat);
    }
};

static PlatformKeyboardEventQueue keyboardEventQueue;

void keyboardISR()
{
    // Here would be platform specific code to fetch data from the keyboard.
    static KeyboardEvent event;
    event.timestamp = Qul::Platform::currentTimestamp();
    event.type = Qul::PlatformInterface::KeyPressEvent;
    event.key = Qul::PlatformInterface::Key_A;
    event.nativeScanCode = 0;
    event.modifiers = Qul::PlatformInterface::NoKeyboardModifier;
    event.textUTF = (char *) "A"; /* UTF-8 representation */
    event.autorepeat = false;

    keyboardEventQueue.postEvent(event);
}
//! [keyInput]

//! [customInputTemperatureInterface]
struct TemperatureInput : public Qul::Singleton<TemperatureInput>, public Qul::EventQueue<double>
{
    Qul::Signal<void(double value)> temperatureChanged;

    enum Unit { Celsius, Fahrenheit, Kelvin };
    Qul::Property<TemperatureInput::Unit> unit;
    void setUnit(TemperatureInput::Unit newUnit)
    {
        switch (unit.value()) {
        case Celsius:
            if (newUnit == Celsius)
                return;
            newUnit == Kelvin ? cachedValue += 273.15 : cachedValue = cachedValue * 1.8 + 32.0;
            break;
        case Fahrenheit:
            if (newUnit == Fahrenheit)
                return;
            newUnit == Kelvin ? cachedValue = (cachedValue - 32.0) / 1.8 + 273.15
                              : cachedValue = (cachedValue - 32.0) / 1.8;
            break;
        case Kelvin:
            if (newUnit == Kelvin)
                return;
            newUnit == Celsius ? cachedValue -= 273.15 : cachedValue = (cachedValue - 273.15) * 1.8 + 32.0;
            break;
        }
        unit.setValue(newUnit);
        temperatureChanged(cachedValue);
    }

    void onEvent(const double &temperature) override
    {
        temperatureChanged(temperature);
        cachedValue = temperature;
    }

private:
    // Create friendship for a base class to be able to access private constructor
    friend class Qul::Singleton<TemperatureInput>;
    TemperatureInput() {}
    TemperatureInput(const TemperatureInput &);
    TemperatureInput &operator=(const TemperatureInput &);

    double cachedValue;
};
//! [customInputTemperatureInterface]

//! [customInputTemperatureSend]
void temperatureSensorISR()
{
    // Here would be platform specific code to read temperature sensor value.

    switch (TemperatureInput::instance().unit.value()) {
    case TemperatureInput::Celsius:
        TemperatureInput::instance().postEvent(30.0);
        break;
    case TemperatureInput::Fahrenheit:
        TemperatureInput::instance().postEvent(86.0);
        break;
    case TemperatureInput::Kelvin:
        TemperatureInput::instance().postEvent(303.15);
        break;
    default:
        assert(!"Unknown temperature unit requested!");
    }
}
//! [customInputTemperatureSend]

//! [customInputKnobEvent]
struct RotaryKnobInputEvent : public Qul::Object
{
    enum Action : int { NudgeDown, NudgeUp, NudgeLeft, NudgeRight, Push, Back, RotateLeft, RotateRight };

    enum ActionState : int { ActionStart, ActionRepeat, ActionStop };

    RotaryKnobInputEvent(Action action, ActionState state)
        : action(action)
        , actionState(state)
    {}

    Action action;
    ActionState actionState;
};
//! [customInputKnobEvent]

//! [customInputKnobInterface]
struct RotaryKnobInput : public Qul::Singleton<RotaryKnobInput>, public Qul::EventQueue<RotaryKnobInputEvent>
{
    Qul::Signal<void(int action, int state)> rotaryKnobEvent;

    void onEvent(const RotaryKnobInputEvent &event) override { rotaryKnobEvent(event.action, event.actionState); }

private:
    // Create friendship for a base class to be able to access private constructor
    friend class Qul::Singleton<RotaryKnobInput>;
    RotaryKnobInput() {}
    RotaryKnobInput(const RotaryKnobInput &);
    RotaryKnobInput &operator=(const RotaryKnobInput &);
};
//! [customInputKnobInterface]

//! [customInputKnobISR]
void rotaryKnobISR()
{
    // Here would be platform specific code to read state from rotary knob device.

    static int action = RotaryKnobInputEvent::ActionStart;
    static int state = RotaryKnobInputEvent::NudgeDown;

    RotaryKnobInput::instance().postEvent(
        RotaryKnobInputEvent(RotaryKnobInputEvent::Action(action), RotaryKnobInputEvent::ActionState(state)));

    if (++state > RotaryKnobInputEvent::ActionStop) {
        state = RotaryKnobInputEvent::ActionStart;

        if (++action > RotaryKnobInputEvent::RotateRight)
            action = RotaryKnobInputEvent::NudgeDown;
    }
}
//! [customInputKnobISR]

} // namespace Private

#endif
} // namespace Platform
} // namespace Qul
