//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/sdf/data.h"
#include "pxr/usd/sdf/fileFormat.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _Tokens,

    ((Extension, "phony"))
    ((Id, "phony"))
    ((Target, "usd"))
);

class UsdProfilesTestPhonyFileFormat
    : public SdfFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

    bool CanRead(const std::string& file) const override
    { return true; }

    bool Read(
        SdfLayer* layer,
        const std::string& resolvedPath,
        bool metadataOnly) const override
    {
        SdfAbstractDataRefPtr data = InitData(GetDefaultFileFormatArguments());
        _SetLayerData(layer, data);
        return true;
    }

private:
    UsdProfilesTestPhonyFileFormat()
        : SdfFileFormat(_Tokens->Id, TfToken(), _Tokens->Target,
                        _Tokens->Extension)
    {
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdProfilesTestPhonyFileFormat, SdfFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE
