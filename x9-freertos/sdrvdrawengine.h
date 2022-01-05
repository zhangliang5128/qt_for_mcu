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

using namespace sdm;

namespace Qul {
namespace Platform {

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


} // namespace Platform
} // namespace Qul
