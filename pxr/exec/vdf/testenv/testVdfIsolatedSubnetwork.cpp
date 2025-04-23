//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/grapherOptions.h"
#include "pxr/exec/vdf/grapher.h"
#include "pxr/exec/vdf/isolatedSubnetwork.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/rawValueAccessor.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/simpleExecutor.h"
#include "pxr/exec/vdf/testUtils.h"
#include "pxr/exec/vdf/typedVector.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (axis)
    (moves)
    (input1)
    (input2)
    (out)
);

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define(GfVec3d(0));
}

static const int NUM_POINTS = 10;

static void
GeneratePoints(const VdfContext &context) 
{
    const int size = NUM_POINTS;
    VdfTypedVector<GfVec3d> result;
    result.template Resize<GfVec3d>(size);

    VdfVector::ReadWriteAccessor<GfVec3d> a = 
        result.template GetReadWriteAccessor<GfVec3d>();
    for (int i = 0; i < size; ++i) {
        a[i] = GfVec3d(0, 0, 0);
    }

    VdfRawValueAccessor rawValueAccessor(context);
    rawValueAccessor.SetOutputVector(
        *VdfTestUtils::OutputAccessor(context).GetOutput(),
        VdfMask::AllOnes(size),
        result);
}


static void
TranslatePoints(const VdfContext &context)
{
    // We only expect one value for the "axis" input -- so we use the 
    // GetInputValue API, which is very simple.
    GfVec3d axis = context.GetInputValue<GfVec3d>(_tokens->axis);

    // We don't know how many inputs we will have for the "moves" input, so
    // we will use an iterator, that we'll also use to output our data into.
    VdfReadWriteIterator<GfVec3d> iter(context, _tokens->moves);

    // Now loop over all of our inputs and translate the points.
    for ( ; !iter.IsAtEnd(); ++iter) {
        *iter += axis;
    }

}

static void
AddPoints(const VdfContext &context)
{
    size_t numPoints = 0;
    VdfReadIterator<GfVec3d> it(context, _tokens->input1);
    for (; !it.IsAtEnd(); ++it) {
        ++numPoints;
    }

    VdfTypedVector<GfVec3d> result;
    result.template Resize<GfVec3d>(numPoints);
    VdfVector::ReadWriteAccessor<GfVec3d> a = 
        result.template GetReadWriteAccessor<GfVec3d>();

    VdfReadIterator<GfVec3d> iter(context, _tokens->input1);
    if (context.HasInputValue<GfVec3d>(_tokens->input2)) {
        VdfReadIterator<GfVec3d> iter2(context, _tokens->input2);
    
        size_t i = 0;
        for (; !iter.IsAtEnd(); ++iter, ++iter2, ++i)
            a[i] = (*iter + *iter2);
    } else {
        size_t i = 0;
        for (; !iter.IsAtEnd(); ++iter, ++i)
            a[i] = *iter;
    }

    VdfRawValueAccessor rawValueAccessor(context);
    rawValueAccessor.SetOutputVector(
        *VdfTestUtils::OutputAccessor(context).GetOutput(),
        VdfMask::AllOnes(numPoints),
        result);
}

std::string
MakeTranslateChain(VdfTestUtils::Network &graph, 
                   VdfTestUtils::CallbackNodeType &translateNode,
                   const std::string &first, const std::string &axis, 
                   const VdfMask &axisMask, int num) 
{
    VdfMask allOnes = VdfMask::AllOnes(NUM_POINTS);

    std::string prev = first;
    std::string current = "";
    for (int i = 0; i < num; ++i) {

        current = first + "_" + TfStringify(i);
        graph.Add(current, translateNode);

        graph[axis] >> graph[current].In(_tokens->axis, axisMask);
        graph[prev] >> graph[current].In(_tokens->moves, allOnes);

        prev = current;
    }
    return prev;
}

