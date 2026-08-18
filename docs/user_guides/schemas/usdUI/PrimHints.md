% Note: This file is _not_ generated, as UIHints are not defined
% in schema.usda

(PrimHints)=
# PrimHints

PrimHints provide UI hints for prims presented in a UI. PrimHints cover 
prim-level hints, such as how and when 
{ref}`display groups <usdUI_uihints_display_groups>` are displayed in a UI.

Note that prims also have access to {ref}`ObjectHints`, for authoring prim
display names and hidden (in a UI) state.

The following simple example shows a prim with several attributes in two 
display groups, "Controller" and "Widget Settings". The prim UI hints indicate 
that a DCC Tool should initially display "Controller" fully expanded and 
"Widget Settings" not expanded. Also, "Widget Settings" should only be displayed 
if the "widgetReadOnlyMode == 0" expression evaluates to true.

```{code-block} usda
def "ControllerA" (
    uiHints = {
        # UI hints from ObjectHints
        string displayName = "ControllerA"
        bool hidden = 0

        dictionary displayGroupsExpanded = {
            bool "Controller" = 1
            bool "Widget Settings" = 0
        }
        dictionary displayGroupsShownIf = {
            string "Widget Settings" = "widgetReadOnlyMode == 0"
        }

    }
)
{
    float controlValue = 1.0 (
        uiHints = {
            string displayName = "value"
            string displayGroup = "Controller"
        }
    )
    bool controlIsOffset = false (
        uiHints = {
            string displayName = "is offset"
            string displayGroup = "Controller"
        }
    )
    bool showWidget = true (
        uiHints = {
            string displayName = "show"
            string displayGroup = "Widget Settings"
        }
    )
    color3f[] widgetColor = (1.0, 0.5, 0.5) (
        uiHints = {
            string displayName = "color"
            string displayGroup = "Widget Settings"
        }
    )
    float widgetSize = 10.0 (
        uiHints = {
            string displayName = "size"
            string displayGroup = "Widget Settings"
        }
    )

    bool widgetReadOnlyMode = 0 
}

```

Here's a mocked-up example of how this prim might appear in a scene browser UI, 
with "widgetReadOnlyMode" set to false.

![UI mock-up for prim hints](uihints-primhints.svg)

```{contents}
:depth: 2
:local:
:backlinks: none
```

(PrimHints_fields)=

## PrimHints fields

(PrimHints_displayGroupsExpanded)=

### displayGroupsExpanded

**USD type**: `dictionary`

A dictionary, keyed by display group name, that indicates if a display group
should be expanded or collapsed by default in a UI. 

(PrimHints_displayGroupsShownIf)=

### displayGroupsShownIf

**USD type**: `dictionary`

A dictionary, keyed by display group name, that contains expressions (based
on {usdcpp}`SdfBooleanExpression`) that are used to conditionally control if
if a display group should be shown in a UI. See 
{ref}`usdUI_uihints_expressions` for more information about boolean expressions.