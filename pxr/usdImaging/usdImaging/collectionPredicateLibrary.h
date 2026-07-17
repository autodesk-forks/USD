//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_IMAGING_COLLECTION_PREDICATE_LIBRARY_H
#define PXR_USD_IMAGING_USD_IMAGING_COLLECTION_PREDICATE_LIBRARY_H

/// \file usdImaging/collectionPredicateLibrary.h

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/collectionPredicateLibrary.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \defgroup group_usdImaging_collectionPredicates UsdImaging Collection Predicate API
/// Predicate functions mirroring UsdGetCollectionPredicateLibrary for use in
/// path expressions evaluated on prims in a Hydra scene index populated from
/// a USD stage.
/// @{
///
/// The functions \ref UsdImagingGetCollectionPredicateLibrary provide the
/// following predicate functions.  All predicates read from the
/// \c __usdPrimInfo data source (UsdImagingUsdPrimInfoSchema) stored on each
/// scene index prim, and mirror the semantics of the corresponding functions
/// in UsdCollectionPredicateLibrary.
///
/// \li \c abstract(bool isAbstract = true)
///     Because abstract USD prims are not transported to Hydra, this predicate
///     will always return false when isAbstract=true, and vice versa.
///     The predicate is provided for completeness sake.
///
/// \li \c defined(bool isDefined = true)
///     Returns true if the scene index prim's specifier is "def",
///     corresponding to UsdPrim::IsDefined().
///
/// \li \c model(bool isModel = true)
///     Returns true if the scene index prim's kind metadata is in the
///     "model" kind hierarchy (component, group, assembly, or any registered
///     sub-kind), corresponding to UsdPrim::IsModel().
///
/// \li \c group(bool isGroup = true)
///     Returns true if the scene index prim's kind metadata is in the
///     "group" kind hierarchy (group, assembly, or any registered sub-kind),
///     corresponding to UsdPrim::IsGroup().
///
/// \li \c kind(kind1, ... kindN, strict = false)
///     Returns true if the scene index prim's kind metadata is one of
///     kind1...kindN.  When strict is false (the default), sub-kinds are
///     matched via KindRegistry::IsA().  When strict is true, only an exact
///     kind match is accepted.
///
/// \li \c specifier(spec1, ... specN)
///     Returns true if the scene index prim's specifier is one of
///     spec1...specN.  Arguments must be unnamed strings: "over", "class",
///     or "def".
///
/// \li \c isa(schema1, ... schemaN, strict = false)
///     Returns true if the scene index prim's typeName (from
///     UsdImagingUsdPrimInfoSchema) resolves to a USD typed schema that is
///     one of schema1...schemaN (or a subtype if strict is false).
///
/// \li \c hasAPI(apiSchema1, ... apiSchemaN, [instanceName = name])
///     Returns true if the scene index prim's applied API schema list
///     contains an entry matching one of apiSchema1...apiSchemaN.  For
///     multi-apply schemas, any instance name matches when instanceName is
///     not supplied; the exact instance name is required when it is supplied.
///
/// \li \c variant(set1=selGlob1, ... setN=selGlobN)
///     Return true if the scene index prim has selections matching
///     the literal names or glob patterns selGlob1...selGlobN for the variant
///     sets set1...setN.
///
/// \section usdImaging_predicate_usage Usage Examples
///
/// \c "/World//{kind:component}" matches all descendant prims of /World
/// that have kind "component" (or a registered sub-kind).
///
/// \c "//{defined}" matches all scene index prims with specifier "def".
///
/// \c "//{model}" matches all scene index prims whose kind is in the
/// model hierarchy.
///
/// \c "//{isa:Mesh}" matches all scene index prims whose USD type is Mesh.
///
/// \c "//{hasAPI:GeomModelAPI}" matches all scene index prims that have
/// the GeomModelAPI applied.
///
/// \c "//{variant(shadingVariant=default)}" matches all scene index prims that
/// have the variant set "shadingVariant" selected to "default".
///

/// \brief Return a predicate library containing only the USD predicate
/// functions listed above.
///
/// \note The library does not include the core Hydra predicate functions.
///       Use the overload below if that is desired.
///
USDIMAGING_API
const HdCollectionPredicateLibrary &
UsdImagingGetCollectionPredicateLibrary();

/// \brief Return a new predicate library that extends \p base with the USD
/// predicate functions listed above.
///
/// A typical use is to compose the USD predicates on top of the core Hydra
/// predicates so that a single evaluator can match both kinds of expression:
///
/// \code
///   HdCollectionExpressionEvaluator eval(
///       sceneIndex,
///       expr,
///       UsdImagingGetCollectionPredicateLibrary(
///           HdGetCollectionPredicateLibrary()));
/// \endcode
///
/// The returned library is a value; callers that call this frequently with
/// the same base should cache the result themselves.
///
/// \sa HdGetCollectionPredicateLibrary
/// \sa HdCollectionExpressionEvaluator
///
USDIMAGING_API
HdCollectionPredicateLibrary
UsdImagingGetCollectionPredicateLibrary(
    const HdCollectionPredicateLibrary &base);

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_COLLECTION_PREDICATE_LIBRARY_H
