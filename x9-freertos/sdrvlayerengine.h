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
#include <platforminterface/drawingengine.h>
#include <platforminterface/layerengine.h>
#include <config.h>
#include <lk_wrapper.h>
#include <g2dlite_api.h>
namespace sdm {
    #include "sdm_display.h"
}

#include "disp_data_type.h"
#include <vector>
#include <map>

using namespace sdm;
//COLOR_ABGR8888
//change to COLOR_ABGR8888, for pngloader decode fmt is COLOR_ABGR8888
#define DISPLAY_QT_LAYER_0 { \
    0,/*layer*/\
    0,/*layer_dirty*/\
    1,/*layer_en*/\
    COLOR_ABGR8888,/*fmt*/\
    {0,0,1920,720},/*x,y,w,h src */ \
    {0x0,0x0,0x0,0x0},/*y,u,v,a*/ \
    {7680,0,0,0},/*stride*/ \
    {0,0,1920,720},/*start*/ \
    {0,0,1920,720},/*dst*/ \
    0,/*ckey_en*/\
    0,/*ckey*/\
    1,/*alpha_en*/\
    0xff,/*alpha*/\
    0,/*z-order*/ \
    0/*security*/\
}

#define DISPLAY_QT_LAYER_1 { \
    1,/*layer*/\
    0,/*layer_dirty*/\
    1,/*layer_en*/\
    COLOR_ABGR8888,/*fmt*/\
    {0,0,1920,720},/*x,y,w,h src */ \
    {0x0,0x0,0x0,0x0},/*y,u,v,a*/ \
    {7680,0,0,0},/*stride*/ \
    {0,0,1920,720},/*start*/ \
    {0,0,1920,720},/*dst*/ \
    0,/*ckey_en*/\
    0,/*ckey*/\
    1,/*alpha_en*/\
    0x0,/*alpha*/\
    1,/*z-order*/ \
    0/*security*/\
}

#define G2D_QT_LAYER_0 { \
    0,/*layer*/\
    1,/*layer_en*/\
    COLOR_ABGR8888,/*fmt*/\
    0,/*zorder*/ \
    {0,0,1920,720},/*src*/\
    {0x0,0x0,0x0,0x0},/*addr:y,u,v,a*/ \
    {7680,0,0,0},/*stride*/ \
    {0,0,1920,720},/*dst*/ \
    BLEND_PIXEL_COVERAGE,/*blend*/\
    0xff,/*alpha*/\
    {0,0},/*ckey*/\
    {0,0,0,0},/*rle*/ \
    {0,0x0},/*clut*/ \
    PD_NONE /*pd_type*/ \
}

#define G2D_QT_LAYER_1 { \
    1,/*layer*/\
    1,/*layer_en*/\
    COLOR_ABGR8888,/*fmt*/\
    1,/*zorder*/ \
    {0,0,1920,720}, /*src*/\
    {0x0,0x0,0x0,0x0},/*addr:y,u,v,a*/ \
    {7680,0,0,0},/*stride*/ \
    {0,0,1920,720},/*dst*/ \
    BLEND_PIXEL_COVERAGE,/*blend*/\
    0x0,/*alpha*/\
    {0,0},/*ckey*/\
    {0,0,0,0},/*rle*/ \
    {0,0x0}, /*clut*/ \
    PD_NONE /*pd_type*/ \
}

#define USE_HW_ACC     0
#define RES_G2D_G2D2   0x4362200A
#define DCHWLAYERNUM   2
#define ERROR_STATUS  -1
#define DEFAULT_STATUS 0
#define FOUR_BIT       4
#define THREE_BIT      3
#define TWO_BIT        2 

static int QT_DISPLAY_ID = SCREEN_1;
static void *G2D = NULL;

struct g2dlite {
    int index;
    addr_t reg_addr;
    uint32_t irq_num;
};

static sdm_display_t * m_sdm;
static struct sdm_buffer sdm_bufs[2] = {DISPLAY_QT_LAYER_0, DISPLAY_QT_LAYER_1};
static struct sdm_post_config post_data;

namespace Qul {
namespace Platform {

enum SDRVLayerType{
    SDRV_ITEM_LAYER = 0,
    SDRV_IMAGE_LAYER,
    SDRV_SPRITE_LAYER
};

class SDRVDrawingEngine : public PlatformInterface::DrawingEngine
{
    void blendImage(Qul::PlatformInterface::DrawingDevice *drawingDevice, 
                    const Qul::PlatformInterface::Point &pos, 
                    const Qul::PlatformInterface::Texture &source, 
                    const Qul::PlatformInterface::Rect &sourceRect, 
                    int sourceOpacity, 
                    Qul::PlatformInterface::DrawingEngine::BlendMode blendMode = BlendMode_SourceOver);

    void blendRect (Qul::PlatformInterface::DrawingDevice * drawingDevice , 
                    const Qul::PlatformInterface::Rect & rect , 
                    Qul::PlatformInterface::Rgba32 color , 
                    Qul::PlatformInterface::DrawingEngine::BlendMode blendMode = BlendMode_SourceOver);

