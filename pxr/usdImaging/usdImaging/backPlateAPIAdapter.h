//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_BACKPLATE_API_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_BACKPLATE_API_ADAPTER_H

#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingBackPlateAPIAdapter : public UsdImagingAPISchemaAdapter
{
public:
    using BaseAdapter = UsdImagingAPISchemaAdapter;

    USDIMAGING_API
    UsdImagingBackPlateAPIAdapter();

    USDIMAGING_API
    ~UsdImagingBackPlateAPIAdapter() override;

    USDIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
