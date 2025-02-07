//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIPRESENT2_TEST_TESTWINDOW_H
#define PXR_IMAGING_HGIPRESENT2_TEST_TESTWINDOW_H

#include "pxr/pxr.h"

#include "pxr/base/gf/vec2i.h"

#include "pxr/imaging/hgiPresent2/testenv/testWindowHandle.h"
#include "pxr/imaging/hio/image.h"

#include <memory>
#include <string_view>

PXR_NAMESPACE_OPEN_SCOPE

class HgiPresent2TestWindow
{
public:
    virtual ~HgiPresent2TestWindow() = default;

    virtual HgiPresent2TestWindowHandle GetHandle() const = 0;

    virtual GfVec2i GetSize() const = 0;

    virtual void SetSize(const GfVec2i &size) = 0;

    virtual bool Update() = 0;

    static std::unique_ptr<HgiPresent2TestWindow> Create(
        std::string_view apiName, const GfVec2i &size);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
