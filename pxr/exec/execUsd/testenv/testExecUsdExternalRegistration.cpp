//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/exec/registerSchema.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/exec/vdf/context.h"

PXR_NAMESPACE_USING_DIRECTIVE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (attributeName)
    (primComputation)
);

struct MySchemaType {};


EXEC_REGISTER_SCHEMA(MySchemaType)
{
    self.PrimComputation(_tokens->primComputation)
        .Callback<int>(+[](const VdfContext &ctx) {
            ctx.SetOutput(ctx.GetInputValue<int>(_tokens->attributeName));
        })
        .Inputs(
            AttributeValue<int>(_tokens->attributeName)
        );
}


int main()
{
    return 0;
}
