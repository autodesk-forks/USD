//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_MTLX_COMBINEDVERSION_H
#define PXR_IMAGING_HD_MTLX_COMBINEDVERSION_H

#include <MaterialXCore/Generated.h>

#define MTLX_COMBINED_VERSION                                                  \
    ((MATERIALX_MAJOR_VERSION * 100 * 100) + (MATERIALX_MINOR_VERSION * 100) \
     + MATERIALX_BUILD_VERSION)
#endif
