//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/evaluationState.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/inputSpec.h"
#include "pxr/exec/vdf/inputVector.h"
#include "pxr/exec/vdf/iterators.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/outputSpec.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"
#include "pxr/exec/vdf/tokens.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (input)
    (out1)
    (out2)
);

static void
CallbackFunction(const VdfContext &context) 
{
}

static void
TestVdfNodeGetOutputErrorConditions()
{
    // This function exercises error conditions in VdfNode::GetOutput().
    VdfNetwork net;

    VdfOutputSpecs emptyOutputSpec;
    VdfOutputSpecs multipleOutputsSpec;
    multipleOutputsSpec
        .Connector<int>(VdfTokens->out)
        .Connector<int>(_tokens->out1)
        .Connector<int>(_tokens->out2)
        ;

    VdfNode *node1 = new VdfTestUtils::CallbackNode(&net, VdfInputSpecs(), 
                                       emptyOutputSpec, &CallbackFunction); 

    VdfNode *node2 = new VdfTestUtils::CallbackNode(&net, VdfInputSpecs(), 
                                       multipleOutputsSpec, &CallbackFunction); 

    // Pull on an invalid output.
    node1->GetOutput(TfToken("naganafindit"));

    // Pull on an invalid output
    node1->GetOutput();

    // Call the single output method on a node with multiple outputs.
    node2->GetOutput();

}

static void 
TestVdfContextErrorConditions()
{
    VdfNetwork net;
    VdfNode *emptyNode = new VdfTestUtils::CallbackNode(&net, 
        VdfInputSpecs(), 
        VdfOutputSpecs(), 
        &CallbackFunction); 

    VdfSimpleExecutor executor;
    VdfMask mask;

    VdfSchedule schedule;
    VdfEvaluationState state(executor, schedule, NULL);
    VdfContext context(state, *emptyNode);

    VdfWeightedReadIterator<int> wi(context, {}, TfToken("nagana"));

    // Error condition, invalid input name specified.
    VdfReadWriteIterator<int> rwi(context, TfToken("nagana"));
}

static void
TestVdfConnectorSpecErrorConditions()
{
    VdfInputSpecs inspecs;
    inspecs
        .ReadConnector<int>(_tokens->input)
        ;

    const VdfInputSpec *spec = inspecs.GetInputSpec(0);

    // Call GetAssociatedOutputName on an input connector.  It is incorrect to 
    // do so and an error will be issued.
    TF_AXIOM(spec->GetAssociatedOutputName() == TfToken(""));
}

int 
main(int argc, char **argv) 
{
    TestVdfNodeGetOutputErrorConditions();

    TestVdfContextErrorConditions();

    TestVdfConnectorSpecErrorConditions();

    return 0;
}
