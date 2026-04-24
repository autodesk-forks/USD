//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/namespaceEditor.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/enum.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/scope.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static
UsdNamespaceEditor::CanApplyResult
_CanApplyEdits(const UsdNamespaceEditor &editor)
{
    return editor.CanApplyEdits();
}
static
std::string 
_FormatWhyNot(const UsdNamespaceEditor::CanApplyResult &result) {
    return TfStringJoin(result.errors, "; ");
}

void wrapUsdNamespaceEditor()
{
    using This = UsdNamespaceEditor;

    scope s = class_<This>("NamespaceEditor", no_init)
        .def(init<const UsdStagePtr &>())
        .def(init<const UsdStagePtr &, const This::EditOptions &>())

        .def("AddDependentStage", &This::AddDependentStage)
        .def("RemoveDependentStage", &This::RemoveDependentStage)
        .def("SetDependentStages", &This::SetDependentStages)

        .def("DeletePrimAtPath", &This::DeletePrimAtPath)
        .def("MovePrimAtPath", &This::MovePrimAtPath)

        .def("DeletePrim", &This::DeletePrim)
        .def("RenamePrim", &This::RenamePrim)
        .def("ReparentPrim", 
            (bool (This::*) (const UsdPrim &, const UsdPrim &))
               &This::ReparentPrim)
        .def("ReparentPrim", 
            (bool (This::*) (const UsdPrim &, const UsdPrim &, 
                const TfToken &)) &This::ReparentPrim)

        .def("DeletePropertyAtPath", &This::DeletePropertyAtPath)
        .def("MovePropertyAtPath", &This::MovePropertyAtPath)

        .def("DeleteProperty", &This::DeleteProperty)
        .def("RenameProperty", &This::RenameProperty)
        .def("ReparentProperty", 
            (bool (This::*) (const UsdProperty &, const UsdPrim &))
               &This::ReparentProperty)
        .def("ReparentProperty", 
            (bool (This::*) (const UsdProperty &, const UsdPrim &, 
                const TfToken &)) &This::ReparentProperty)

        .def("ApplyEdits", &This::ApplyEdits)
        .def("CanApplyEdits", &_CanApplyEdits)
        .def("GetLayersToEdit", &This::GetLayersToEdit,
            return_value_policy<TfPySequenceToList>())
    ;

    class_<This::EditOptions>("EditOptions")
        .def(init<>())
        .add_property("allowRelocatesAuthoring", 
            &This::EditOptions::allowRelocatesAuthoring, 
            &This::EditOptions::allowRelocatesAuthoring)
    ;

    class_<This::CanApplyResult>("CanApplyResult")
        .def(init<>())
        .def("__bool__", &This::CanApplyResult::operator bool)
        .add_property("whyNot", &_FormatWhyNot)
        .add_property("errors", make_getter(&This::CanApplyResult::errors, 
            return_value_policy<TfPySequenceToList>()))
        .add_property("warnings", make_getter(&This::CanApplyResult::warnings, 
            return_value_policy<TfPySequenceToList>()))
    ;
}