    void synchronizeForCpuAccess(Qul::PlatformInterface::DrawingDevice * drawingDevice , 
                                 const Qul::PlatformInterface::Rect & rect);
};

struct SDRVHardwareLayer
{
    SDRVHardwareLayer(const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                      const Qul::PlatformInterface::Size &s,
                      int hwPixelFormat,
                      SDRVLayerType t,
                      SDRVHardwareLayer* pl);

    void updateProperties(const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                          const Qul::PlatformInterface::Size &s);

    void setG2dLayerProperties(const int src_x,
                               const int src_y,
                               const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                               const Qul::PlatformInterface::Size &s);

    void setDCLayerProperties(const int start_x,
                              const int start_y,
                              const int src_x,
                              const int src_y,
                              const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                              const Qul::PlatformInterface::Size &s);

    void saveLayerPropertise(const Qul::PlatformInterface::LayerEngine::LayerPropertiesBase &p,
                             const Qul::PlatformInterface::Size &s);

    ~SDRVHardwareLayer();

    int setHwLayerBuffer(const unsigned char * buf, int stride=-1);

    bool isRootLayer() { return m_isRootLayer;}
    int getZorder() { return m_zorder;}
    int getHwFmt() { return m_hwPixelFormat;}
    int getBufferStride();
    Qul::PlatformInterface::LayerEngine::LayerPropertiesBase getProperties() { return m_props;}
    Qul::PlatformInterface::Size getSize() { return m_size;}
    sdm_buffer getSdmBufferConfig(){return m_dcLayer;}
    g2dlite_input_cfg getG2dInputConfig(){return m_g2dLayer;}
    SDRVLayerType getSdrvLayerType(){return m_type;}
    SDRVHardwareLayer* getParentLayer(){return m_parentlayer;}

    /*layer Properties*/
    Qul::PlatformInterface::LayerEngine::LayerPropertiesBase m_props;
    /*layer size*/
    Qul::PlatformInterface::Size m_size;

    /*SDRVLayerType*/
    SDRVLayerType m_type;
    /*usr dc compose layer*/
    sdm_buffer m_dcLayer;
    /*usr g2d compose layer*/
    g2dlite_input_cfg m_g2dLayer;
    /*parent layer*/
    SDRVHardwareLayer* m_parentlayer;
    /*current layer status ,rootlayer or normal layer*/
    bool m_isRootLayer;
    /*layer Properties zorder*/
    int m_zorder;
    /*hardware pixel format*/
    int m_hwPixelFormat;
};

typedef std::map<const PlatformInterface::Screen *, std::vector<SDRVHardwareLayer *> > ScreenLayerVecMap;
typedef std::map<int, SDRVHardwareLayer *> SpriteChildMap;
static SDRVDrawingEngine sdrvDrawingEngine;

class SDRVLayerEngine : public PlatformInterface::LayerEngine
{
public:
    int init();
    int initDisplay(const PlatformInterface::Screen *screen);
    static int getDCHwLayerNum(){ return DCHWLAYERNUM;}
    static int bltSpriteLayer(const PlatformInterface::Screen *screen);
    static int bltRootLayer(const PlatformInterface::Screen *screen);

    int addRootLayer(const PlatformInterface::Screen *screen, SDRVHardwareLayer * layer);
    int delRootLayer(SDRVHardwareLayer * layer);
    static std::vector<SDRVHardwareLayer *> findRootLayerWithType(const PlatformInterface::Screen *screen, SDRVLayerType type);
    static std::vector<SDRVHardwareLayer *> findAllRootLayer(const PlatformInterface::Screen *screen);
    int getScreenRootLayerNum(const PlatformInterface::Screen *screen);
    int getScreenNum();
    static bool compare_z(SDRVHardwareLayer* l1, SDRVHardwareLayer* l2){
        return l1->getZorder() < l2->getZorder();
    }
    ItemLayer *allocateItemLayer(const PlatformInterface::Screen *,
                                 const ItemLayerProperties &props,
                                 SpriteLayer *spriteLayer = nullptr) override;
    ImageLayer *allocateImageLayer(const PlatformInterface::Screen *,
                                   const ImageLayerProperties &props,
                                   SpriteLayer *spriteLayer = nullptr) override;
    SpriteLayer *allocateSpriteLayer(const PlatformInterface::Screen *, const SpriteLayerProperties &props) override;

    void deallocateItemLayer(ItemLayer *layer) override;
    void deallocateImageLayer(ImageLayer *layer) override;
    void deallocateSpriteLayer(SpriteLayer *layer) override;

    void updateItemLayer(ItemLayer *layer, const ItemLayerProperties &props) override;
    void updateImageLayer(ImageLayer *layer, const ImageLayerProperties &props) override;
    void updateSpriteLayer(SpriteLayer *layer, const SpriteLayerProperties &props) override;

    static PlatformInterface::DrawingDevice *beginFrame(const PlatformInterface::LayerEngine::ItemLayer *,
                                                        const PlatformInterface::Rect &,
                                                        int refreshInterval);
    static void endFrame(const PlatformInterface::LayerEngine::ItemLayer *);
    static FrameStatistics presentFrame(const PlatformInterface::Screen *screen, const PlatformInterface::Rect &rect);
    static ScreenLayerVecMap mScreenRootLayerVecMap;
    static unsigned char* rootFrameBuffer[2];
    static int rootFrameBufferIndex;
};

} // namespace Platform
} // namespace Qul