VdfNode *
BuildTestNetwork1(VdfTestUtils::Network &graph)
{
    // We're going to build a network like this:
    //
    //        Axis1 InputPoints1  Axis2  InputPoints2  Axis3  IP3  Axis4  IP4
    //           \   /               \   /              \      /     \     /
    //          Translate1       Translate2                T3           T4
    //              \                /                      \          /
    //                  AddPoints1                           AddPoints2
    //                        \                                 /
    //                                   AddPointsFinal

    graph.AddInputVector<GfVec3d>("axisInputs", 4);
    graph["axisInputs"]
        .SetValue(0, GfVec3d(1, 0, 0))
        .SetValue(1, GfVec3d(0, 1, 0))
        .SetValue(2, GfVec3d(1, 0, 0))
        .SetValue(3, GfVec3d(0, 1, 0));

    VdfMask axis1Mask(4);
    VdfMask axis2Mask(4);
    VdfMask axis3Mask(4);
    VdfMask axis4Mask(4);
    axis1Mask.SetIndex(0);
    axis2Mask.SetIndex(1);
    axis3Mask.SetIndex(2);
    axis4Mask.SetIndex(3);


    VdfTestUtils::CallbackNodeType generatePoints(&GeneratePoints);
    generatePoints
        .Out<GfVec3d>(_tokens->out);

    graph.Add("inputPoints1", generatePoints);
    graph.Add("inputPoints2", generatePoints);
    graph.Add("inputPoints3", generatePoints);
    graph.Add("inputPoints4", generatePoints);


    VdfTestUtils::CallbackNodeType translatePoints(&TranslatePoints);
    translatePoints
        .Read<GfVec3d>(_tokens->axis)
        .ReadWrite<GfVec3d>(_tokens->moves, _tokens->out)
        ;

    graph.Add("Translate1", translatePoints);
    graph.Add("Translate2", translatePoints);
    graph.Add("Translate3", translatePoints);
    graph.Add("Translate4", translatePoints);


    VdfTestUtils::CallbackNodeType addPoints(&AddPoints);

    addPoints
        .Read<GfVec3d>(_tokens->input1)
        .Read<GfVec3d>(_tokens->input2)
        .Out<GfVec3d>(_tokens->out);
        ;

    graph.Add("AddPoints1",     addPoints);
    graph.Add("AddPoints2",     addPoints);
    graph.Add("AddPointsFinal", addPoints);


    VdfMask allOnes = VdfMask::AllOnes(NUM_POINTS);

    const int numTranslates = 1;

    graph["axisInputs"] >> graph["Translate1"].In(_tokens->axis, axis1Mask);
    graph["inputPoints1"] >> graph["Translate1"].In(_tokens->moves, allOnes);


    std::string lastChain1 = MakeTranslateChain(graph, translatePoints,
            "Translate1", "axisInputs", axis1Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate2"].In(_tokens->axis, axis2Mask);
    graph["inputPoints2"] >> graph["Translate2"].In(_tokens->moves, allOnes);


    std::string lastChain2 = MakeTranslateChain(graph, translatePoints, 
            "Translate2", "axisInputs", axis2Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate3"].In(_tokens->axis, axis3Mask);
    graph["inputPoints3"] >> graph["Translate3"].In(_tokens->moves, allOnes);

    std::string lastChain3 = MakeTranslateChain(graph, translatePoints,
            "Translate3", "axisInputs", axis3Mask, numTranslates);

    graph["axisInputs"] >> graph["Translate4"].In(_tokens->axis, axis4Mask);
    graph["inputPoints4"] >> graph["Translate4"].In(_tokens->moves, allOnes);

    std::string lastChain4 = MakeTranslateChain(graph, translatePoints,
            "Translate4", "axisInputs", axis4Mask, numTranslates);

    graph[lastChain1] >> graph["AddPoints1"].In(_tokens->input1, allOnes);
    graph[lastChain2] >> graph["AddPoints1"].In(_tokens->input2, allOnes);
    graph[lastChain3] >> graph["AddPoints2"].In(_tokens->input1, allOnes);
    graph[lastChain4] >> graph["AddPoints2"].In(_tokens->input2, allOnes);

    graph["AddPoints1"] >> graph["AddPointsFinal"].In(_tokens->input1, allOnes);
    graph["AddPoints2"] >> graph["AddPointsFinal"].In(_tokens->input2, allOnes);

    return graph["AddPointsFinal"];
}

// Utility class to run, stat and graph a network multiple times.
//
class Runner 
{
public :

    Runner(const VdfNetwork &net, VdfNode *out) :
        _allOnes(NUM_POINTS),
        _net(net),
        _out(out)
    {
        _allOnes.SetAll();

        _options.SetUniqueIds(false);
        _options.SetDrawMasks(true);
        _options.SetPrintSingleOutputs(true);
    }

    GfVec3d Snapshot(const std::string &purpose, bool run = true)
    {
        char filename[256];
    
        //
        // Graph network
        //
    
        printf("\n/// Snapshot: %s\n\n", purpose.c_str());

        sprintf(filename, "%s.dot", purpose.c_str());
        VdfGrapher::GraphToFile(_net, filename, _options);
    
        if (!run)
            return GfVec3d(0);

        //
        // Stat network
        //
        _net.DumpStats(std::cerr);

        //
        // Run network
        //
    
        VdfRequest request(VdfMaskedOutput(_out->GetOutput(), _allOnes));

        VdfScheduler::Schedule(request, &_schedule, true /* topologicalSort */);

        _exec.Run(_schedule);

        GfVec3d res = _exec.GetOutputValue(
            *_out->GetOutput(_tokens->out), _allOnes)
                ->GetReadAccessor<GfVec3d>()[0];

        std::cout << "Result is: " << res << std::endl << std::endl;

        return res;
    }

    void Invalidate(const VdfNode &node)
    {
        VdfMaskedOutputVector outputs;
        
        TF_FOR_ALL(i, node.GetOutputsIterator()) {

            VdfOutput *output = i->second;
            VdfMask mask;
        
            if (output->GetAffectsMask()) 
                mask = *output->GetAffectsMask();
            else 
                mask = VdfMask::AllOnes(output->GetNumDataEntries());
        
            outputs.push_back(VdfMaskedOutput(output, mask));
        }

        _exec.InvalidateValues(outputs);
    }

private :

    VdfMask           _allOnes;
    const VdfNetwork &_net;
    VdfNode          *_out;
    VdfSchedule       _schedule;
    VdfSimpleExecutor _exec;
    VdfGrapherOptions _options;
};

int 
main(int argc, char **argv) 
{
    VdfTestUtils::Network graph;

    VdfNode    *out = BuildTestNetwork1(graph);
    VdfNetwork &net = graph.GetNetwork();

    Runner runner(net, out);
    GfVec3d res;

    res = runner.Snapshot("original");

    if (res != GfVec3d(4, 4, 0)) {
        std::cout << "*** Test failed, unexpected result: " << res << std::endl;
        return 1;
    }

    ////////////////////////////////////////////////////////////////////////

    // Applying edit operation...
    std::cout << "/// Editing network..." << std::endl;

    struct Filter : public VdfNetwork::EditFilter
    {
        Filter(Runner &runner)
        :   _runner(runner),
            _nodesAsked(0) {}

        virtual bool CanDelete(const VdfNode *node)
        {
            printf("> asking: %s\n", node->GetDebugName().c_str());
            _nodesAsked++;
            return true;
        }

        Runner &_runner;
        size_t  _nodesAsked;
    };

    // Filter that never allows anything to be deleted.
    struct FilterNever : public VdfNetwork::EditFilter
    {
        virtual bool CanDelete(const VdfNode *node)
        {
            return false;
        }
    };

    Filter filter(runner);

    VdfConnection *connection = graph.GetConnection(
        "Translate2_0:out -> AddPoints1:input2");

    TF_AXIOM(connection);
    
    VdfNode *sourceNode = &connection->GetSourceNode();

    // Invalidate from connection downwards.
    runner.Invalidate(connection->GetTargetNode());

    printf(
        "\nTesting that isolating a single connection with a never filter "
        "will remove the single connection regardless.\n");

    FilterNever never;

    VdfIsolatedSubnetworkRefPtr branch =
        VdfIsolatedSubnetwork::IsolateBranch(connection, &never);

    TF_AXIOM(branch);

    printf("\nTesting that the connection got isolated.\n");
    TF_AXIOM(branch->GetIsolatedConnections().size() == 1);

    printf("\nTesting that isolating a node works.\n");

    branch = VdfIsolatedSubnetwork::IsolateBranch(sourceNode, &filter);

    printf("\nTesting that we isolated the right amount of nodes and connections.\n");
    TF_AXIOM(branch->GetIsolatedNodes().size() == 3);

    printf("\nTesting that the network got reduced in size.\n");
    TF_AXIOM(net.GetNumOwnedNodes() == 13);

    printf("\nTesting that the network computes a new correct result.\n");
    res = runner.Snapshot("removed_branch");
    if (res != GfVec3d(4, 2, 0)) {
        std::cout << "*** Test failed, unexpected result: " << res << std::endl;
        return 1;
    }

    printf("\nOk.\n");

    return 0;
}


