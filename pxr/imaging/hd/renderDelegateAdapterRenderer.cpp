//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderDelegateAdapterRenderer.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/rendererCreateArgsSchema.h"
#include "pxr/imaging/hd/renderIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (renderDriver)
);

std::vector<HdDriver>
_ComputeDrivers(HdContainerDataSourceHandle const &rendererCreateArgs)
{
    const HdRendererCreateArgsSchema argsSchema(rendererCreateArgs);
    const HdSampledDataSourceContainerSchema driverSchema =
        argsSchema.GetDrivers();

    std::vector<HdDriver> drivers;
    for (const TfToken &name : driverSchema.GetNames()) {
        HdSampledDataSourceHandle const ds = driverSchema.Get(name);
        if (!ds) {
            continue;
        }
        drivers.push_back({ name, ds->GetValue(0.0f)});
        // For concreteness, we now use Hgi rather than renderDriver as key.
        if (name == HdRendererCreateArgsSchemaTokens->hgi) {
            drivers.push_back({ _tokens->renderDriver, ds->GetValue(0.0f)});
        }
    }
    
    return drivers;
}

/// HdDriverVector is a std::vector<HdDriver*> (and the argument to
/// HdRenderDelegate::SetDrivers) - even though HdDriver is not intended
/// as an (abstract) base class but a struct of a TfToken and VtValue
/// and it is the VtValue that can hold a pointer to an abstract base
/// class such as Hgi.
///
/// Creating a std::vector<HdDriver*> here.
/// HdRenderDelegateAdapterRenderer::_drivers owns the HdDriver struct's.
///
static
HdDriverVector
_ToPointers(const std::vector<HdDriver> &drivers)
{
    HdDriverVector result;
    result.reserve(drivers.size());
    for (const HdDriver &driver : drivers) {
        result.push_back(const_cast<HdDriver*>(&driver));
    }
    return result;
}

HdRenderDelegateAdapterRenderer::HdRenderDelegateAdapterRenderer(
    HdPluginRenderDelegateUniqueHandle renderDelegate,
    HdSceneIndexBaseRefPtr const &terminalSceneIndex,
    HdContainerDataSourceHandle const &rendererCreateArgs)
 : _drivers(_ComputeDrivers(rendererCreateArgs))
 , _renderDelegate(std::move(renderDelegate))
 , _renderIndex(
     HdRenderIndex::New(
         _renderDelegate.Get(),
         _ToPointers(_drivers),
         terminalSceneIndex))
 , _engine(std::make_unique<HdEngine>())
{
}

HdRenderDelegateAdapterRenderer::~HdRenderDelegateAdapterRenderer() = default;

PXR_NAMESPACE_CLOSE_SCOPE
