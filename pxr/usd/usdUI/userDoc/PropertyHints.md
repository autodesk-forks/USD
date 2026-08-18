% Note: This file is _not_ generated, as UIHints are not defined
% in schema.usda

(PropertyHints)=
# PropertyHints

PropertyHints provide UI hints for properties presented in a UI. 
PropertyHints cover property-level hints, such as the display group a
property belongs to (if any), and an expression that conditionally controls 
whether the property is shown in a UI or not.

Note that properties also have access to {ref}`ObjectHints`, for authoring 
property display names and hidden (in a UI) state.

The following simple example shows a prim with several properties. Each
property specifies a display group. This example does not demonstrate prim-level
hints for display groups (see {ref}`PrimHints` for an example that 
demonstrates this).

```{code-block} usda
def "PrimWithPropertyHints" (
    uiHints = {
        string displayName = "Example prim"
        bool hidden = 0
    }
)
{
    int exampleAttribute = 1 (
        uiHints = {
            string displayName = "An example attribute"
            bool hidden = 0

            string displayGroup = "Display Group 1"
            string shownIf = "showProperties == 1"
        }
    )

    rel exampleRelationship (
        uiHints = {
            string displayName = "An example relationship"
            bool hidden = 0

            string displayGroup = ""
            string shownIf = "showProperties == 1"
        }
    )

    bool showProperties = true
}

```

```{contents}
:depth: 2
:local:
:backlinks: none
```

(PropertyHints_fields)=

## PropertyHints fields

(PropertyHints_displayGroup)=

### displayGroup

**USD type**: `string`

The name of the display group the property belongs to. A property can only
belong to a single display group. 

A property can specify it belongs to a "nested" display group by using a ":"
separator in the display group name. For example, a property with 
`displayGroup` set to "GroupA:NestedGroup" would belong to the 
"NestedGroup" display group, which would be a child of the "GroupA"
display group. See {ref}`usdUI_uihints_display_groups` for more examples.

(PropertyHints_shownIf)=

### shownIf

**USD type**: `string`

A string containing an expression (based off {usdcpp}`SdfBooleanExpression`) 
that controls if the property is shown in a UI or not. If the expression 
evaluates to true, the property is shown, otherwise it is omitted in a UI.
This hint is used along with the object-level `hidden` hint to determine
property visibility. 

See {ref}`usdUI_uihints_expressions` for more information about boolean
expressions.