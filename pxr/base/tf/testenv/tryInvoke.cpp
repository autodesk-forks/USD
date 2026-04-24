//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/tryInvoke.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
Test_TfTryInvoke()
{
    auto strLen = [](std::string const &s) { return s.size(); };
    auto strVisit = [](std::string const &s) {};

    // Invocable, non-void Ret -- returns optional<size_t> with result.
    {
        auto r = TfTryInvoke<size_t>(strLen, std::string("hello"));
        TF_AXIOM(r);
        TF_AXIOM(*r == 5);
    }

    // Invocable, void Ret -- returns true.
    {
        auto r = TfTryInvoke<void>(strVisit, std::string("hello"));
        TF_AXIOM(r);
        static_assert(std::is_same_v<decltype(r), bool>);
    }

    // Not invocable, non-void Ret -- returns empty optional<size_t>.
    {
        auto r = TfTryInvoke<size_t>(strLen, 42);
        TF_AXIOM(!r);
        static_assert(std::is_same_v<decltype(r), std::optional<size_t>>);
    }

    // Not invocable, void Ret -- returns false.
    {
        auto r = TfTryInvoke<void>(strVisit, 42);
        TF_AXIOM(!r);
        static_assert(std::is_same_v<decltype(r), bool>);
    }

    // TfNotInvoked -- non-void returns empty optional, void returns false.
    {
        auto r1 = TfNotInvoked<size_t>();
        TF_AXIOM(!r1);
        static_assert(std::is_same_v<decltype(r1), std::optional<size_t>>);

        auto r2 = TfNotInvoked<void>();
        TF_AXIOM(!r2);
        static_assert(std::is_same_v<decltype(r2), bool>);
    }

    // Return value is always truthy when invoked, falsy when not -- regardless
    // of Ret.
    {
        auto alwaysTrue = [](int) { return true; };
        auto alwaysFalse = [](int) { return false; };

        TF_AXIOM(TfTryInvoke<bool>(alwaysTrue, 42));
        TF_AXIOM(TfTryInvoke<bool>(alwaysFalse, 42)); // invoked, so truthy
        TF_AXIOM(!TfTryInvoke<bool>(alwaysTrue, std::string("not invocable")));
    }

    return true;
}

TF_ADD_REGTEST(TfTryInvoke);
