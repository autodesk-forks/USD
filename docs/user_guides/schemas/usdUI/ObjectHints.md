% Note: This file is _not_ generated, as UIHints are not defined
% in schema.usda

(ObjectHints)=
# ObjectHints

ObjectHints provide UI hints for all USD objects that can be 
presented in a UI. ObjectHints cover general UI features present in prims
and properties, such as the user-facing name of the object shown in a UI, or 
whether the object is hidden in a UI.

The following simple example shows a prim and an attribute with ObjectHints
for `displayName` and `hidden`. In the UI for a DCC tool, the "Placeholder" prim
would show "ModelA Placeholder" for the name and "Model is annotated" for the 
name for the `isAnnotated` attribute. The `isAnnotated` would be hidden and not 
displayed in the UI by default.

```{code-block} usda
def "Placeholder" (
    uiHints = {
        string displayName = "ModelA placeholder"
        bool hidden = 0
    }
)
{
    bool isAnnotated  (
        uiHints = {
            string displayName = "Model is annotated"
            bool hidden = 1
        }
    )
}
```

```{contents}
:depth: 2
:local:
:backlinks: none
```

(ObjectHints_fields)=

## ObjectHints fields

(ObjectHints_displayName)=

### displayName

**USD type**: `string`

The user-facing name of the object (prim or property). 

(ObjectHints_hidden)=

### hidden

**USD type**: `bool`

A flag indicating if the object should be hidden from the UI. Note that this
hint is for UI purposes only, and should not be interpreted by any renderer 
as making the object hidden from the rendered scene. For example, a Gprim 
with this hint enabled should still be drawn by the renderer.

Although this is just a hint and application developers will interpret how best 
to implement this in an application's UI, Pixar uses this hint as a way of
simplifying hierarchy displays, by hiding only the representation of the object 
itself, not its subtree, and "pulling up" everything below it one level in 
the hierarchical nesting.

For properties, this hint is used along with the property-level `shownIf` hint
to determine property visibility. See {ref}`usdUI_uihints_expressions` for more
details.