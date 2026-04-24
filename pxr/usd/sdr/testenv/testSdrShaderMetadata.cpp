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

    // Test that setting label's value to an empty VtValue clears the item
    m.SetItem(SdrNodeMetadata->Label, VtValue());
    TF_VERIFY(!m.HasLabel());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Label));
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
TestNodeDomain()
{
    // Domain should be initialized to Rendering by default.
    SdrShaderNodeMetadata m;
    TF_VERIFY(m.HasDomain());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Domain));
    TF_VERIFY(m.GetDomain() == SdrNodeDomain->Rendering);
    TF_VERIFY(m.GetItemValueAs<TfToken>(SdrNodeMetadata->Domain)
              == SdrNodeDomain->Rendering);

    // Empty token values for Domain don't cause re-initialization
    // to Rendering; they are still considered valid values.
    VtDictionary d;
    d[SdrNodeMetadata->Domain] = TfToken();
    m = SdrShaderNodeMetadata(d);
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Domain));
    TF_VERIFY(m.GetDomain() == TfToken());
    TF_VERIFY(m.GetItemValueAs<TfToken>(SdrNodeMetadata->Domain)
              == TfToken());

    // Domain can be cleared.
    m.ClearItem(SdrNodeMetadata->Domain);
    TF_VERIFY(!m.HasDomain());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Domain));
    
    // Domain can be set to a non-empty token value
    m.SetItem(SdrNodeMetadata->Domain, TfToken("foo"));
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Domain));
    TF_VERIFY(m.GetDomain() == TfToken("foo"));
    TF_VERIFY(m.GetItemValueAs<TfToken>(SdrNodeMetadata->Domain)
              == TfToken("foo"));
    TF_VERIFY(m.GetDomain() == TfToken("foo"));

    // Non-empty token values for Domain at construction time persist
    d[SdrNodeMetadata->Domain] = TfToken("bar");
    m = SdrShaderNodeMetadata(d);
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Domain));
    TF_VERIFY(m.GetDomain() == TfToken("bar"));
    TF_VERIFY(m.GetItemValueAs<TfToken>(SdrNodeMetadata->Domain)
              == TfToken("bar"));
}

void
TestSdrShaderNodeMetadata()
{
    TestNodeLabel();
    TestNodeOpenPages();
    TestNodeDomain();
}

int main()
{
    TestSdrShaderNodeMetadata();
}
