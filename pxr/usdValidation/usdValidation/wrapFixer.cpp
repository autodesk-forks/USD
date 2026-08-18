//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdValidation/usdValidation/fixer.h"
#include "pxr/usdValidation/usdValidation/error.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdValidationFixer()
{
    class_<UsdValidationFixer>("ValidationFixer", no_init)
        .add_property("name",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                      return fixer.GetName(); 
                },
                return_value_policy<return_by_value>()))
        .add_property("description",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                    return fixer.GetDescription(); 
                },
                return_value_policy<return_by_value>()))
        .add_property("errorName",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                    return fixer.GetErrorName(); 
                },
                return_value_policy<return_by_value>()))
        .add_property("keywords",
            make_function(
                +[](const UsdValidationFixer &fixer) { 
                    return fixer.GetKeywords(); 
                },
                return_value_policy<TfPySequenceToList>()))
        .def("IsAssociatedWithErrorName", 
             &UsdValidationFixer::IsAssociatedWithErrorName, (arg("errorName")))
        .def("HasKeyword", 
             &UsdValidationFixer::HasKeyword, (arg("keyword")))
        .def("CanApplyFix", 
             &UsdValidationFixer::CanApplyFix,
             (arg("error"), arg("editTarget"), 
              arg("timeCode") = UsdTimeCode::Default()))
        .def("ApplyFix", 
             &UsdValidationFixer::ApplyFix,
             (arg("error"), arg("editTarget"), 
              arg("timeCode") = UsdTimeCode::Default()));

    TfPyRegisterStlSequencesFromPython<const UsdValidationFixer*>();
}
