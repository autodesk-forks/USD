//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_TRY_INVOKE_H
#define PXR_BASE_TF_TRY_INVOKE_H

/// \file tf/tryInvoke.h

#include "pxr/pxr.h"

#include <functional>
#include <optional>
#include <type_traits>

/// Return the result type of \c TfTryInvoke<Ret> in the case where the function
/// was not invoked -- \c false if \p Ret is \c void, otherwise an empty \c
/// std::optional<Ret>.  Call this when you need to produce the same result type
/// as \c TfTryInvoke in a context where the function was not called.
///
/// \see TfTryInvoke
template <class Ret>
constexpr auto
TfNotInvoked()
{
    if constexpr (std::is_void_v<Ret>) {
        return false;
    } else {
        return std::optional<Ret>{};
    }
}

/// Invoke \p fn with \p args if \p fn is invocable with those arguments and
/// return a result that indicates both whether the call was made and, if so,
/// what it returned.
///
/// The template parameter \p Ret specifies the expected return type of
/// \p fn:
/// - If \p fn is invocable and \p Ret is non-void, invoke \p fn and return \c
///   std::optional<Ret> containing the result.
/// - If \p fn is invocable and \p Ret is void, invoke \p fn and return \c true.
/// - If \p fn is not invocable with \p args, return \c TfNotInvoked<Ret>() --
///   an empty \c std::optional<Ret> or \c false.
///
/// The return value is always truthy if \p fn was invoked and falsy if it was
/// not, regardless of whether \p Ret is void.
///
/// Use this to build operations over heterogeneous collections where a callable
/// may only handle a subset of element types:
///
/// \code
/// auto process = [](std::string const &s) { return s.size(); };
///
/// // Invocable -- returns optional<size_t> containing the result.
/// auto r1 = TfTryInvoke<size_t>(process, std::string("hello"));
/// TF_AXIOM(r1 && *r1 == 5);
///
/// // Not invocable -- returns empty optional<size_t>.
/// auto r2 = TfTryInvoke<size_t>(process, 42);
/// TF_AXIOM(!r2);
/// \endcode
///
/// \see TfNotInvoked
template <class Ret, class Fn, class... Args>
auto
TfTryInvoke(Fn &&fn, Args &&...args)
{
    if constexpr (std::is_invocable_v<Fn, Args...>) {
        if constexpr (std::is_void_v<Ret>) {
            std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
            return true;
        } else {
            return std::optional<Ret>(
                std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...));
        }
    } else {
        return TfNotInvoked<Ret>();
    }
}

#endif // PXR_BASE_TF_TRY_INVOKE_H
