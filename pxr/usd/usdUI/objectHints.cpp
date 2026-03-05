//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/usdUI/objectHints.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USDUI_WRITE_LEGACY_UI_HINTS, true,
    "Also write deprecated core metadata fields displayName, displayGroup, "
    "and hidden when writing them to their new locations in the uiHints "
    "dictionary.");

TF_DEFINE_PUBLIC_TOKENS(UsdUIHintKeys, USDUI_HINT_KEYS);

UsdUIObjectHints::UsdUIObjectHints() = default;

UsdUIObjectHints::UsdUIObjectHints(const UsdObject& obj)
    : _obj(obj)
{
}

std::string
UsdUIObjectHints::GetDisplayName() const
{
    if (!_obj) {
        return {};
    }

    std::string name;
    if (_obj.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->DisplayName,
            &name)) {
        return name;
    }

    // XXX: Fall back to legacy field
    return _obj.GetDisplayName();
}

bool
UsdUIObjectHints::SetDisplayName(const std::string& name)
{
    if (!_obj) {
        TF_CODING_ERROR("Invalid object");
        return false;
    }

    if (!_obj.SetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->DisplayName,
            name)) {
        return false;
    }

    if (TfGetEnvSetting(USDUI_WRITE_LEGACY_UI_HINTS)) {
        // Use generic API to avoid deprecation warning in
        // UsdObject::SetDisplayName()
        _obj.SetMetadata(SdfFieldKeys->DisplayName, name);
    }

    return true;
}

bool
UsdUIObjectHints::GetHidden() const
{
    if (!_obj) {
        return false;
    }

    bool hidden = false;
    if (_obj.GetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->Hidden,
            &hidden)) {
        return hidden;
    }

    // XXX: Fall back to legacy field
    return _obj.IsHidden();
}

bool
UsdUIObjectHints::SetHidden(bool hidden)
{
    if (!_obj) {
        TF_CODING_ERROR("Invalid object");
        return false;
    }

    if (!_obj.SetMetadataByDictKey(
            UsdUIHintKeys->UIHints,
            UsdUIHintKeys->Hidden,
            hidden)) {
        return false;
    }

    if (TfGetEnvSetting(USDUI_WRITE_LEGACY_UI_HINTS)) {
        // Use generic API to avoid deprecation warning in
        // UsdObject::SetHidden()
        _obj.SetMetadata(SdfFieldKeys->Hidden, hidden);
    }

    return true;
}

/* static */
TfToken
UsdUIObjectHints::_MakeKeyPath(
    const TfToken& key1,
    const TfToken& key2)
{
    return TfToken(
        TfStringPrintf("%s%c%s",
                       key1.GetText(),
                       UsdObject::GetNamespaceDelimiter(),
                       key2.GetText()));
}

PXR_NAMESPACE_CLOSE_SCOPE
