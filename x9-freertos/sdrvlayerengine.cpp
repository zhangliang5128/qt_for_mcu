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
#include <platforminterface/platforminterface.h>

#include <platform/platform.h>
#include <platform/alloc.h>
#include <platform/mem.h>

#include "sdrvlayerengine.h"

#include <cstdio>

namespace Qul {
namespace Platform {

#define PIXEL_GPU_LIMIT 1000

extern volatile unsigned int currentFrame;
ScreenLayerVecMap SDRVLayerEngine::mScreenRootLayerVecMap;
unsigned char* SDRVLayerEngine::rootFrameBuffer[2] = {NULL, NULL};
int SDRVLayerEngine::rootFrameBufferIndex = 0;
static bool already_copy_source = false;

static int toHwPixelFormat(Qul::PlatformInterface::LayerEngine::ColorDepth depth)
{
    switch (depth) {
    case Qul::PlatformInterface::LayerEngine::Bpp32Alpha:
        return COLOR_ABGR8888;
    case Qul::PlatformInterface::LayerEngine::Bpp32:
        return COLOR_RGB888;
    case Qul::PlatformInterface::LayerEngine::Bpp16:
        return COLOR_RGB565;
    default:
        printf("toHwPixelFormat Unsupported colorDepth %d\n", depth);
        return ERROR_STATUS;
    }
}

static Qul::PixelFormat toPixelFormat(Qul::PlatformInterface::LayerEngine::ColorDepth depth)
{
    switch (depth) {
    case Qul::PlatformInterface::LayerEngine::Bpp32Alpha:
        return Qul::PixelFormat_ARGB32;
    case Qul::PlatformInterface::LayerEngine::Bpp32:
        return Qul::PixelFormat_RGB32;
    case Qul::PlatformInterface::LayerEngine::Bpp16:
        return Qul::PixelFormat_RGB16;
    default:
        printf("toPixelFormat Unsupported colorDepth %d\n", depth);
        return Qul::PixelFormat_Invalid;
    }
}

static int toHwPixelFormatFromPixelFormat(Qul::PixelFormat format)
{
    switch (format) {
        case Qul::PixelFormat_ARGB32:
        case Qul::PixelFormat_ARGB32_Premultiplied:
            return COLOR_ABGR8888;
        case Qul::PixelFormat_RGB32:
            return COLOR_RGB888;
        case Qul::PixelFormat_RGB16:
            return COLOR_RGB565;
        default:
            printf("toHwPixelFormatFromPixelFormat Unsupported pixel format %d\n", format);
            return ERROR_STATUS;
    }
}

static int bytesPerPixelFromPixelFormat(Qul::PixelFormat format)
{
    switch (format) {
        case Qul::PixelFormat_ARGB32:
        case Qul::PixelFormat_ARGB32_Premultiplied:
        case Qul::PixelFormat_RGB32:
            return FOUR_BIT;
        case Qul::PixelFormat_RGB16:
            return TWO_BIT;
        default:
            printf("bytesPerPixelFromPixelFormat Unsupported pixel format %d\n", format);
            return ERROR_STATUS;
    }
}

/*The platform might decide exactly which pixel format to use for a given color depth and alpha channel configuration.*/
static int bytesPerPixelFromColorDepth(Qul::PlatformInterface::LayerEngine::ColorDepth colorDepth)
{
    switch (colorDepth) {
    case Qul::PlatformInterface::LayerEngine::Bpp16:
        return TWO_BIT;
    case Qul::PlatformInterface::LayerEngine::Bpp24:
        return THREE_BIT;
    case Qul::PlatformInterface::LayerEngine::Bpp32:
    case Qul::PlatformInterface::LayerEngine::Bpp32Alpha:
        return FOUR_BIT;
    default:
        printf("bytesPerPixel Unsupported colorDepth %d\n", colorDepth);
        return DEFAULT_STATUS;
    }
}
static int bytesPerPixelFromHwPixelFormat(int hwFmt)
{
    switch (hwFmt) {
    case COLOR_ABGR8888:
        return FOUR_BIT;
    case COLOR_RGB888:
        return THREE_BIT;
    case COLOR_RGB565:
    case PixelFormat_RGB16:
        return TWO_BIT;
    default:
        printf("bytesPerPixelFromHwPixelFormat Unsupported pixel format %d\n", hwFmt);
        return DEFAULT_STATUS;
    }
}

// if root, use dc
// else , use g2d
SDRVHardwareLayer::SDRVHardwareLayer(const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                                     const Qul::PlatformInterface::Size &s,
                                     int hwPixelFormat,
                                     SDRVLayerType t,
                                     SDRVHardwareLayer* pl)
    : m_type(t)
    , m_parentlayer(pl)
    , m_isRootLayer(pl ? false : true)
    , m_hwPixelFormat(hwPixelFormat)
    , m_zorder(p.z)
{
    //printf("SDRV SDRVLayerEngine SDRVHardwareLayer start %d,%d,%d,%d\n", hwPixelFormat, t, isRoot(), p.z);
    {
        // DC
        if (p.z == 0) 
            m_dcLayer = DISPLAY_QT_LAYER_0;
        else 
            m_dcLayer = DISPLAY_QT_LAYER_1;

        m_dcLayer.fmt = hwPixelFormat;
    }

    {
        // G2D
        if (p.z == 0) 
            m_g2dLayer = G2D_QT_LAYER_0;
        else 
            m_g2dLayer = G2D_QT_LAYER_1;
        
        m_g2dLayer.fmt = hwPixelFormat;

    }
    updateProperties(p, s);
}

void SDRVHardwareLayer::updateProperties(const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                        const Qul::PlatformInterface::Size &s)
{
    saveLayerPropertise(p,s);

    //for DC
    if (isRootLayer()) {
        setDCLayerProperties(0,0,0,0,p,s);
        setG2dLayerProperties(0,0,p,s);
    }
    else 
        setG2dLayerProperties(0,0,p,s);

}

void SDRVHardwareLayer::saveLayerPropertise(const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                                            const Qul::PlatformInterface::Size &s)
{
    m_size = s;
    m_props = p;
    m_zorder = p.z;
    //printf("SDRV SDRVLayerEngine updateProperties size %d,%d,%d,%d\n", p.position.x(), p.position.x(), s.width(), s.height());
    //printf("SDRV SDRVLayerEngine updateProperties alpha=%f, enable=%d\n", p.opacity, p.enabled);

}

void SDRVHardwareLayer::setDCLayerProperties(const int start_x,
                                             const int start_y,
                                             const int src_x,
                                             const int src_y,
                                             const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                                             const Qul::PlatformInterface::Size &s)
{
    m_dcLayer.layer_en = p.enabled;
    m_dcLayer.start.x = 0;
    m_dcLayer.start.y = 0;
    m_dcLayer.start.w = s.width();
    m_dcLayer.start.h = s.height();
    m_dcLayer.src.x = 0;
    m_dcLayer.src.y = 0;
    m_dcLayer.src.w = s.width();
    m_dcLayer.src.h = s.height();
    //TODO: ALPHA_EN
    if (p.opacity > 0.00001f && p.opacity < 0.99999f)
        m_dcLayer.alpha_en = 1;
    else
        m_dcLayer.alpha_en = 0;
    
    m_dcLayer.alpha =  int(std::round(0xff * p.opacity));
    m_dcLayer.dst.x = p.position.x();
    m_dcLayer.dst.y = p.position.y();
    m_dcLayer.dst.w = s.width();
    m_dcLayer.dst.h = s.height();
    m_dcLayer.src_stride[0] = s.width()*bytesPerPixelFromHwPixelFormat(m_dcLayer.fmt);
}


//for g2d blend child
void SDRVHardwareLayer::setG2dLayerProperties(const int src_x,
                                              const int src_y,
                                              const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                                              const Qul::PlatformInterface::Size &s)
{
    m_g2dLayer.layer_en = p.enabled;
    m_g2dLayer.src.x = 0;
    m_g2dLayer.src.y = 0;
    m_g2dLayer.src.w = s.width();
    m_g2dLayer.src.h = s.height();
    m_g2dLayer.dst.x = p.position.x();
    m_g2dLayer.dst.y = p.position.y();
    m_g2dLayer.dst.w = s.width();
    m_g2dLayer.dst.h = s.height();
    m_g2dLayer.alpha = int(std::round(0xff * p.opacity));
    if (p.opacity > 0.00001f && p.opacity < 0.99999f){
        // printf("p.opacity > 0.00001f && p.opacity < 0.99999f---------------\r\n");
    }
        
        // m_g2dLayer.blend = BLEND_PIXEL_NONE;
    // else
    m_g2dLayer.blend = BLEND_PIXEL_COVERAGE;

    m_g2dLayer.src_stride[0] = s.width()*bytesPerPixelFromHwPixelFormat(m_g2dLayer.fmt);
}

SDRVHardwareLayer::~SDRVHardwareLayer() {}

int SDRVHardwareLayer::setHwLayerBuffer(const unsigned char * buf, int stride)
{
    //printf("SDRV SDRVLayerEngine setHwLayerBuffer %p,%d \n", buf,stride);

    //for dc
    {
        m_dcLayer.addr[0] = (unsigned long) buf;
        if (stride != -1)
            m_dcLayer.src_stride[0] = stride;
    }
    //for g2d
    {
        //printf("SDRV SDRVLayerEngine setHwLayerBuffer set g2dLayer.addr[0] \n");
        m_g2dLayer.addr[0] = (unsigned long) buf;
        if (stride != -1)
            m_g2dLayer.src_stride[0] = stride;
    }

    return DEFAULT_STATUS;
}

/*get buffer stride*/
int SDRVHardwareLayer::getBufferStride()
{
    if (isRootLayer())
        return getSdmBufferConfig().src_stride[0];   
    else
        return getG2dInputConfig().src_stride[0];
}

struct SDRVSpriteLayer : public Qul::PlatformInterface::LayerEngine::SpriteLayer, public SDRVHardwareLayer
{
    SDRVSpriteLayer(const Qul::PlatformInterface::LayerEngine::SpriteLayerProperties &p, SDRVSpriteLayer * spritelayer)
        : SDRVHardwareLayer(p, p.size, toHwPixelFormat(p.colorDepth),
                            SDRVLayerType::SDRV_SPRITE_LAYER, spritelayer)
    {
        // Allocate double buffers for hardware framebuffer layer
        int bufernum = doublebuf ? 2 : 1;
        for (int i = 0; i < bufernum; ++i)
            framebuffers[i] = (unsigned char *)qul_malloc(p.size.width() * p.size.height() * bytesPerPixelFromColorDepth(p.colorDepth));
    }

