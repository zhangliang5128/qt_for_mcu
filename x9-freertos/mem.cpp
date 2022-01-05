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
#ifdef __cplusplus
extern "C" {
    #include "heap.h"
}
#endif

#include "string.h"

#include <platform/mem.h>

namespace Qul {
namespace Platform {

// ![printMemoryStats]
void printHeapStats(void)
{
    //malloc_stats(); // gcc function prining stats to standard error
}

void printStackStats(void)
{
    // Replace this with actual measuring for your platform
    //uint32_t maxUsedStackSize = 0;
    //fprintf(stderr, "max used stack size: %u\r\n", maxUsedStackSize);
}
// ![printMemoryStats]

// ![memAlloc]
void *qul_malloc(std::size_t size)
{
        return malloc(0x1000, size);
}

void qul_free(void *ptr)
{
    efree(ptr);
}

void *qul_realloc(void *ptr, size_t s)
{
    if (!ptr)
    {
        return qul_malloc(s);
    }

    if (s == 0)
    {
        qul_free(ptr);
        return qul_malloc(0);
    }

    void* new_ptr = qul_malloc(s);
    memcpy(new_ptr, ptr, s);
    qul_free(ptr);
    return new_ptr;
}
// ![memAlloc]

} // namespace Platform
} // namespace Qul
