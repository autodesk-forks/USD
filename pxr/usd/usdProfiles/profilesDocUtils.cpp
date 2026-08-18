//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/profilesDocUtils.h"
#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _PredInfo {
    std::string id;
    bool deprecated;
};

struct _CapInfo {
    std::string id;
    std::string displayName;
    std::string docstring;
    std::string style;
    std::string subgraph;
    bool isProfile;
    std::vector<_PredInfo> predecessors;
};

struct _Model {
    // Buckets keyed by subgraph tag (empty string = ungrouped).
    std::map<std::string, std::vector<_CapInfo>> bySubgraph;
    // Flat list of all caps for full-DAG renderings.
    std::vector<_CapInfo> all;
    // Resolved style map: token-string -> mermaid style string.
    std::map<std::string, std::string> styles;
};

static std::string
_GetString(const VtDictionary& d, const std::string& key)
{
    auto it = d.find(key);
    if (it == d.end() || !it->second.IsHolding<std::string>()) {
        return std::string();
    }
    return it->second.UncheckedGet<std::string>();
}

static bool
_GetBool(const VtDictionary& d, const std::string& key)
{
    auto it = d.find(key);
    if (it == d.end() || !it->second.IsHolding<bool>()) {
        return false;
    }
    return it->second.UncheckedGet<bool>();
}

static _Model
_BuildModel(const std::map<std::string, std::string>& styleOverrides)
{
    _Model model;

    std::set<TfToken> caps = UsdProfileRegistry::GetAllCapabilities();
    for (const TfToken& tok : caps) {
        VtDictionary md = UsdProfileRegistry::GetCapabilityMetadata(tok);
        _CapInfo info;
        info.id = tok.GetString();
        info.displayName = _GetString(md, "name");
        if (info.displayName.empty()) {
            info.displayName = info.id;
        }
        info.docstring = _GetString(md, "docstring");
        info.style = _GetString(md, "style");
        info.subgraph = _GetString(md, "subgraph");
        info.isProfile = _GetBool(md, "isProfile");

        auto preds = UsdProfileRegistry::GetPredecessors(tok);
        for (const auto& p : preds) {
            _PredInfo pi;
            pi.id = p.capability.GetString();
            pi.deprecated =
                (p.status == UsdProfileRegistry::QueryStatus::Deprecated);
            info.predecessors.push_back(std::move(pi));
        }
        std::sort(info.predecessors.begin(), info.predecessors.end(),
                  [](const _PredInfo& a, const _PredInfo& b) {
                      return a.id < b.id;
                  });

        model.all.push_back(info);
        model.bySubgraph[info.subgraph].push_back(std::move(info));
    }

    // Stable ordering within each bucket: by display name then id.
    auto cmp = [](const _CapInfo& a, const _CapInfo& b) {
        if (a.displayName != b.displayName) {
            return a.displayName < b.displayName;
        }
        return a.id < b.id;
    };
    for (auto& kv : model.bySubgraph) {
        std::sort(kv.second.begin(), kv.second.end(), cmp);
    }
    std::sort(model.all.begin(), model.all.end(), cmp);

    // Resolve styles: overrides if non-empty, otherwise registry.
    if (!styleOverrides.empty()) {
        model.styles = styleOverrides;
    } else {
        for (const auto& kv : UsdProfileRegistry::GetCapabilityStyles()) {
            model.styles[kv.first.GetString()] = kv.second;
        }
    }

    return model;
}

// Sanitize an id for use as a Mermaid/DOT node identifier.
static std::string
_SanitizeId(const std::string& id)
{
    std::string out;
    out.reserve(id.size());
    for (char c : id) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty() || (out[0] >= '0' && out[0] <= '9')) {
        out.insert(out.begin(), 'n');
    }
    return out;
}

static std::string
_EscapeQuotes(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// Sorted iteration order over subgraph buckets: alphabetical, with the
// empty (ungrouped) bucket emitted last under a "(ungrouped)" header.
static std::vector<std::string>
_SortedSubgraphKeys(const _Model& m)
{
    std::vector<std::string> keys;
    keys.reserve(m.bySubgraph.size());
    for (const auto& kv : m.bySubgraph) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end(),
              [](const std::string& a, const std::string& b) {
                  if (a.empty() != b.empty()) {
                      return !a.empty();  // empty goes last
                  }
                  return a < b;
              });
    return keys;
}

