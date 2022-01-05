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

#include <sdrvdrawengine.h>

#include <cstdio>

namespace Qul {
namespace Platform {

#define PIXEL_GPU_LIMIT 1000

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
    printf("blendRect----------------------333---------------\r\n");
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
    //  if(rect.width() ==  72)
        hal_g2dlite_fill_rect(G2D, argb8888_to_rgb2101010(color), color.alpha(), 0, 0, 0, &output);
    //  else
        //  hal_g2dlite_fill_rect(G2D, 0xFFFFFFFF, 0xFF, 0, 0, 0, &output);
}

void SDRVDrawingEngine::synchronizeForCpuAccess(Qul::PlatformInterface::DrawingDevice * drawingDevice , 
                                                const Qul::PlatformInterface::Rect & rect)
{
    // arch_clean_cache_range((addr_t), pixel size;
}


} // namespace Platform
} // namespace Qul