    void updateProperties(const Qul::PlatformInterface::LayerEngine::SpriteLayerProperties &p)
    {
        SDRVHardwareLayer::updateProperties(p, p.size);
    }

    ~SDRVSpriteLayer()
    {
        int bufernum = doublebuf ? 2 : 1;
        for (int i = 0; i < bufernum; ++i)
            qul_free(framebuffers[i]);
    }

    unsigned char *getNextDrawBuffer()
    {
        return doublebuf ? framebuffers[!frontBufferIndex] : framebuffers[0];
    }

    void swap()
    {
        //first set hw buffer
        setHwLayerBuffer(getNextDrawBuffer());
        //second swap buffer
        if (doublebuf)
            frontBufferIndex = !frontBufferIndex;
    }

    int addChildLayer(SDRVHardwareLayer *child)
    {
        //printf("SDRV addChildLayer %p\n", child);
        if (!child || child->isRootLayer()) {
            //printf("SDRV addChildLayer add error\n");
            return ERROR_STATUS;
        }

        mSpriteChildMap.insert(SpriteChildMap::value_type(child->getZorder(), child));
        return DEFAULT_STATUS;
    }

    int delChildLayer(SDRVHardwareLayer *child)
    {
        //printf("SDRV delChildLayer %p\n", child);
        if (!child || child->isRootLayer())  {
            //printf("SDRV delChildLayer add error\n");
            return ERROR_STATUS;
        }

        SpriteChildMap::iterator it;
        it = mSpriteChildMap.find(child->getZorder());
        if (it->second == child)
            mSpriteChildMap.erase(it);
        else
            printf("error: delChildLayer find not match\n");
        
        return DEFAULT_STATUS;
    }

