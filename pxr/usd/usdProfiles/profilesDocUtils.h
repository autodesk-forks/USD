//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PROFILES_DOC_UTILS_H
#define PXR_USD_PROFILES_DOC_UTILS_H

/// \file usdProfiles/profilesDocUtils.h

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/api.h"

#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Markdown report of all capabilities and profiles in the registry,
/// grouped by subgraph tag. Two top-level sections:
///   ## Profiles
///   ## Capabilities
/// Each entry includes display name, id, docstring, and predecessors
/// (with deprecation noted).
USDPROFILES_API std::string UsdProfilesMarkdown();

/// Markdown report (as above) with an embedded Mermaid diagram of the
/// full capability DAG inserted before the Profiles section.
/// \p styles overrides registry styles per style-token; if empty, the
/// registry's CapabilityStyles map is used.
USDPROFILES_API std::string UsdProfilesMarkdown(
    const std::map<std::string, std::string>& styles);

/// Mermaid `graph LR` representation of the capability DAG.
/// Edges run capability → predecessor; deprecated edges use a dashed link.
/// Nodes carry a class derived from their style token, and Mermaid
/// `classDef` lines are emitted from \p styles (or registry styles when
/// empty). Subgraph tags are emitted as Mermaid `subgraph` clusters.
USDPROFILES_API std::string UsdProfilesMermaid(
    const std::map<std::string, std::string>& styles);

/// Graphviz DOT representation of the capability DAG. Same edge direction
/// as the Mermaid form. Profiles get a doubled border; deprecated edges
/// are dashed. Subgraph tags become \c cluster_<tag> subgraphs.
USDPROFILES_API std::string UsdProfilesDot();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PROFILES_DOC_UTILS_H
