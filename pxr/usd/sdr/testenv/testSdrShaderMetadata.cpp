//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNodeMetadata.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
TestNodeLabel()
{
    // Test the typical behavior for a token-valued metadata item
    SdrShaderNodeMetadata m;
    TF_VERIFY(!m.HasLabel());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Label));
    m.SetItem(SdrNodeMetadata->Label, TfToken("foo"));
    TF_VERIFY(m.HasLabel());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Label));
    TF_VERIFY(m.GetLabel() == TfToken("foo"));
    TF_VERIFY(m.GetItemValueAs<TfToken>(SdrNodeMetadata->Label)
              == m.GetLabel());
    TF_VERIFY(m.GetItemValue(SdrNodeMetadata->Label)
              == VtValue(TfToken("foo")));
    m.SetItem(SdrNodeMetadata->Label, TfToken(""));
    TF_VERIFY(m.HasLabel());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Label));
    m.ClearLabel();
    TF_VERIFY(!m.HasLabel());

    // Test that ingestion carries over the label value
    VtDictionary d;
    d[SdrNodeMetadata->Label] = TfToken("");
    m = SdrShaderNodeMetadata(std::move(d));
    TF_VERIFY(m.HasLabel());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Label));
    TF_VERIFY(m.GetItemValue(SdrNodeMetadata->Label) == VtValue(TfToken("")));
}

void
TestNodeRole()
{
    // Test that an empty Token value clears Role. This is a
    // special behavior of Role that doesn't apply to other token
    // valued metadata.
    SdrShaderNodeMetadata m;
    TF_VERIFY(!m.HasRole());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Role));
    m.SetRole(TfToken());
    TF_VERIFY(!m.HasRole());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Role));
    m.SetRole(TfToken("hi"));
    TF_VERIFY(m.HasRole());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Role));
    m.SetRole(TfToken());
    TF_VERIFY(!m.HasRole());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Role));

    // Test that ingesting an empty TfToken Role means that
    // the metadata "doesn't have" a Role item.
    VtDictionary d;
    d[SdrNodeMetadata->Role] = TfToken("");
    m = SdrShaderNodeMetadata(d);
    TF_VERIFY(!m.HasRole());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Role));
    // "Get" returns the default constructed type
    TF_VERIFY(m.GetRole() == TfToken());
    TF_VERIFY(m.GetItemValue(SdrNodeMetadata->Role) == VtValue());

    // Test the positive ingestion case
    d[SdrNodeMetadata->Role] = TfToken("hi");
    m = SdrShaderNodeMetadata(d);
    TF_VERIFY(m.HasRole());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Role));
}

void
TestNodeOpenPages()
{
    // Tests the typical behavior for a metadata item with a complex type
    SdrShaderNodeMetadata m;
    m.SetItem(SdrNodeMetadata->OpenPages,
              SdrTokenVec({TfToken("foo"), TfToken("bar")}));
    TF_VERIFY(m.HasOpenPages());
    TF_VERIFY(m.GetOpenPages().size() == 2);

    // Test clearing the item
    m.ClearItem(SdrNodeMetadata->OpenPages);
    TF_VERIFY(!m.HasOpenPages());
    TF_VERIFY(m.GetOpenPages().size() == 0);
}

void
TestSdrShaderNodeMetadata()
{
    TestNodeLabel();
    TestNodeRole();
    TestNodeOpenPages();
}

int main()
{
    TestSdrShaderNodeMetadata();
}
