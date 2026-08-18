# ExecUsd: Execution system for Usd

The execUsd library is built on top of [exec](../exec/README.md) and
[esfUsd](../esfUsd/README.md).

The execUsd library is the primary entry point for OpenExec. The API defined
here, and in the [exec](../exec/README.md) library, supports:
- Registration of computational behaviors associated with USD schemas (see the
  [Computation Definition Language](#group_Exec_ComputationDefinitionLanguage)
  documentation)
- Ingesting a UsdStage to compile the data flow network that contains nodes that
  embody computations (see ExecUsdSystem)
- Requesting values for efficient, vectorized, multithreaded evaluation (see
  ExecUsdRequest)

See the [OpenExec overview](docs/overview.md) for more details.
