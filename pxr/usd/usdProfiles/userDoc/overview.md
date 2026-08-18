# Overview

UsdProfiles provides infrastructure for capability-based profile compatibility
queries in USD scenes. It answers questions like: "does this prim satisfy the
requirements of profile X?" and "which USD capabilities does this prim use?"

UsdProfiles has two main components:

- **UsdProfileRegistry** — a singleton that loads capability metadata from
  plugins and exposes the capability DAG for querying.
- **UsdProfilesClaimsAPI** — a prim-level applied schema that records which
  capabilities a prim uses and which profiles it has been declared compatible
  with.

(usdProfiles_capability_dag)=
## The Capability DAG

Capabilities are named features of the USD format, identified by reverse-domain
tokens such as `usd`, `usd.geom.mesh`, or `usd.shading.mtlx`. They are
organized as a directed acyclic graph (DAG) where an edge from capability A to
capability B means "A depends on B" — a prim that uses A implicitly relies on
everything B requires.

Edges can be marked **deprecated**, indicating that a path through that edge
represents an older, superseded route. This lets the registry report three
distinct reachability states for a given ancestor:

- `ValidPath` — reachable via at least one non-deprecated path.
- `Deprecated` — only reachable via deprecated paths.
- `DeprecationConflict` — reachable via both deprecated and non-deprecated
  paths.

### Registering Capabilities via Plugins

Capabilities are registered through `plugInfo.json` entries. The registry
discovers and merges all registered capability data at startup:

```{code-block} json
{
    "Plugins": [{
        "Info": {
            "Profiles": {
                "Capabilities": {
                    "usd": {
                        "name": "USD",
                        "docstring": "Root USD capability",
                        "style": "core",
                        "subgraph": "Foundation"
                    },
                    "usd.geom.mesh": {
                        "predecessors": ["usd"],
                        "name": "Polygon Mesh",
                        "docstring": "Polygonal mesh geometry",
                        "style": "core"
                    },
                    "studio.vfx.26.08": {
                        "predecessors": [
                            "usd.geom.mesh",
                            { "name": "studio.vfx.25.11", "deprecated": true }
                        ],
                        "name": "VFX Profile 26.08",
                        "docstring": "August 2026 VFX pipeline profile",
                        "style": "profile",
                        "isProfile": true
                    }
                },
                "CapabilityStyles": {
                    "core":    "fill:#cce5ff,stroke:#004085",
                    "profile": "fill:#d4edda,stroke:#155724"
                }
            }
        },
        "Name": "MyStudioCapabilities",
        "Type": "resource"
    }]
}
```

Every capability graph must have exactly one root node named `usd`.

### Profiles

A **profile** is a capability node tagged with `"isProfile": true`. Profiles
represent named, coherent sets of capabilities for a specific platform or
pipeline target, such as a versioned studio release target or a platform
conformance level.

Profiles participate in the DAG the same way as any other capability. Expressing
profile supersession uses a deprecated edge from the new profile to the old one:

```{code-block} json
"studio.vfx.26.08": {
    "predecessors": [
        "usd.geom.mesh",
        { "name": "studio.vfx.25.11", "deprecated": true }
    ],
    "isProfile": true
}
```

This declares that `studio.vfx.26.08` requires `usd.geom.mesh` directly (valid
path) and that `studio.vfx.25.11` is now a superseded predecessor (deprecated
path). Querying `HasPredecessor("studio.vfx.26.08", "usd.geom.mesh")` returns
`DeprecationConflict` because two paths exist — one direct and one through the
deprecated `studio.vfx.25.11`.

### Capability Versioning

Capabilities can be versioned with a `_vN` suffix (e.g. `usd.physics_v2`).
`ResolveCapability()` automatically selects the highest registered versioned
form of an unversioned name, so callers can pass `"usd.physics"` and
transparently receive results for `"usd.physics_v2"`. `CoversCapabilities()`
and `HasPredecessor()` both resolve tokens before querying the graph.

(usdProfiles_schema_implied_capabilities)=
## Schema-Implied Capabilities

