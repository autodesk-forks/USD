//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent2/noOp.h"

#include "pxr/imaging/hgi/hgi.h"


PXR_NAMESPACE_OPEN_SCOPE


HgiPresent2NoOp::HgiPresent2NoOp()
    : HgiPresent2Impl(nullptr)
{}

bool
HgiPresent2NoOp::IsColorFormatSupported(HgiFormat) const
{
    return true;
}


bool
HgiPresent2NoOp::IsDepthFormatSupported(HgiFormat) const
{
    return true;
}

std::optional<bool>
HgiPresent2NoOp::IsValid() const
{
    return true;
}

void
HgiPresent2NoOp::UpdateParams(const HgiPresent2Params&)
{
}

void
HgiPresent2NoOp::Present(
        HgiTextureHandle const &,
        HgiTextureHandle const &)
{
}


PXR_NAMESPACE_CLOSE_SCOPE