static std::string
_SubgraphHeader(const std::string& key)
{
    return key.empty() ? std::string("(ungrouped)") : key;
}

// --- Markdown ---------------------------------------------------------------

static void
_RenderEntry(std::ostringstream& os, const _CapInfo& c)
{
    os << "#### " << c.displayName << "\n";
    os << "`" << c.id << "`\n\n";
    if (!c.docstring.empty()) {
        os << c.docstring << "\n\n";
    }
    if (!c.predecessors.empty()) {
        os << "**Predecessors:** ";
        for (size_t i = 0; i < c.predecessors.size(); ++i) {
            if (i > 0) {
                os << ", ";
            }
            os << "`" << c.predecessors[i].id << "`";
            if (c.predecessors[i].deprecated) {
                os << " *(deprecated)*";
            }
        }
        os << "\n\n";
    }
}

static std::string _RenderMermaid(const _Model& model);

static std::string
_RenderMarkdown(bool includeMermaid, const _Model& model)
{
    std::ostringstream os;
    os << "# Profile Registry\n\n";

    if (includeMermaid) {
        os << "```mermaid\n";
        os << _RenderMermaid(model);
        os << "```\n\n";
    }

    auto keys = _SortedSubgraphKeys(model);

    auto emitSection = [&](const char* heading, bool wantProfiles) {
        // First check whether any matching entry exists at all; suppress
        // the entire section header if not.
        bool anyOverall = false;
        for (const auto& key : keys) {
            for (const auto& c : model.bySubgraph.at(key)) {
                if (c.isProfile == wantProfiles) {
                    anyOverall = true;
                    break;
                }
            }
            if (anyOverall) break;
        }
        if (!anyOverall) {
            return;
        }
        os << "## " << heading << "\n\n";
        for (const auto& key : keys) {
            const auto& bucket = model.bySubgraph.at(key);
            bool any = false;
            for (const auto& c : bucket) {
                if (c.isProfile == wantProfiles) {
                    any = true;
                    break;
                }
            }
            if (!any) {
                continue;
            }
            os << "### " << _SubgraphHeader(key) << "\n\n";
            for (const auto& c : bucket) {
                if (c.isProfile == wantProfiles) {
                    _RenderEntry(os, c);
                }
            }
        }
    };

    emitSection("Profiles", true);
    emitSection("Capabilities", false);

    return os.str();
}

// --- Mermaid ---------------------------------------------------------------

static std::string
_RenderMermaid(const _Model& model)
{
    std::ostringstream os;
    os << "graph LR\n";

    // Build a stable id -> safeId map.
    std::unordered_map<std::string, std::string> safeIds;
    for (const auto& c : model.all) {
        safeIds[c.id] = _SanitizeId(c.id);
    }

    auto emitNode = [&](const _CapInfo& c) {
        os << "    " << safeIds[c.id]
           << "[\"" << _EscapeQuotes(c.displayName) << "\"]";
        if (!c.style.empty()) {
            os << ":::" << _SanitizeId(c.style);
        }
        os << "\n";
    };

    auto keys = _SortedSubgraphKeys(model);
    for (const auto& key : keys) {
        const auto& bucket = model.bySubgraph.at(key);
        if (key.empty()) {
            for (const auto& c : bucket) {
                emitNode(c);
            }
        } else {
            // Prefix cluster ids with "sg_" so they cannot collide with
            // node ids (e.g. capability "imaging" vs. subgraph "imaging").
            os << "    subgraph sg_" << _SanitizeId(key)
               << "[\"" << _EscapeQuotes(key) << "\"]\n";
            for (const auto& c : bucket) {
                os << "    ";  // extra indent inside cluster
                emitNode(c);
            }
            os << "    end\n";
        }
    }

    // Edges across the full DAG.
    for (const auto& c : model.all) {
        for (const auto& p : c.predecessors) {
            auto it = safeIds.find(p.id);
            const std::string predSafe =
                (it != safeIds.end()) ? it->second : _SanitizeId(p.id);
            os << "    " << safeIds[c.id]
               << (p.deprecated ? " -.-> " : " --> ")
               << predSafe << "\n";
        }
    }

    // Class definitions from resolved style map.
    for (const auto& kv : model.styles) {
        os << "    classDef " << _SanitizeId(kv.first)
           << " " << kv.second << "\n";
    }

    return os.str();
}