    int getChildNum()
    {
        return mSpriteChildMap.size();
    }

    int bltChildLayer()
    {
        //printf("SDRV bltChildLayer child num = %d\n", getChildNum());
        if ( getChildNum() <= 0)  
            return DEFAULT_STATUS;

        struct g2dlite_input input;
        memset(&input, 0, sizeof(g2dlite_input));
        std::map<int, SDRVHardwareLayer *>::iterator it;
        it=mSpriteChildMap.begin();
        input.layer[input.layer_num] = (it->second)->getG2dInputConfig();
        input.layer[input.layer_num].zorder = 0;
        input.layer[input.layer_num].layer = 0;
        input.layer_num++;

        for (++it; it != mSpriteChildMap.end(); ++it) {
            if (input.layer_num < 2) {
                input.layer[input.layer_num] = (it->second)->getG2dInputConfig();
                input.layer[input.layer_num].zorder = 1;
                input.layer[input.layer_num].layer = 1;
                input.layer_num++;
            } else {
                //printf("SDRV bltChildLayer should not happen...");
            }

            //printf("SDRV bltChildLayer input layer_num = %d\n", input.layer_num);
            input.output.width = getSize().width();
            input.output.height = getSize().height();
            input.output.fmt = getHwFmt();
            input.output.addr[0] = (unsigned long)(getNextDrawBuffer());
            input.output.stride[0] = getBufferStride();
            input.output.rotation = 0;
            //printf("SDRV bltChildLayer hal_g2dlite_blend start\n");
            // //printf("SDRV g2d input.layer[0]:%d,%d,%d,%d\n", input.layer[0].layer, input.layer[0].fmt, input.layer[0].zorder, input.layer[0].alpha);
            // //printf("SDRV g2d input.layer[1]:%d,%d,%d,%d\n", input.layer[1].layer, input.layer[1].fmt, input.layer[1].zorder, input.layer[1].alpha);
            // //printf("SDRV g2d input.output:%d,%p,%d,%d\n",input.output.fmt, input.output.addr[0], input.layer_num, input.output.stride[0]);
            hal_g2dlite_blend(G2D, &input);
            //printf("SDRV bltChildLayer hal_g2dlite_blend end\n");

            //for second g2d blend
            memset(&input, 0, sizeof(g2dlite_input));
            input.layer[input.layer_num] = getG2dInputConfig();//sprite layer g2dconfig
            input.layer[input.layer_num].addr[0] = (unsigned long)(getNextDrawBuffer());
            input.layer[input.layer_num].zorder = 0;
            input.layer[input.layer_num].layer = 0;
            input.layer_num++;
        }
        return DEFAULT_STATUS;
    }

    bool doublebuf = true;
    int frontBufferIndex = 0;
    unsigned char *framebuffers[2];

