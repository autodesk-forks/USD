//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticTrap.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stl.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/return_internal_reference.hpp"

#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {
    
class Tf_PyDiagnosticTrap
{
public:
    TfDiagnosticTrap &__enter__() {
        _trap.reset(new TfDiagnosticTrap {});
        return *_trap;
    }

    void __exit__(object, object, object) {
        _trap->Dismiss();
    }

private:
    std::unique_ptr<TfDiagnosticTrap> _trap;
};

template <class Ret>
static auto
_MakeCallback(object callable)
{
    auto fn = extract<std::function<Ret (object)>>(callable)();
    return TfOverloads {
        [fn](TfError const &e)   { return fn(object(e)); },
        [fn](TfWarning const &w) { return fn(object(w)); },
        [fn](TfStatus const &s)  { return fn(object(s)); }
    };
}

static object
__iter__(TfDiagnosticTrap const &trap)
{
    list diagnostics;
    trap.ForEach(TfOverloads {
            [&diagnostics](TfError const &e)   { diagnostics.append(e); },
            [&diagnostics](TfWarning const &w) { diagnostics.append(w); },
            [&diagnostics](TfStatus const &s)  { diagnostics.append(s); }
        });
    return diagnostics.attr("__iter__")();
}

} // anon

void wrapDiagnosticTrap()
{
    {
        // for ForEach() and predicates -- see USD-12000 that tracks providing a
        // single shared registration mechanism for these.
        TfPyFunctionFromPython<bool (object)>();
        TfPyFunctionFromPython<void (object)>();

        const auto seqToList = return_value_policy<TfPySequenceToList>();
        
        using This = TfDiagnosticTrap;
        class_<This, noncopyable>("_DiagnosticTrapCtx", no_init)
            .def("Dismiss",       &This::Dismiss)
            .def("Clear",         &This::Clear)
            .def("ClearErrors",   &This::ClearErrors)
            .def("ClearWarnings", &This::ClearWarnings)
            .def("ClearStatuses", &This::ClearStatuses)
            .def("IsClean",       &This::IsClean)
            .def("HasErrors",     &This::HasErrors)
            .def("HasWarnings",   &This::HasWarnings)
            .def("HasStatuses",   &This::HasStatuses)
            .def("GetErrors",     &This::GetErrors,   seqToList)
            .def("GetWarnings",   &This::GetWarnings, seqToList)
            .def("GetStatuses",   &This::GetStatuses, seqToList)
            .def("ForEach", +[](This const &trap, object callable) {
                trap.ForEach(_MakeCallback<void>(callable));
            })
            .def("HasAnyMatching", +[](This const &trap, object callable) {
                return trap.HasAnyMatching(_MakeCallback<bool>(callable));
            })
            .def("HasAllMatching", +[](This const &trap, object callable) {
                return trap.HasAllMatching(_MakeCallback<bool>(callable));
            })
            .def("CountMatching", +[](This const &trap, object callable) {
                return trap.CountMatching(_MakeCallback<bool>(callable));
            })
            .def("EraseMatching", +[](This &trap, object callable) {
                trap.EraseMatching(_MakeCallback<bool>(callable));
            })
            .def("__iter__",      &__iter__)
            ;
    }
    {
        using This = Tf_PyDiagnosticTrap;
        class_<This, noncopyable>("DiagnosticTrap")
            .def("__enter__", &This::__enter__, return_internal_reference<>())
            .def("__exit__",  &This::__exit__)
            ;
    }
}
