//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/collectionPredicateLibrary.h"
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/kind/registry.h"

#include "pxr/base/arch/regex.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

using FnArg = SdfPredicateExpression::FnArg;
using FnArgs = std::vector<FnArg>;

/// Return true if \p args contain a named "strict" argument with a truthy
/// value (bool true, non-zero int, or string beginning with '1', 'y', 'Y').
/// Returns \p defaultStrict if no "strict" argument is present.
static inline bool
_IsStrict(FnArgs const &args, bool defaultStrict = false)
{
    for (FnArg const &arg : args) {
        if (arg.argName == "strict") {
            if (arg.value.IsHolding<bool>()) {
                return arg.value.UncheckedGet<bool>();
            }
            if (arg.value.IsHolding<int>()) {
                return arg.value.UncheckedGet<int>() != 0;
            }
            if (arg.value.IsHolding<std::string>()) {
                const std::string &s = arg.value.UncheckedGet<std::string>();
                if (!s.empty()) {
                    const char c = s.front();
                    return c == '1' || c == 'y' || c == 'Y';
                }
            }
            return false;
        }
    }
    return defaultStrict;
}

/// Add the USD predicate functions to \p lib.
/// This is the shared implementation used by both public overloads.
static void
_AddUsdPredicates(HdCollectionPredicateLibrary &lib)
{
    using PredicateFunction = HdCollectionPredicateLibrary::PredicateFunction;
    using PredResult = SdfPredicateFunctionResult;

    /// Note: PredResult::MakeVarying is used throughout because USD prim
    /// attributes (specifier, kind, type, API schemas) can vary from prim to
    /// prim and cannot be assumed constant over a prim's descendants.

    // -------------------------------------------------------------------------
    // abstract(bool isAbstract = true)
    //
    // Since abstract prims are not transported to Hydra (see the traversal
    // predicate used by UsdImagingStageSceneIndex), this predicate will always
    // return false when isAbstract=true, and vice versa.
    //
    // The predicate is provided for completeness sake.
    //
    lib.Define(
        "abstract",
        [](const HdSceneIndexPrim &p, bool isAbstract) {
            return isAbstract
                ? PredResult::MakeConstant(false)
                : PredResult::MakeVarying(true);
        },
        {{"isAbstract", true}});

    // -------------------------------------------------------------------------
    // defined(bool isDefined = true)
    //
    // Returns true when the prim's specifier is "def",
    // corresponding to UsdPrim::IsDefined().
    //
    lib.Define(
        "defined",
        [](const HdSceneIndexPrim &p, bool isDefined) {
            const HdTokenDataSourceHandle specDs =
                UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                .GetSpecifier();
            const bool primIsDefined =
                specDs &&
                specDs->GetTypedValue(0.0) ==
                    UsdImagingUsdPrimInfoSchemaTokens->def;
            return PredResult::MakeVarying(primIsDefined == isDefined);
        },
        {{"isDefined", true}});

    // -------------------------------------------------------------------------
    // model(bool isModel = true)
    //
    // Returns true when the prim's kind metadata is in the "model" kind
    // hierarchy, corresponding to UsdPrim::IsModel().
    //
    lib.Define(
        "model",
        [](const HdSceneIndexPrim &p, bool isModel) {
            const HdTokenDataSourceHandle kindDs =
                UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                .GetKind();
            const bool primIsModel =
                kindDs &&
                KindRegistry::IsA(kindDs->GetTypedValue(0.0),
                                  KindTokens->model);
            return PredResult::MakeVarying(primIsModel == isModel);
        },
        {{"isModel", true}});

    // -------------------------------------------------------------------------
    // group(bool isGroup = true)
    //
    // Returns true when the prim's kind metadata is in the "group" kind
    // hierarchy, corresponding to UsdPrim::IsGroup().
    //
    lib.Define(
        "group",
        [](const HdSceneIndexPrim &p, bool isGroup) {
            const HdTokenDataSourceHandle kindDs =
                UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                .GetKind();
            const bool primIsGroup =
                kindDs &&
                KindRegistry::IsA(kindDs->GetTypedValue(0.0),
                                  KindTokens->group);
            return PredResult::MakeVarying(primIsGroup == isGroup);
        },
        {{"isGroup", true}});

    // -------------------------------------------------------------------------
    // kind(kind1, ... kindN, strict = false)
    //
    // Returns true when the prim's kind is one of kind1...kindN.
    // Sub-kinds are matched via KindRegistry::IsA() unless strict=true.
    //
    lib.DefineBinder(
        "kind",
        [](FnArgs const &args) -> PredicateFunction {
            const bool checkSubKinds = !_IsStrict(args, /*default*/false);

            std::vector<TfToken> queryKinds;
            for (FnArg const &arg : args) {
                if (arg.argName.empty() &&
                    arg.value.IsHolding<std::string>()) {
                    TfToken k(arg.value.UncheckedGet<std::string>());
                    if (KindRegistry::HasKind(k)) {
                        queryKinds.push_back(std::move(k));
                    }
                }
            }
            if (queryKinds.empty()) {
                return nullptr;
            }

            return [queryKinds, checkSubKinds](const HdSceneIndexPrim &p) {
                const HdTokenDataSourceHandle kindDs =
                    UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                    .GetKind();
                if (!kindDs) {
                    return PredResult::MakeVarying(false);
                }
                const TfToken primKind = kindDs->GetTypedValue(0.0);
                for (TfToken const &queryKind : queryKinds) {
                    if (checkSubKinds
                            ? KindRegistry::IsA(primKind, queryKind)
                            : primKind == queryKind) {
                        return PredResult::MakeVarying(true);
                    }
                }
                return PredResult::MakeVarying(false);
            };
        });

    // -------------------------------------------------------------------------
    // specifier(spec1, ... specN)
    //
    // Returns true when the prim's specifier is one of spec1...specN.
    // Arguments must be unnamed strings: "over", "class", or "def".
    //
    lib.DefineBinder(
        "specifier",
        [](FnArgs const &args) -> PredicateFunction {
            std::vector<TfToken> querySpecs;
            for (FnArg const &arg : args) {
                if (!arg.argName.empty() ||
                    !arg.value.IsHolding<std::string>()) {
                    return nullptr;
                }
                const std::string &v = arg.value.UncheckedGet<std::string>();
                if (v == "over" || v == "def" || v == "class") {
                    querySpecs.push_back(TfToken(v));
                } else {
                    return nullptr;
                }
            }
            if (querySpecs.empty()) {
                return nullptr;
            }

            return [querySpecs](const HdSceneIndexPrim &p) {
                const HdTokenDataSourceHandle specDs =
                    UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                    .GetSpecifier();
                if (!specDs) {
                    return PredResult::MakeVarying(false);
                }
                const TfToken spec = specDs->GetTypedValue(0.0);
                for (TfToken const &qs : querySpecs) {
                    if (spec == qs) {
                        return PredResult::MakeVarying(true);
                    }
                }
                return PredResult::MakeVarying(false);
            };
        });

    // -------------------------------------------------------------------------
    // isa(schema1, ... schemaN, strict = false)
    //
    // Returns true when the prim's typeName resolves to a USD typed schema
    // that is one of schema1...schemaN (or a subtype thereof if strict=false).
    //
    // Requires the USD schema registry to be initialised with the relevant
    // plugins loaded.  Returns false for any prim whose typeName cannot be
    // resolved to a known TfType.
    //
    lib.DefineBinder(
        "isa",
        [](FnArgs const &args) -> PredicateFunction {
            const bool exactMatch = _IsStrict(args, /*default*/false);

            std::vector<TfType> queryTypes;
            for (FnArg const &arg : args) {
                if (arg.argName.empty() &&
                    arg.value.IsHolding<std::string>()) {
                    const TfType schemaType =
                        UsdSchemaRegistry::GetTypeFromSchemaTypeName(
                            TfToken(arg.value.UncheckedGet<std::string>()));
                    if (schemaType) {
                        queryTypes.push_back(schemaType);
                    }
                }
            }

            return [queryTypes, exactMatch](const HdSceneIndexPrim &p) {
                const HdTokenDataSourceHandle typeNameDs =
                    UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                    .GetTypeName();
                if (!typeNameDs) {
                    return PredResult::MakeVarying(false);
                }
                const TfToken typeName = typeNameDs->GetTypedValue(0.0);
                if (typeName.IsEmpty()) {
                    return PredResult::MakeVarying(false);
                }
                const TfType primType =
                    UsdSchemaRegistry::GetTypeFromSchemaTypeName(typeName);
                if (!primType) {
                    return PredResult::MakeVarying(false);
                }
                for (TfType const &queryType : queryTypes) {
                    if (exactMatch ? primType == queryType
                                   : primType.IsA(queryType)) {
                        return PredResult::MakeVarying(true);
                    }
                }
                return PredResult::MakeVarying(false);
            };
        });

    // -------------------------------------------------------------------------
    // hasAPI(apiSchema1, ... apiSchemaN, [instanceName = name])
    //
    // Returns true when the prim's applied API schema list contains an entry
    // matching one of apiSchema1...apiSchemaN.  The schema names are matched
    // directly against the tokens in the apiSchemas array (as produced by
    // UsdPrim::GetAppliedSchemas()):
    //
    //   - For single-apply schemas the entry equals the schema type name.
    //   - For multi-apply schemas the entry has the form "SchemaName:instance".
    //     When instanceName is not supplied, any instance is accepted.
    //     When instanceName is supplied, only the exact instance is accepted.
    //
    lib.DefineBinder(
        "hasAPI",
        [](FnArgs const &args) -> PredicateFunction {
            TfToken instanceName;
            for (FnArg const &arg : args) {
                if (arg.argName == "instanceName") {
                    if (!arg.value.IsHolding<std::string>()) {
                        return nullptr;
                    }
                    instanceName =
                        TfToken(arg.value.UncheckedGet<std::string>());
                    break;
                }
            }

            // Collect the schema type names from unnamed string arguments.
            // We match them directly against the stored token strings rather
            // than going through UsdSchemaRegistry, so that the predicate
            // works without requiring plugins to be loaded.
            std::vector<TfToken> querySchemaNames;
            for (FnArg const &arg : args) {
                if (arg.argName.empty() &&
                    arg.value.IsHolding<std::string>()) {
                    querySchemaNames.push_back(
                        TfToken(arg.value.UncheckedGet<std::string>()));
                }
            }

            return [querySchemaNames, instanceName](
                       const HdSceneIndexPrim &p) {
                const HdTokenArrayDataSourceHandle apiDs =
                    UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                    .GetApiSchemas();
                if (!apiDs) {
                    return PredResult::MakeVarying(false);
                }
                const VtArray<TfToken> schemas = apiDs->GetTypedValue(0.0);

                for (TfToken const &queryName : querySchemaNames) {
                    for (TfToken const &schema : schemas) {
                        if (instanceName.IsEmpty()) {
                            // Accept exact match (single-apply) or any
                            // instance of a multi-apply schema.
                            if (schema == queryName) {
                                return PredResult::MakeVarying(true);
                            }
                            const std::string prefix =
                                queryName.GetString() + ":";
                            if (TfStringStartsWith(
                                    schema.GetString(), prefix)) {
                                return PredResult::MakeVarying(true);
                            }
                        } else {
                            // Accept only the exact multi-apply instance.
                            const TfToken target(
                                queryName.GetString() + ":" +
                                instanceName.GetString());
                            if (schema == target) {
                                return PredResult::MakeVarying(true);
                            }
                        }
                    }
                }
                return PredResult::MakeVarying(false);
            };
        });

    // -------------------------------------------------------------------------
    // variant(setName1=selGlob1, ... setNameN=selGlobN)
    //
    // Returns true when ALL of the given variant set selections match.
    // All arguments must be named: setName=selectionPattern.
    // If selectionPattern is a valid identifier it is matched exactly;
    // otherwise it is treated as a GLOB pattern (via ArchRegex).
    //
    lib.DefineBinder(
        "variant",
        [](FnArgs const &args) -> PredicateFunction {
            std::vector<std::pair<std::string, std::string>> exactSels;
            std::vector<std::pair<std::string, ArchRegex>> globSels;

            for (FnArg const &arg : args) {
                if (arg.argName.empty() ||
                    !arg.value.IsHolding<std::string>()) {
                    return nullptr;
                }
                const std::string &selStr =
                    arg.value.UncheckedGet<std::string>();
                if (TfIsValidIdentifier(selStr)) {
                    exactSels.push_back({arg.argName, selStr});
                } else {
                    ArchRegex regex(selStr, ArchRegex::GLOB);
                    if (!regex) {
                        return nullptr;
                    }
                    globSels.push_back({arg.argName, std::move(regex)});
                }
            }

            return [exactSels, globSels](const HdSceneIndexPrim &p) {
                const auto varSelsSchema =
                    UsdImagingUsdPrimInfoSchema::GetFromParent(p.dataSource)
                    .GetVariantSelections();
                // Check exact matches first, then glob patterns.
                for (auto const &setNsel : exactSels) {
                    if (const auto tokenDs =
                            varSelsSchema.Get(TfToken(setNsel.first))) {
                        const TfToken sel = tokenDs->GetTypedValue(0.0);
                        if (sel != TfToken(setNsel.second)) {
                            return PredResult::MakeVarying(false);
                        }
                    } else {
                        return PredResult::MakeVarying(false);
                    }
                }
                for (auto const &setNglob : globSels) {
                    if (const auto tokenDs =
                            varSelsSchema.Get(TfToken(setNglob.first))) {
                        const TfToken sel = tokenDs->GetTypedValue(0.0);
                        if (!setNglob.second.Match(sel.GetString())) {
                            return PredResult::MakeVarying(false);
                        }
                    } else {
                        return PredResult::MakeVarying(false);
                    }
                }
                return PredResult::MakeVarying(true);
            };
        });
}

} // anon

const HdCollectionPredicateLibrary &
UsdImagingGetCollectionPredicateLibrary()
{
    static const HdCollectionPredicateLibrary library = []() {
        HdCollectionPredicateLibrary lib; // empty library
        _AddUsdPredicates(lib);
        return lib;
    }();
    return library;
}

HdCollectionPredicateLibrary
UsdImagingGetCollectionPredicateLibrary(
    const HdCollectionPredicateLibrary &base)
{
    HdCollectionPredicateLibrary lib = base;
    _AddUsdPredicates(lib);
    return lib;
}

PXR_NAMESPACE_CLOSE_SCOPE
