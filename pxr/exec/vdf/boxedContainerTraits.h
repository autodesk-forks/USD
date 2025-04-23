//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_BOXED_CONTAINER_TRAITS_H
#define PXR_EXEC_VDF_BOXED_CONTAINER_TRAITS_H

#include "pxr/pxr.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
class Vdf_BoxedContainer;

template <typename T>
struct Vdf_IsBoxedContainer : std::false_type
{
};

template <typename T>
struct Vdf_IsBoxedContainer<Vdf_BoxedContainer<T>> : std::true_type
{
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