It is not a requirement that a DCC provides explicit capabilities during usd
export processes, unless there is a reasonable possibility that the schema 
involved might not exist at a second client, and that is considered important
to flag the lack of that schema. USD already guarantees that authored data is
retained even if it is not interpreted by the runtime, so capability
declarations are informative. As such, tools should enforce whatever contract 
they have with users about whether or not capabilities should be provided
during the usd export process.

Rather than requiring DCC tools to hard-code which capabilities each applied
schema implies, API schemas can declare their implications directly in their
`schema.usda` definition via `customData.extraPlugInfo.impliesCapabilities`.
The codegen tool (`usdGenSchema`) propagates this declaration verbatim into the
per-type block of the library's `plugInfo.json`, making it available to the
runtime without any additional registration step.

### Declaring Implications in a Schema

```{code-block} usda
class "ColorSpaceAPI"
(
    inherits = </APISchemaBase>
    customData = {
        string apiSchemaType = "singleApply"
        dictionary extraPlugInfo = {
            string[] impliesCapabilities = ["usd.core.colorspace"]
        }
    }
)
{ ... }
```

`impliesCapabilities` is a plain string array of capability names, like
`predecessors`. It carries only the capability identity. **The strength of
that capability (`hard`, `soft`, or `enhancement`) is not a property of the
schema, it is a property of how a particular application consumes the
capability**, and is therefore recorded on the consumer's
`UsdProfilesClaimsAPI` rather than at the implication site (see
[Recording Capability Usages](#usdProfiles_using_claimsapi)).

A schema may list any number of capabilities, and the same capability can be
listed by multiple schemas. The runtime can use the union to understand the 
capabilities required to work with the layer. When different claim strengths
are made on the same capability at different points in a layer, the strongest
claim applies.

### Inheritance Through the Schema Hierarchy

Schema authors do **not** re-declare `impliesCapabilities` on derived schemas.
`GetSchemaImpliedCapabilities()` walks the full ancestor chain via
`TfType::GetAllAncestorTypes` and unions declarations from every level. A
schema that inherits from `ColorSpaceAPI` therefore implies
`usd.core.colorspace` automatically.

```{code-block} python
from pxr import Usd, UsdProfiles

t = Usd.SchemaRegistry.GetTypeFromName(Usd.Tokens.ColorSpaceAPI)
caps = UsdProfiles.ProfileRegistry.GetSchemaImpliedCapabilities(t)
# caps == ["usd.core.colorspace"]
```

Results are cached on first call per `TfType`, so the walk and plugin
metadata reads happen only once regardless of how many prims apply the schema.

### Production Capability Seeds

For implied capabilities to be useful, the referenced capabilities must
actually exist in the registry. The `pxr/usd/usd` library seeds the part of
the DAG it owns by including a `Profiles.Capabilities` block alongside the
`Types` block in its `plugInfo.json`:

```{code-block} json
"Profiles": {
    "Capabilities": {
        "usd": {
            "name": "USD",
            "docstring": "Root capability for the USD taxonomy.",
            "style": "core",
            "subgraph": "Foundation"
        },
        "usd.core": {
            "predecessors": ["usd"],
            "name": "usd core library",
            "docstring": "Capabilities introduced by the pxr/usd/usd library.",
            "style": "core",
            "subgraph": "Foundation"
        },
        "usd.core.colorspace": {
            "predecessors": ["usd.core"],
            "name": "Color Space",
            "docstring": "Color space authoring via UsdColorSpaceAPI / UsdColorSpaceDefinitionAPI.",
            "style": "core",
            "subgraph": "Foundation"
        }
    }
}
```

Downstream libraries follow the same pattern: each library that introduces new
capabilities owns its own `Profiles.Capabilities` block in its `plugInfo.json`.

(usdProfiles_file_format_implied_capabilities)=
### File Format Plugin Implications

File format plugins (`SdfFileFormat`-derived types) can also declare implied
capabilities using the same `impliesCapabilities` key directly in their
`plugInfo.json` type entry. This is the appropriate mechanism for format
plugins that are not USD API schemas and therefore have no `schema.usda` to
annotate.

For example, `usdMtlx` declares that referencing a `.mtlx` layer requires the
`usd.mtlx.fileformat` capability:

```{code-block} json
"UsdMtlxFileFormat": {
    "bases": ["SdfFileFormat"],
    "extensions": ["mtlx"],
    "formatId": "mtlx",
    "impliesCapabilities": ["usd.mtlx.fileformat"],
    ...
}
```

The corresponding `Profiles.Capabilities` block (in the same `plugInfo.json`)
seeds the part of the DAG that `usdMtlx` owns:

```{code-block} json
"Profiles": {
    "Capabilities": {
        "usd.mtlx": {
            "predecessors": ["usd.core"],
            "name": "usdMtlx library",
            "docstring": "Capabilities introduced by the pxr/usd/usdMtlx library.",
            "style": "core",
            "subgraph": "Foundation"
        },
        "usd.mtlx.fileformat": {
            "predecessors": ["usd.mtlx"],
            "name": "MaterialX File Format",
            "docstring": "Support for referencing .mtlx files as USD layers via UsdMtlxFileFormat.",
            "style": "core",
            "subgraph": "Foundation"
        }
    }
}
```

To query file format implications directly:

```{code-block} python
from pxr import Tf, UsdProfiles, UsdMtlx

t = Tf.Type.FindByName("UsdMtlxFileFormat")
caps = UsdProfiles.ProfileRegistry.GetFileFormatImpliedCapabilities(t)
# caps == ["usd.mtlx.fileformat"]
```

`PopulateCapabilityUsages()` incorporates file format implications
automatically: after traversing the prim namespace for schema-implied
capabilities, it iterates `stage.GetUsedLayers()`, identifies each layer's
file format type, and unions any declared `impliesCapabilities` with the
schema-derived set. Each implied capability that the user has not already
declared a strength for is written to the prim's `capabilityUsages` metadata
at the default strength `"hard"`. Authored entries are preserved verbatim,
so callers can override the strength of any implied capability by calling
`SetCapabilityUsage()` before `PopulateCapabilityUsages()`.

Note that `UsdMtlx` must be imported to guarantee that `FindByName` can find 
the `UsdMtlxFileFormat` type; if the `UsdMtlx` plugin has not been loaded, the
`Type` will not be resolved and consequently neither will the capability.

(usdProfiles_using_claimsapi)=
## Using ClaimsAPI

### Recording Capability Usages

When a DCC tool saves a prim that uses specific USD features, it records the
capabilities used and a degradation class for each:

```{code-block} python
from pxr import Usd, UsdProfiles

stage = Usd.Stage.CreateNew("shot.usda")
prim  = stage.DefinePrim("/Char/Body", "Mesh")
api   = UsdProfiles.ClaimsAPI.Apply(prim)

api.SetCapabilityUsage("usd.geom.mesh",      "hard")
api.SetCapabilityUsage("usd.shading.mtlx",   "hard")
api.SetCapabilityUsage("usd.geom.hairAndFur", "soft")
```

| Degradation class | Meaning |
|---|---|
| `hard` | Load-bearing; a consumer lacking this capability produces incorrect results. |
| `soft` | The prim degrades gracefully if the capability is absent. |
| `enhancement` | Improves quality but its absence is acceptable. |

### Auto-populating Capability Usages from Schema Implications

Schemas may author implications where it is desirable to have capabilities
automatically aggregated. This allows the introduction of some automation -
instead of manually calling `SetCapabilityUsage` for every capability, one can
let `PopulateCapabilityUsages()` do the work automatically. It
traverses the entire namespace rooted at the `ClaimsAPI` prim, inspects every
applied API schema on every descendant (including the root itself), and merges
their implied capabilities into a single dictionary, writing each capability at
the default "hard" strength unless it has already been authored on the prim.The
result is written to the prim's `capabilityUsages` metadata, preserving any 
authored strength and filling in undeclared implied capabilities at the default
`"hard"` strength.

Note that there's no need to imply anything that does not need reporting. 
e.g. `UsdCollections` are always available. There is nothing to imply or report.
Right now colorspace information is allowably optional from an implementation 
that is e.g. only concerned with physics. A layer may or may not care about 
reporting that colorspace information is present.

It is important to recognize that Capabilities are not a ground truth 
"algebraic" solve of composition, they are the means by which users may express 
what they intend on behalf of layers they publish.

```{code-block} python
from pxr import Usd, UsdProfiles

stage  = Usd.Stage.Open("shot.usda")
root   = stage.GetPrimAtPath("/Shot")
claims = UsdProfiles.ClaimsAPI.Apply(root)

merged = claims.PopulateCapabilityUsages()
# merged == {"usd.core.colorspace": "hard"}  (if any descendant has ColorSpaceAPI)
```

This is the recommended approach for pipeline post-processes and DCC save
hooks: apply `ClaimsAPI` to the namespace root (typically the shot or asset
prim), then call `PopulateCapabilityUsages()` once after all
other schemas have been applied. The method handles multi-apply schema
instance deduplication automatically — `ColorSpaceDefinitionAPI:foo` and
`ColorSpaceDefinitionAPI:bar` on the same prim each resolve to the same
`TfType` and therefore contribute the capability only once.

```{admonition} Preserves authored usages
:class: note

`PopulateCapabilityUsages()` merges into the existing `capabilityUsages`
dictionary rather than replacing it. Authored entries survive verbatim; only
capabilities that have no authored strength are added, at the default `"hard"`.
Call `SetCapabilityUsage` *before* `PopulateCapabilityUsages` to lock in a
non-default strength for any implied capability.
```

### Declaring Profile Compatibility

A pipeline conformance step declares which profiles the prim satisfies. A
fully compatible declaration (no exceptions) is an unqualified assertion:

```{code-block} python
api.SetProfileCompatible("studio.vfx.26.08")
```

It may be that certain capabilities are present but known to fail a strict
validation check (using the pending UsdValidation extensions for Capabilities
and Profiles), These capabilities may be listed as exceptions via a
deprecation declaration:

```{code-block} python
api.SetProfileCompatibleWithExceptions(
    "studio.mobile.v1",
    ["usd.geom.hairAndFur"])
```

### Querying Compatibility

`IsCompatibleWith()` performs the full compatibility check. It calls
`UsdProfileRegistry::CoversCapabilities()` with the stored capability usages
as the required set and the stored exception list for that profile:

```{code-block} python
PR = UsdProfiles.ProfileRegistry
QS = PR.QueryStatus

status, results = api.IsCompatibleWith("studio.vfx.26.08")

if status == QS.ValidPath:
    print("fully compatible")
elif status == QS.Deprecated:
    print("compatible via a deprecated path only")
elif status == QS.DeprecationConflict:
    print("compatible via both deprecated and non-deprecated paths")
elif status == QS.NoPath:
    print("not compatible, or profile not declared")

# per-capability detail
for r in results:
    print(f"  {r.capability}: {r.status}")
```

`IsCompatibleWith()` returns `QueryStatus.NoPath` immediately if the profile
has not been declared compatible via `SetProfileCompatible` or
`SetProfileCompatibleWithExceptions`, without consulting the registry.

```{admonition} Querying Without a Stage
:class: note

`UsdProfileRegistry` static methods (`HasPredecessor`, `CoversCapabilities`,
etc.) can be called without a stage. They query only the global capability
DAG, making them suitable for tooling and validation pipelines that do not
have a scene loaded.
```

(usdProfiles_data_layout)=
## Data Layout

All ClaimsAPI data is stored in the prim's `customData` dictionary under the
key `profilesInfo`, using two sub-dictionaries:

```{code-block} usda
def Mesh "Body" (
    prepend apiSchemas = ["ClaimsAPI"]
)
{
    # mesh attributes ...
}
```

Reading the raw dictionary back:

```{code-block} python
info = api.GetProfilesInfo()
# {
#   "capabilityUsages": {
#       "usd.geom.mesh":      "hard",
#       "usd.shading.mtlx":   "hard",
#       "usd.geom.hairAndFur": "soft"
#   },
#   "profileCompatibility": {
#       "studio.vfx.26.08": [],
#       "studio.mobile.v1": ["usd.geom.hairAndFur"]
#   }
# }
```

`GetProfilesInfo()` / `SetProfilesInfo()` expose the full dictionary for bulk
access. The typed accessors (`GetCapabilityUsages`, `GetCompatibleProfiles`,
etc.) read and write the same storage.
