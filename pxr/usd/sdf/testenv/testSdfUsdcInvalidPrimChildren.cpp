//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/usd/sdf/childrenView.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include <string>
#include <cstring>

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestInvalidPrimChildren()
{
    // This test relies on verification checks that are only enabled when
    // PXR_PREFER_SAFETY_OVER_SPEED is enabled.
#ifdef PXR_PREFER_SAFETY_OVER_SPEED
    // open the layer and check for invalid child prim names and specs
    TfErrorMark m;

    auto layer = SdfLayer::FindOrOpen("root.usdc");
    TF_AXIOM(!layer);
    
    TF_AXIOM(!m.IsClean());
    TF_AXIOM(std::any_of(m.begin(), m.end(),
                    [](const TfError& e) {
                        return TfStringEndsWith(e.GetCommentary(),
                                "Invalid child identifier '' found "
                                "on parent '/_materials'");
                    }));
    TF_AXIOM(std::any_of(m.begin(), m.end(),
                    [](const TfError& e) {
                        return TfStringEndsWith(e.GetCommentary(),
                                "Invalid children found in primChildren field "
                                "for path '/_materials'");
                    }));
#endif
}

static void
TestDuplicatePrimChildren()
{
    // This test relies on verification checks that are only enabled when
    // PXR_PREFER_SAFETY_OVER_SPEED is enabled.
#ifdef PXR_PREFER_SAFETY_OVER_SPEED
    // open the layer and check for invalid child prim names and specs
    TfErrorMark m;

    auto layer = SdfLayer::FindOrOpen("duplicate_prim_children.usdc");
    TF_AXIOM(!layer);

    TF_AXIOM(!m.IsClean());
    TF_AXIOM(std::any_of(m.begin(), m.end(),
                    [](const TfError& e) {
                        return TfStringEndsWith(e.GetCommentary(),
                                "Duplicate child identifier 'Child1' found "
                                "on parent '/Root'");
                    }));
    TF_AXIOM(std::any_of(m.begin(), m.end(),
                    [](const TfError& e) {
                        return TfStringEndsWith(e.GetCommentary(),
                                "Invalid children found in primChildren field "
                                "for path '/Root'");
                    }));
#endif    
}

int main(int argc, char** argv)
{
    TestInvalidPrimChildren();
    TestDuplicatePrimChildren();

    return 0;
}
