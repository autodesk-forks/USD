//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https:/</openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationContext.h"
#include "pxr/usd/usd/validationError.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdValidationContext()
{
    TfPyRegisterStlSequencesFromPython<UsdValidationError>();
    class_<UsdValidationContext>("ValidationContext", no_init)
        .def(init<const TfTokenVector &, bool>(
            (arg("keywords"), arg("includeAllAncestors") = true)))
        .def(init<const PlugPluginPtrVector &, bool>(
            (arg("plugins"), arg("includeAllAncestors") = true)))
        .def(init<const UsdValidatorMetadataVector &, bool>(
            (arg("metadata"), arg("includeAllAncestors") = true)))
        .def(init<const std::vector<TfType> &>(arg("schemaTypes")))
        .def(init<const std::vector<const UsdValidator*> &>(arg("validators")))
        .def(init<const std::vector<const UsdValidatorSuite*> &>(arg("suites")))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const SdfLayerHandle &layer) 
                -> UsdValidationErrorVector {
                return ctx.Validate(layer);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("layer")))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage) 
                -> UsdValidationErrorVector {
                return ctx.Validate(stage);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("stage")))
        .def("Validate",
             +[](const UsdValidationContext &ctx, const UsdStagePtr &stage, 
                const Usd_PrimFlagsPredicate &predicate) 
                -> UsdValidationErrorVector {
                return ctx.Validate(stage, predicate);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("stage"), arg("predicate")))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, 
                 const std::vector<UsdPrim> &prims) 
                -> UsdValidationErrorVector {
                return ctx.Validate(prims);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("prims")))
        .def("Validate", 
             +[](const UsdValidationContext &ctx, const UsdPrimRange &prims) 
                -> UsdValidationErrorVector {
                return ctx.Validate(prims);
             },
             return_value_policy<TfPySequenceToList>(),
             (arg("prims")));
}