// --- Graphviz DOT ----------------------------------------------------------

// Best-effort parse of a `fill:#hex,stroke:#hex` style string into a
// fill color hex value. Returns empty string on no match.
static std::string
_ParseFillColor(const std::string& style)
{
    const std::string key = "fill:";
    auto pos = style.find(key);
    if (pos == std::string::npos) {
        return std::string();
    }
    pos += key.size();
    auto end = style.find(',', pos);
    std::string val =
        (end == std::string::npos)
            ? style.substr(pos)
            : style.substr(pos, end - pos);
    // Trim whitespace.
    size_t a = val.find_first_not_of(" \t");
    size_t b = val.find_last_not_of(" \t");
    if (a == std::string::npos) {
        return std::string();
    }
    return val.substr(a, b - a + 1);
}

static std::string
_RenderDot(const _Model& model)
{
    std::ostringstream os;
    os << "digraph profiles {\n";
    os << "    rankdir=LR;\n";
    os << "    node [shape=box];\n";

    std::unordered_map<std::string, std::string> safeIds;
    for (const auto& c : model.all) {
        safeIds[c.id] = _SanitizeId(c.id);
    }

    auto emitNode = [&](const _CapInfo& c, const std::string& indent) {
        os << indent << safeIds[c.id] << " [";
        os << "label=\"" << _EscapeQuotes(c.displayName)
           << "\\n" << _EscapeQuotes(c.id) << "\"";
        if (c.isProfile) {
            os << ", peripheries=2";
        }
        if (!c.style.empty()) {
            auto sit = model.styles.find(c.style);
            if (sit != model.styles.end()) {
                std::string fill = _ParseFillColor(sit->second);
                if (!fill.empty()) {
                    os << ", style=\"filled\", fillcolor=\""
                       << _EscapeQuotes(fill) << "\"";
                }
            }
        }
        os << "];\n";
    };

    auto keys = _SortedSubgraphKeys(model);
    int clusterIdx = 0;
    for (const auto& key : keys) {
        const auto& bucket = model.bySubgraph.at(key);
        if (key.empty()) {
            for (const auto& c : bucket) {
                emitNode(c, "    ");
            }
        } else {
            os << "    subgraph cluster_" << clusterIdx++ << " {\n";
            os << "        label=\"" << _EscapeQuotes(key) << "\";\n";
            for (const auto& c : bucket) {
                emitNode(c, "        ");
            }
            os << "    }\n";
        }
    }

    for (const auto& c : model.all) {
        for (const auto& p : c.predecessors) {
            auto it = safeIds.find(p.id);
            const std::string predSafe =
                (it != safeIds.end()) ? it->second : _SanitizeId(p.id);
            os << "    " << safeIds[c.id] << " -> " << predSafe;
            if (p.deprecated) {
                os << " [style=dashed]";
            }
            os << ";\n";
        }
    }

    os << "}\n";
    return os.str();
}

} // anonymous namespace

std::string
UsdProfilesMarkdown()
{
    _Model m = _BuildModel({});
    return _RenderMarkdown(false, m);
}

std::string
UsdProfilesMarkdown(const std::map<std::string, std::string>& styles)
{
    _Model m = _BuildModel(styles);
    return _RenderMarkdown(true, m);
}

std::string
UsdProfilesMermaid(const std::map<std::string, std::string>& styles)
{
    _Model m = _BuildModel(styles);
    return _RenderMermaid(m);
}

std::string
UsdProfilesDot()
{
    _Model m = _BuildModel({});
    return _RenderDot(m);
}

PXR_NAMESPACE_CLOSE_SCOPE
