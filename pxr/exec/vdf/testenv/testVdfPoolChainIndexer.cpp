//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (childPoints)
);


static void
CallbackFunction(const VdfContext &)
{
}

static void
BuildNetwork(VdfTestUtils::Network &graph)
{
    // Create a pair of callback node types that roughly resemble the
    // movers and parallel movers in a pool chain.

    VdfTestUtils::CallbackNodeType moverType(&CallbackFunction);
    moverType
        .ReadWrite<int>(
            _VdfPoolChainIndexTokens->pool, _VdfPoolChainIndexTokens->pool)
        ;

    VdfTestUtils::CallbackNodeType parallelMoverType(&CallbackFunction);
    parallelMoverType
        .ReadWrite<int>(
            _VdfPoolChainIndexTokens->pool, _VdfPoolChainIndexTokens->pool)
        .Read<int>(_tokens->childPoints)
        ;

    // Pool chain indexing doesn't consider masks, so just use the same one
    // for everything in this test.
    VdfMask mask = VdfMask::AllOnes(1);

    /* Build a network with a parallel mover where Mover3 & 4
     * feed into the childPoints of the parallel mover.
     * 
     *
     *     Mover1
     *         |
     *     Mover2
     *        /|\
     *       / | \
     *  Mover3 |  \
     *    /    |  |
     *   | Mover4 |
     *   |     |  |
     *    \   /   |
     *     \ /   /
     *  ParallelMover
     *          |
     *      Mover5
     *
     */

    graph.Add("Mover1", moverType);
    graph.Add("Mover2", moverType);
    graph.Add("Mover3", moverType);
    graph.Add("Mover4", moverType);
    graph.Add("Mover5", moverType);

    graph.Add("ParallelMover", parallelMoverType);

    VdfTestUtils::Node &mover1 = graph["Mover1"];
    VdfTestUtils::Node &mover2 = graph["Mover2"];
    VdfTestUtils::Node &mover3 = graph["Mover3"];
    VdfTestUtils::Node &mover4 = graph["Mover4"];
    VdfTestUtils::Node &mover5 = graph["Mover5"];

    VdfTestUtils::Node &parallelMover = graph["ParallelMover"];

    mover1.Output(_VdfPoolChainIndexTokens->pool)
        >> mover2.In(_VdfPoolChainIndexTokens->pool, mask);

    // Connect Mover2's pool output to the 3 targets:
    // (Mover3, Mover4, ParallelMover)
    mover2.Output(_VdfPoolChainIndexTokens->pool)
        >> mover3.In(_VdfPoolChainIndexTokens->pool, mask);
    mover2.Output(_VdfPoolChainIndexTokens->pool)
        >> mover4.In(_VdfPoolChainIndexTokens->pool, mask);
    mover2.Output(_VdfPoolChainIndexTokens->pool)
        >> parallelMover.In(_VdfPoolChainIndexTokens->pool, mask);

    // Connect childPoints into the parallel mover
    mover3.Output(_VdfPoolChainIndexTokens->pool)
        >> parallelMover.In(_tokens->childPoints, mask);
    mover4.Output(_VdfPoolChainIndexTokens->pool)
        >> parallelMover.In(_tokens->childPoints, mask);

    // Connect the Mover5 downstream of ParallelMover
    parallelMover.Output(_VdfPoolChainIndexTokens->pool)
        >> mover5.In(_VdfPoolChainIndexTokens->pool, mask);
}

int
main(int argc, char *argv[])
{
    // Test that pool chain indexing places movers in child branches of a
    // parallel mover before the parallel mover in the pool chain index order.

    VdfTestUtils::Network graph;
    BuildNetwork(graph);

    const VdfNetwork &network = graph.GetNetwork();

    VdfPoolChainIndex mover1Index =
        network.GetPoolChainIndex(*graph["Mover1"].GetOutput());
    VdfPoolChainIndex mover2Index =
        network.GetPoolChainIndex(*graph["Mover2"].GetOutput());
    VdfPoolChainIndex mover3Index =
        network.GetPoolChainIndex(*graph["Mover3"].GetOutput());
    VdfPoolChainIndex mover4Index =
        network.GetPoolChainIndex(*graph["Mover4"].GetOutput());
    VdfPoolChainIndex mover5Index =
        network.GetPoolChainIndex(*graph["Mover5"].GetOutput());

    VdfPoolChainIndex parallelMoverIndex =
        network.GetPoolChainIndex(*graph["ParallelMover"].GetOutput());

    TF_AXIOM(mover1Index < mover2Index);

    TF_AXIOM(mover2Index < mover3Index);
    TF_AXIOM(mover2Index < mover4Index);
    TF_AXIOM(mover2Index < parallelMoverIndex);

    TF_AXIOM(mover3Index < parallelMoverIndex);
    TF_AXIOM(mover4Index < parallelMoverIndex);
    TF_AXIOM(mover3Index != mover4Index);

    TF_AXIOM(parallelMoverIndex < mover5Index);

    return 0;
}
