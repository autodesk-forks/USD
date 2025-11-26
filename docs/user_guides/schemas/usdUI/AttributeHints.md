% Note: This file is _not_ generated, as UIHints are not defined
% in schema.usda

(AttributeHints)=
# AttributeHints

AttributeHints provide UI hints for attributes presented in a UI. 
AttributeHints cover attribute-level hints, such as UI labels for possible
values for the attribute, and an ordering used when displaying the labels
in a sequence in the UI.

Note that attributes also have access to hints from {ref}`ObjectHints` and
{ref}`PropertyHints`.

The following simple example shows a prim with a `priority` attribute that
maps 3 values to labels, and specifies a UI ordering for the labels.

```{code-block} usda
def "PrimWithAttributesWithLabels" (
    uiHints = {
        # UI hints from ObjectHints
        string displayName = "Example Prim"
        bool hidden = 0
    }
)
{
    int priority = 1 (
        uiHints = {
            string displayName = "priority"
            bool hidden = 0

            dictionary valueLabels = {
                int high = 3
                int "very low" = 1
                int med = 2
            }
            token[] valueLabelsOrder = ["very low", "med", "high"]
        }
    )
}
```

Here's a mocked-up example of how this might appear in a UI with a drop-down 
list, showing 'medium' as the selected option.

![UI mock-up for attribute hints](uihints-attributehints.svg)

```{note}
For string and token-valued attributes, use the `allowedTokens` attribute 
metadata to define which token values are allowed for the attribute. If you are
additionally using `valueLabels` and `valueLabelsOrder` UI hints, make sure that 
the UI hint values match your set of allowed tokens.

Additionally, attributes support value limits, which can be used in combination
with attribute UI hints to provide user feedback in a UI on what values are
valid for a given attribute. For more information, see 
{usdcpp}`UsdAttributeLimits`.
```

```{contents}
:depth: 2
:local:
:backlinks: none
```

(AttributeHints_fields)=

## AttributeHints fields

(AttributeHints_valueLabels)=

### valueLabels

**USD type**: `dictionary`

A dictionary, keyed by label name, that maps UI labels to attribute values. 

(AttributeHints_valueLabelsOrder)=

### valueLabelsOrder

**USD type**: `token[]`

An array of tokens that indicate the order in which value labels should be
presented in a UI. This might be used in a drop-down list or other UI element 
that needs to show the list of value labels in a sequence.
