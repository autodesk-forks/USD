// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stage.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char** argv) {
    ArSetPreferredResolver("FetchResolver");

    std::cout << "WasmFetchResolver Example" << std::endl;

    const std::string invalidUrl = "stages/teapots/no-stage.usda";
    std::cout << "Attempt to load invalid stage:" << std::endl;
    UsdStageRefPtr stage = UsdStage::Open(invalidUrl);
    if (!stage) {
        std::cout << "Failed to load: " << invalidUrl << std::endl;
    }

    const std::string validUrl = "stages/teapots/stage.usda";
    std::cout << "Loading valid stage:" << std::endl;
    stage = UsdStage::Open(validUrl);

    if (stage) {
        std::cout << "Successfully loaded: " << validUrl << std::endl;
    }

    return 0;
}