    SpriteChildMap mSpriteChildMap;
};

void SDRVDrawingEngine::blendImage(Qul::PlatformInterface::DrawingDevice *drawingDevice, 
                    const Qul::PlatformInterface::Point &pos, 
                    const Qul::PlatformInterface::Texture &source, 
                    const Qul::PlatformInterface::Rect &sourceRect, 
                    int sourceOpacity, 
                    Qul::PlatformInterface::DrawingEngine::BlendMode blendMode)
{
    // printf("blendImage----------------dsadsdsds------------------------------------------\r\n");
    // if(sourceRect.width() * sourceRect.height() < PIXEL_GPU_LIMIT) {
        // drawingDevice->fallbackDrawingEngine()->blendImage(drawingDevice, pos, source, sourceRect, sourceOpacity, blendMode);
        // printf("fallback----------------fallback--------------------fallback-----------------fallback----blendImage~~~~~~~~~~~~~~~~-\r\n");
        // return;
    // }
    
    int x = sourceRect.x();
    int y = sourceRect.y();
    int w =  sourceRect.width();
    int h =  sourceRect.height();
    // fallbackDrawingEngine()->blendImage(drawingDevice, pos, source, sourceRect, sourceOpacity, blendMode);

    static struct g2dlite_input input;
    input.layer_num = 2;
    // printf("g2dlite_input----------------------------------------------------------\r\n");
    for (int i = 0; i < input.layer_num; i++) {
        struct g2dlite_input_cfg  *l = &input.layer[i];
        l->layer_en = 1;
        l->layer = i;
        l->fmt = COLOR_ARGB8888;
        l->zorder = i;
        l->ckey.en = 0;

        l->alpha = 255;

        if (i == 0) {
            l->blend = BLEND_PIXEL_NONE;
            l->addr[0] = (unsigned long) (drawingDevice->bits());
            l->src.x = pos.x();
            l->src.y = pos.y();
            l->src.w = w;
            l->src.h = h;
            l->src_stride[0] = drawingDevice->width()*4;

            l->dst.x = 0;// canvas :output x y
            l->dst.y = 0;
            l->dst.w = w;
            l->dst.h = h;
        } else {
            l->blend = BLEND_PIXEL_COVERAGE;
            l->addr[0] = (unsigned long) (source.data());
            l->src.x = x;
            l->src.y = y;
            l->src.w = w;
            l->src.h = h;
            l->src_stride[0] = source.width()*4;

            l->dst.x = 0;// canvas :output x y
            l->dst.y = 0;
            l->dst.w = w;
            l->dst.h = h;
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

static uint32_t argb8888_to_rgb2101010(Qul::PlatformInterface::Rgba32 color)
{
    return (((color.value >> 16 & 0x000000ff) << 22) | ((color.value >> 8 & 0x000000ff) << 14) | ((color.value & 0x000000ff) << 2));
}

void SDRVDrawingEngine::blendRect(Qul::PlatformInterface::DrawingDevice * drawingDevice , 
                                  const Qul::PlatformInterface::Rect & rect , 
                                  Qul::PlatformInterface::Rgba32 color , 
                                  Qul::PlatformInterface::DrawingEngine::BlendMode blendMode)
{
    // printf("blendRect----------------------333---------------\r\n");
    // printf("rect x = %d,rect y = %d,rect width = %d,rect height = %d----color.alpha() = %d----color.value = %u   --blendMode = %d----drawingDevice->width() = %d-drawingDevice->height() = %d-drawingDevice.format = %d-\r\n",rect.x(),rect.y(),rect.width(),rect.height(),color.alpha(),color.value,blendMode,drawingDevice->width(),drawingDevice->height(),drawingDevice->format());
    
    // arch_clean_cache_range((addr_t)drawingDevice->bits(), 1920*720*4);
    // if(rect.width() < 10){//rect.width() * rect.height() < PIXEL_GPU_LIMIT) {
        // drawingDevice->fallbackDrawingEngine()->blendRect(drawingDevice, rect, color, blendMode);
    //     printf("fallback----------------fallback--------------------fallback-----------------fallback---DrawingEngine!!!!!!!!!!!!!--\r\n");
        // return;
    // }
    int rgb_to_bit = bytesPerPixelFromHwPixelFormat(drawingDevice->format());
    if(!rgb_to_bit)
        return;

    struct g2dlite_output_cfg output;
    output.width = rect.width();
    output.height = rect.height();
    output.o_x = rect.x();
    output.o_y = rect.y();
    output.fmt = COLOR_RGB565;
    output.addr[0] = (unsigned long)(drawingDevice->bits() + rect.y() * drawingDevice->width() * rgb_to_bit + rect.x() * rgb_to_bit);
    output.stride[0] = drawingDevice->width() * rgb_to_bit;
    output.rotation = 0;

    hal_g2dlite_fill_rect(G2D, argb8888_to_rgb2101010(color), color.alpha(), 0, 0, 0, &output);

}

void SDRVDrawingEngine::synchronizeForCpuAccess(Qul::PlatformInterface::DrawingDevice * drawingDevice , 
                                                const Qul::PlatformInterface::Rect & rect)
{
    // arch_clean_cache_range((addr_t), pixel size;
}

struct SDRVItemLayer : public Qul::PlatformInterface::LayerEngine::ItemLayer, public SDRVHardwareLayer
{
    SDRVItemLayer(const Qul::PlatformInterface::LayerEngine::ItemLayerProperties &p, SDRVSpriteLayer * spritelayer)
        : SDRVHardwareLayer(p, p.size, toHwPixelFormat(p.colorDepth), SDRVLayerType::SDRV_ITEM_LAYER, spritelayer),
                            drawingDevice(toPixelFormat(p.colorDepth), p.size, nullptr,
                            p.size.width() * bytesPerPixelFromColorDepth(p.colorDepth),
                            &sdrvDrawingEngine)
    {
        // Allocate double buffers for hardware framebuffer layer
        int bufernum = doublebuf ? 2 : 1;
        framebufferSize = p.size.width() * p.size.height() * bytesPerPixelFromColorDepth(p.colorDepth);
        for (int i = 0; i < bufernum; ++i)
            framebuffers[i] = (unsigned char *)qul_malloc(framebufferSize);
    }

    void updateProperties(const Qul::PlatformInterface::LayerEngine::ItemLayerProperties &p)
    {
        SDRVHardwareLayer::updateProperties(p, p.size);
    }

    ~SDRVItemLayer()
    {
        int bufernum = doublebuf ? 2 : 1;
        for (int i = 0; i < bufernum; ++i)
            qul_free(framebuffers[i]);
    }

    unsigned char *getNextDrawBuffer()
    {
        return doublebuf ? framebuffers[!frontBufferIndex] : framebuffers[0];
    }
    int getFrameBufferSize()
    {
        return framebufferSize;
    }
    void swap()
    {
        //first set hw buffer
        setHwLayerBuffer(getNextDrawBuffer());
        //second swap buffer
        if (doublebuf)
            frontBufferIndex = !frontBufferIndex;
    }
    bool doublebuf = false;
    int frontBufferIndex = 0;
    int framebufferSize  = 0;
    unsigned char *framebuffers[2];
    Qul::PlatformInterface::DrawingDevice drawingDevice;
};

struct SDRVImageLayer : public Qul::PlatformInterface::LayerEngine::ImageLayer, public SDRVHardwareLayer
{
    SDRVImageLayer(const Qul::PlatformInterface::LayerEngine::ImageLayerProperties &p, SDRVSpriteLayer * spritelayer)
        : SDRVHardwareLayer(p,
                            p.texture.size(),
                            toHwPixelFormatFromPixelFormat(p.texture.format()),
                            SDRVLayerType::SDRV_IMAGE_LAYER, spritelayer)
    {
        // if use imageprovider, do not malloc buffer, just setHwLayerBuffer
        //printf("SDRV SDRVImageLayer start %p,%d,%d,%d\n", p.texture.data(), p.texture.size().width(), p.texture.size().height(), p.texture.bytesPerPixel());
        // framebuffers = (unsigned char *)qul_malloc(p.texture.size().width() * p.texture.size().height() * bytesPerPixelFromPixelFormat(p.texture.format()));
        // memcpy(framebuffers, p.texture.data(), p.texture.size().width() * p.texture.size().height() * bytesPerPixelFromPixelFormat(p.texture.format()));
        // arch_clean_cache_range((addr_t)framebuffers, p.texture.size().width()*p.texture.size().height()*bytesPerPixelFromPixelFormat(p.texture.format()));
        // setHwLayerBuffer(framebuffers, p.texture.size().width()*bytesPerPixelFromPixelFormat(p.texture.format()));
        setHwLayerBuffer(p.texture.data(), p.texture.size().width()*bytesPerPixelFromPixelFormat(p.texture.format()));
    }

    void updateProperties(const Qul::PlatformInterface::LayerEngine::ImageLayerProperties &p)
    {
        SDRVHardwareLayer::updateProperties(p, p.texture.size());
    }

    ~SDRVImageLayer()
    {
        qul_free(framebuffers);
    }
    unsigned char *framebuffers;
};

/*g2d init*/
int SDRVLayerEngine::init()
{
    //printf("SDRV SDRVLayerEngine init start\n");
    if (!hal_g2dlite_creat_handle(&G2D, RES_G2D_G2D2)) 
        printf("g2dlite creat handle failed\n");

    hal_g2dlite_init(G2D);
    printf("g2d->index 0x%x\n", ((struct g2dlite *)G2D)->index);
}

/*init display*/
int SDRVLayerEngine::initDisplay(const PlatformInterface::Screen *screen)
{
    //printf("SDRV SDRVLayerEngine initDisplay start %p\n", screen);
    //TODO: For multidisplay,screen name should config
    struct list_node *head = sdm_get_display_list();
    list_for_every_entry(head, m_sdm, sdm_display_t, node) {
        if (m_sdm->handle->display_id == QT_DISPLAY_ID)
            break;
    }
    printf("QT display_id %d\n", m_sdm->handle->display_id);
}

/*SpriteLayer compose*/
int SDRVLayerEngine::bltSpriteLayer(const PlatformInterface::Screen *screen)
{
    //printf("SDRV SDRVLayerEngine bltSpriteLayer start %p\n", screen);

    std::vector<SDRVHardwareLayer *> layers = findRootLayerWithType(screen, SDRVLayerType::SDRV_SPRITE_LAYER);
    std::vector<SDRVHardwareLayer *>::iterator iter = layers.begin();
    while (iter != layers.end()) {
        SDRVHardwareLayer * layer = *iter++;
        //printf("SDRV SDRVLayerEngine bltSpriteLayer layer: %p start\n", layer);
        static_cast<SDRVSpriteLayer *>(layer)->bltChildLayer();
        //printf("SDRV SDRVLayerEngine bltSpriteLayer layer: %p end\n", layer);
        static_cast<SDRVSpriteLayer *>(layer)->swap();
    }
    //printf("SDRV SDRVLayerEngine bltSpriteLayer end %p\n", screen);
    return DEFAULT_STATUS;
}

/*use dc compose layer to post screen*/
int SDRVLayerEngine::bltRootLayer(const PlatformInterface::Screen *screen)
{
    //printf("SDRV SDRVLayerEngine bltRootLayer start %p\n", screen);
    std::vector<SDRVHardwareLayer *> layers = findAllRootLayer(screen);

    if (layers.size() == 0) {
        printf("error: bltRootLayer screen %p root layer num is 0\n", screen);
        return ERROR_STATUS;
    }
    if (layers.size() == 1) {
        printf("warn: bltRootLayer screen %p root layer num is 1, suggest use 2 layer\n", screen);
        sdm_bufs[0] = layers[0]->getSdmBufferConfig();
        sdm_bufs[0].z_order = 0;//force set z_order to 0
        post_data.n_bufs    = 1;
    }
    else if (layers.size() == getDCHwLayerNum()) {
        sdm_bufs[0] = layers[0]->getSdmBufferConfig();
        sdm_bufs[1] = layers[1]->getSdmBufferConfig();
        post_data.n_bufs = 2;
    }
    else if (layers.size() > getDCHwLayerNum()) {
        //printf("warn: bltRootLayer screen %p root layer num > 2, suggest use 2 layer\n", screen);
        //printf("SDRV rootFrameBufferIndex %d\n", rootFrameBufferIndex);
        //TODO: should use g2d blend first
        if (rootFrameBuffer[rootFrameBufferIndex] == NULL) {
            rootFrameBuffer[rootFrameBufferIndex] = (unsigned char*) qul_malloc(
                screen->size().width() * screen->size().height() * 4);
        }

        // sort by zorder
        sort(layers.begin(),layers.end(),compare_z);

        struct g2dlite_input input;
        memset(&input, 0, sizeof(g2dlite_input));
        //printf("SDRV bltRootLayer  layers[0]=%p\n", layers[0]);
        input.layer[input.layer_num] = layers[0]->getG2dInputConfig();
        input.layer[input.layer_num].zorder = 0;
        input.layer[input.layer_num].layer  = 0;
        input.layer_num++;

        for (int i = 1; i <= layers.size()-2; i++) {
            //printf("SDRV bltRootLayer  layers[i]=%p\n", layers[i]);
            if (input.layer_num < 2) {
                input.layer[input.layer_num] = layers[i]->getG2dInputConfig();
                input.layer[input.layer_num].zorder = 1;
                input.layer[input.layer_num].layer  = 1;
                input.layer_num++;
            } else 
                printf("error: g2d bltRootLayer should not happen...");
            

            //printf("SDRV bltChildLayer input layer_num = %d\n", input.layer_num);
            input.output.width = screen->size().width();
            input.output.height = screen->size().height();
            input.output.fmt = COLOR_ABGR8888;
            input.output.addr[0]   = (unsigned long) (rootFrameBuffer[rootFrameBufferIndex]);
            input.output.stride[0] = screen->size().width() * 4;
            input.output.rotation  = 0;
            //printf("SDRV bltRootLayer hal_g2dlite_blend start\n");
            //printf("SDRV g2d input.layer[1] dst:%d,%d,%d,%d\n", input.layer[1].dst.x, input.layer[1].dst.y, input.layer[1].dst.w, input.layer[1].dst.h);

            //printf("SDRV g2d input.layer[0]:%d,%d,%d,%d\n", input.layer[0].layer, input.layer[0].fmt, input.layer[0].zorder, input.layer[0].alpha);
            //printf("SDRV g2d input.layer[1]:%d,%d,%d,%d\n", input.layer[1].layer, input.layer[1].fmt, input.layer[1].zorder, input.layer[1].alpha);
            //printf("SDRV g2d input.output:%d,%p,%d,%d\n",input.output.fmt, input.output.addr[0], input.layer_num, input.output.stride[0]);
            hal_g2dlite_blend(G2D, &input);
            //printf("SDRV bltRootLayer hal_g2dlite_blend end\n");

            //for second g2d blend
            memset(&input, 0, sizeof(g2dlite_input));
            input.layer[input.layer_num] = G2D_QT_LAYER_0;
            input.layer[input.layer_num].addr[0] = (unsigned long)(rootFrameBuffer[rootFrameBufferIndex]);
            input.layer[input.layer_num].zorder  = 0;
            input.layer[input.layer_num].layer   = 0;
            input.layer_num++;
        }

        // dc blend last layer with framebuffer
        sdm_bufs[0] = DISPLAY_QT_LAYER_0;
        sdm_bufs[0].addr[0] = (unsigned long)(rootFrameBuffer[rootFrameBufferIndex]);
        //printf("SDRV bltRootLayer   layers[layers.size()-1]=%p\n",  layers[layers.size()-1]);
        sdm_bufs[1] = layers[layers.size()-1]->getSdmBufferConfig();
        sdm_bufs[1].z_order = 1;
        post_data.n_bufs    = 2;

        rootFrameBufferIndex = !rootFrameBufferIndex;
    }

    //printf("SDRV src0 %d,%d,%d,%d \n", sdm_bufs[0].src.x, sdm_bufs[0].src.y, sdm_bufs[0].src.w, sdm_bufs[0].src.h);
    //printf("SDRV dst0 %d,%d,%d,%d \n", sdm_bufs[0].dst.x, sdm_bufs[0].dst.y, sdm_bufs[0].dst.w, sdm_bufs[0].dst.h);
    //printf("SDRV other0 %p,%d,%d,%d,%d,%d,0x%x,%d \n", sdm_bufs[0].addr[0], sdm_bufs[0].layer, sdm_bufs[0].layer_en, sdm_bufs[0].fmt, sdm_bufs[0].src_stride[0], sdm_bufs[0].z_order, sdm_bufs[0].alpha, sdm_bufs[0].alpha_en);
    //printf("SDRV src1 %d,%d,%d,%d \n", sdm_bufs[1].src.x, sdm_bufs[1].src.y, sdm_bufs[1].src.w, sdm_bufs[1].src.h);
    //printf("SDRV dst1 %d,%d,%d,%d \n", sdm_bufs[1].dst.x, sdm_bufs[1].dst.y, sdm_bufs[1].dst.w, sdm_bufs[1].dst.h);
    //printf("SDRV other1 %p,%d,%d,%d,%d,%d,0x%x,%d \n", sdm_bufs[1].addr[0], sdm_bufs[1].layer, sdm_bufs[1].layer_en, sdm_bufs[1].fmt, sdm_bufs[1].src_stride[0], sdm_bufs[1].z_order, sdm_bufs[1].alpha, sdm_bufs[1].alpha_en);

    post_data.bufs             = sdm_bufs;
    post_data.custom_data      = NULL;
    post_data.custom_data_size = 0;

    //post to screen
    sdm_post(m_sdm->handle, &post_data);
    //printf("SDRV SDRVLayerEngine bltSpriteLayer end %p\n", screen);
    return DEFAULT_STATUS;
}

/*add layer to Rootlayer*/
int SDRVLayerEngine::addRootLayer(const PlatformInterface::Screen *screen, SDRVHardwareLayer *layer)
{
    if (!layer || !screen) 
        return ERROR_STATUS;
    ScreenLayerVecMap::iterator it = mScreenRootLayerVecMap.find(screen);
    if (it == mScreenRootLayerVecMap.end()) {
        std::vector<SDRVHardwareLayer *> vec;
        vec.push_back(layer);
        mScreenRootLayerVecMap.insert(make_pair(screen, vec));
    } else 
        it->second.push_back(layer);
    
    return DEFAULT_STATUS;
}

/*del rootlayer*/
int SDRVLayerEngine::delRootLayer(SDRVHardwareLayer * layer)
{
    if (!layer)  
        return ERROR_STATUS;

    ScreenLayerVecMap::iterator it;
    for (it = mScreenRootLayerVecMap.begin(); it != mScreenRootLayerVecMap.end(); it++) {
        it->second.erase(remove(it->second.begin(),it->second.end(),layer),it->second.end());
    }

    return DEFAULT_STATUS;
}

/*search layer by type*/
std::vector<SDRVHardwareLayer *> SDRVLayerEngine::findRootLayerWithType(const PlatformInterface::Screen *screen, SDRVLayerType type)
{
    //printf("SDRV findRootLayerWithType start %d,%d\n", type, root);
    std::vector<SDRVHardwareLayer *> layers;
    if (screen) {
        ScreenLayerVecMap::iterator it = mScreenRootLayerVecMap.find(screen);
        if (it != mScreenRootLayerVecMap.end()) {
            std::vector<SDRVHardwareLayer *>::iterator iter = it->second.begin();
            while(iter != it->second.end()) {
                SDRVHardwareLayer * layer = *iter++;
                if (layer && (layer->getSdrvLayerType() == type || type == -1)) {
                    //printf("SDRV findRootLayerWithType find out: %p\n", layer);
                    layers.push_back(layer);
                }
            }
        }
    }

    return std::move(layers);
}

/*search layer by screen*/
std::vector<SDRVHardwareLayer *> SDRVLayerEngine::findAllRootLayer(const PlatformInterface::Screen *screen)
{
    //printf("SDRV findAllRootLayer start %d,%d\n", type, root);
    std::vector<SDRVHardwareLayer *> layers;
    if (screen) {
        ScreenLayerVecMap::iterator it = mScreenRootLayerVecMap.find(screen);
        if (it != mScreenRootLayerVecMap.end()) {
            std::vector<SDRVHardwareLayer *>::iterator iter = it->second.begin();
            while(iter != it->second.end()) {
                SDRVHardwareLayer * layer = *iter++;
                if (layer)  {
                    //printf("SDRV findAllRootLayer find out: %p\n", layer);
                    layers.push_back(layer);
                }
            }
        }
    }
    return std::move(layers);
}

/*return ScreenRootLayer number*/
int SDRVLayerEngine::getScreenRootLayerNum(const PlatformInterface::Screen *screen)
{
    std::vector<SDRVHardwareLayer *> layers;
    if (screen) {
        ScreenLayerVecMap::iterator it = mScreenRootLayerVecMap.find(screen);
        if (it != mScreenRootLayerVecMap.end()) 
            return it->second.size();
    }

    return DEFAULT_STATUS;
}

/*return screen number*/
int SDRVLayerEngine::getScreenNum()
{
    return mScreenRootLayerVecMap.size();
}

/*Frame flash begin*/
PlatformInterface::DrawingDevice *SDRVLayerEngine::beginFrame(const PlatformInterface::LayerEngine::ItemLayer *layer,
                                                              const PlatformInterface::Rect &,
                                                              int refreshInterval)
{
    //printf("SDRV SDRVLayerEngine beginFrame start %p, %d, %d\n", layer, refreshInterval, currentFrame);
    auto itemLayer = const_cast<SDRVItemLayer *>(static_cast<const SDRVItemLayer *>(layer));

    unsigned char *bits = itemLayer->getNextDrawBuffer();
    //printf("SDRV SDRVLayerEngine beginFrame nextdrawbuf %p\n", bits);

    // The drawing device also needs the framebuffer pointer for the CPU rendering fallbacks to work
    itemLayer->drawingDevice.setBits(bits);
    //printf("SDRV SDRVLayerEngine beginFrame end\n");
    return &itemLayer->drawingDevice;
}

/*Frame flash end*/
void SDRVLayerEngine::endFrame(const PlatformInterface::LayerEngine::ItemLayer *layer)
{

    //printf("SDRV SDRVLayerEngine endFrame start %p, %d\n", layer, currentFrame);
    auto itemLayer = const_cast<SDRVItemLayer *>(static_cast<const SDRVItemLayer *>(layer));

    //sw need clean cache
    unsigned char *bits = itemLayer->getNextDrawBuffer();
    arch_clean_cache_range((addr_t)bits, itemLayer->getFrameBufferSize());

    itemLayer->swap();
    //printf("SDRV SDRVLayerEngine endFrame end\n");
}

/*use g2d blend layer and post to screen*/
FrameStatistics SDRVLayerEngine::presentFrame(const PlatformInterface::Screen *screen,
                                              const PlatformInterface::Rect &rect)
{
    // static unsigned int lastFrame = 0xffffffff;
    //printf("SDRV SDRVLayerEngine presentFrame start %d, %d\n", currentFrame, lastFrame);

    // lastFrame = currentFrame;

    PlatformInterface::Rgba32 color = screen->backgroundColor();
    //TODO:
    // HW_SetScreenBackgroundColor(color.red(), color.blue(), color.green());
    bltSpriteLayer(screen);

    bltRootLayer(screen);
    // No frame skip compensation implemented for layers
    return FrameStatistics();
}

/*Allocates an item layer for rendering dynamic content.*/
PlatformInterface::LayerEngine::ItemLayer *SDRVLayerEngine::allocateItemLayer(const PlatformInterface::Screen *screen,
                                                                              const ItemLayerProperties &props,
                                                                              SpriteLayer *spriteLayer)
{
    //printf("SDRV allocateItemLayer\n");
    SDRVItemLayer *layer = new SDRVItemLayer(props, static_cast<SDRVSpriteLayer *>(spriteLayer));

    if (spriteLayer)
        static_cast<SDRVSpriteLayer *>(spriteLayer)->addChildLayer(static_cast<SDRVHardwareLayer *>(layer));
    else
        addRootLayer(screen, static_cast<SDRVHardwareLayer *>(layer));

    return layer;
}

/*Allocates an image layer for displaying static image content.*/
PlatformInterface::LayerEngine::ImageLayer *SDRVLayerEngine::allocateImageLayer(const PlatformInterface::Screen *screen,
                                                                                const ImageLayerProperties &props,
                                                                                SpriteLayer *spriteLayer)
{
    //printf("SDRV allocateImageLayer\n");
    SDRVImageLayer *layer = new SDRVImageLayer(props, static_cast<SDRVSpriteLayer *>(spriteLayer));

    if (spriteLayer)
        static_cast<SDRVSpriteLayer *>(spriteLayer)->addChildLayer(static_cast<SDRVHardwareLayer *>(layer));
    else
        addRootLayer(screen, static_cast<SDRVHardwareLayer *>(layer));
    
    return layer;
}

/*Allocates a sprite layer for displaying sprites.*/
PlatformInterface::LayerEngine::SpriteLayer *SDRVLayerEngine::allocateSpriteLayer( const PlatformInterface::Screen *screen,
                                                                                   const SpriteLayerProperties &props)
{
    //printf("SDRV allocateSpriteLayer\n");
    SDRVSpriteLayer * layer = new SDRVSpriteLayer(props, NULL);
    addRootLayer(screen, static_cast<SDRVHardwareLayer *>(layer));
    return layer;
}

/*Deallocates an item layer.*/
void SDRVLayerEngine::deallocateItemLayer(PlatformInterface::LayerEngine::ItemLayer *layer)
{
    //printf("SDRV deallocateItemLayer %p\n", layer);
    SDRVHardwareLayer *  hwlayer = (static_cast<SDRVHardwareLayer *>(static_cast<SDRVItemLayer *>(layer)));
    if (hwlayer) {
        if (hwlayer->isRootLayer())
            delRootLayer(hwlayer);
        else {
            if(hwlayer->getParentLayer())
                static_cast<SDRVSpriteLayer *>(hwlayer->getParentLayer())->delChildLayer(hwlayer);
        }
    }

    delete static_cast<SDRVItemLayer *>(layer);
}

/*Deallocates an image layer.*/
void SDRVLayerEngine::deallocateImageLayer(PlatformInterface::LayerEngine::ImageLayer *layer)
{
    //printf("SDRV deallocateImageLayer %p\n", layer);
    SDRVHardwareLayer *  hwlayer = (static_cast<SDRVHardwareLayer *>(static_cast<SDRVImageLayer *>(layer)));
    if (hwlayer) {
        if (hwlayer->isRootLayer())
            delRootLayer(hwlayer);
        else {
            if(hwlayer->getParentLayer())
                static_cast<SDRVSpriteLayer *>(hwlayer->getParentLayer())->delChildLayer(hwlayer);
        }
    }

    delete static_cast<SDRVImageLayer *>(layer);
}

/*Deallocates a sprite layer.*/
void SDRVLayerEngine::deallocateSpriteLayer(PlatformInterface::LayerEngine::SpriteLayer *layer)
{
    //printf("SDRV deallocateSpriteLayer %p\n", layer);
    delRootLayer(static_cast<SDRVHardwareLayer *>(static_cast<SDRVSpriteLayer *>(layer)));
    delete static_cast<SDRVSpriteLayer *>(layer);
}

/*Updates the properties of an item layer.*/
void SDRVLayerEngine::updateItemLayer(PlatformInterface::LayerEngine::ItemLayer *layer,
                                      const ItemLayerProperties &props)
{
    //printf("SDRV updateItemLayer %p\n", layer);
    static_cast<SDRVItemLayer *>(layer)->updateProperties(props);
}

/*Updates the properties of an image layer.*/
void SDRVLayerEngine::updateImageLayer(PlatformInterface::LayerEngine::ImageLayer *layer,
                                       const ImageLayerProperties &props)
{
    //printf("SDRV updateImageLayer %p\n", layer);
    static_cast<SDRVImageLayer *>(layer)->updateProperties(props);
}

/*Updates the properties of a sprite layer.*/
void SDRVLayerEngine::updateSpriteLayer(PlatformInterface::LayerEngine::SpriteLayer *layer,
                                        const SpriteLayerProperties &props)
{
    //printf("SDRV updateSpriteLayer %p\n", layer);
    static_cast<SDRVSpriteLayer *>(layer)->updateProperties(props);
}
// ![exampleLayerEngineUpdateFunctions]

} // namespace Platform
} // namespace Qul
