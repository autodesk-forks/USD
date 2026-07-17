//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdProfiles/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdProfilesTokensType::UsdProfilesTokensType() :
    enhancement("enhancement", TfToken::Immortal),
    hard("hard", TfToken::Immortal),
    soft("soft", TfToken::Immortal),
    ClaimsAPI("ClaimsAPI", TfToken::Immortal),
    allTokens({
        enhancement,
        hard,
        soft,
        ClaimsAPI
    })
{
}

TfStaticData<UsdProfilesTokensType> UsdProfilesTokens;

PXR_NAMESPACE_CLOSE_SCOPE
