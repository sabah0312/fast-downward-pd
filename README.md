
# fast-downward-pd

This repository extends the [Fast Downward](https://github.com/aibasel/downward) planning system with a module `decompose` that facilitates various plan deordering and decomposition strategies.

The **decompose** module incorporates postprocessing techniques that take a total-order (sequential) plan and generate a partial-order plan (POP) using **EOG-based deordering** `(Kambhampati and Kedar, 1994)`. Then further minimize the orderings of the POP using **Block-deordering** `(Siddiqui and Haslum, 2012; Noor and Siddiqui, 2024a)`. *Block Deordering* encapsulates coherent action sets (i.e., subplans) into blocks to eliminate more orderings in a partial-order plan, resulting in a hierarchically structured plan, known as the *Block Decomposed Partial-Order (BDPO)* plan.

In this `decompose` module, we include the concept **block-substitution** `(Noor and Siddiqui, 2024b)`, which facilitates substituting subplans (i.e., blocks) in a BDPO plan with action sets outside of the plan for the corresponding planning problem. We also incorporate *non-concurrency constraints* in POPs to facilitate the parallel execution of actions within a plan.  

This module includes two methods **FIBS (Flexibility Improvement via Block-Substitution)** `(Noor and Siddiqui, 2024b)` and **CIBS (Concurrency Improvement via Block-Substitutio)** `(Noor and Siddiqui, 2025)` that uses *block-substitution* to improve flexibility and concurrency of a BDPO plan, respectively.

We also include other features such as **Plan Reduction**, which eliminates redundant actions from a plan using Forward Justification or Backward Justification.

## 🔍 Overview

- **EOG (Explanation-based Order Generalization)**: Transforms sequential plans into partial-order plans by eliminating unnecessary orderings.
- **Block Deordering**: Encapsulate coherent action sets (i.e., subplans) into blocks to eliminate more orderings in a partial-order plan, resulting in a Block Decomposed Partial-Order (BDPO) plan.
- **Block Substitution**:  Substitute blocks (i.e., subplans) in a BDPO plan with action sets outside of the plan with respect to the corresponding planning problem,
- **Flexibility Improvement via Block-Substitution (FIBS)**: Eliminate orderings in a BDPO plan by exploiting blocks as candidate subplans to substitute via block-substitution.
- **Concurrency Improvement via Block-Substitution (CIBS)**: Facilitate `concurrency` (parallel execution of actions) by incorporating non-concurrency constraints, and  improve concurrency via block-substitution in a BDPO plan.
- **Plan Reduction**: eliminates redundant actions from a plan using Forward Justification or Backward Justification.


## ▶️ Usage
The fast-downward-pd.py script supports various deordering methods to convert a sequential plan into a partial-order plan and to minimize the ordering and non-concurrency constraints in the plan
#### Basic Syntax
```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> <sequential_plan> --decompose '<method>'
```

#### EOG (Explanation-based Order Generalization)
```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> <sequential_plan> --decompose 'eog()'
```

#### Block Deordering
```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> <sequential_plan> --decompose 'block_deorder()'
```

#### FIBS (Flexibility Improvement via Block-Substitution)

*Basic FIBS decomposition*

```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> <sequential_plan> --decompose 'fibs()'
```

*FIBS with concurrency enabled (allows parallel execution of actions by incorporating non-concurrency constraints)* 

```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> <sequential_plan> --decompose 'fibs(concurrency=true)'
```

*FIBS with plan reduction*

```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> <sequential_plan> --decompose 'fibs(plan_reduction=FJ)'
```

#### Explanation of plan_reduction Options
The plan_reduction parameter specifies which plan reduction algorithm will be used:

|Value      |	Description           |
|-----------|-----------------------|
|BJ         |	Backward Justification|
|FJ         |	Forward Justification |
|NOREDUCTION| No plan reduction     |

#### CIBS (Concurrency Improvement via Block-Substitution)

```bash
./fast-downward-pd.py domain.pddl instance-1.pddl sas_plan.1.lama --decompose 'cibs()'
```

CIBS also allows plan reduction

#### Test 
```bash
./fast-downward-pd.py ./tests/gripper/domain.pddl ./tests/gripper/instance-8.pddl ./tests/gripper/sas_plan.1.lama --decompose 'block_deorder()'
```

```bash
./fast-downward-pd.py ./tests/blocks/domain.pddl ./tests/blocks/instance-13.pddl ./tests/blocks/sas_plan.1.lama --decompose 'fibs()'
```

```bash
./fast-downward-pd.py ./tests/rovers/domain-15.pddl ./tests/rovers/instance-15.pddl ./tests/rovers/sas_plan.1.lama --decompose 'cibs()'
```
For more IPC benchmark problems and solutions, see [IPC_Solutions](https://github.com/sabah0312/IPC-Solutions)
## Compilation

To build, from the top-level directory, run:

```bash
./build.py
```

This creates the default build `release` in the directory `builds`. For information on alternative builds (e.g. `debug`) and further options, call
`./build.py --help`. 
For more information, see [BUILD.md](BUILD.md).

## 📚 Class Hierarchy

```bash
DeorderAlgorithm (deorder_algorithm.h)
└── EOG (eog.h)
    └── BlockDeorder (block_deorder.h)
        ├── CIBS (cibs.h)
        └── FIBS (fibs.h)
```
`src/decompose/` directory contains the core source files for our `decompose` module.

## 📄 Publications and References
This project is based on and extends work published in the following papers:

Noor, S. B., & Siddiqui, F. H. (2025). Improving plan execution flexibility using block substitution. In Proceedings of the International Conference on Autonomous Agents and Multi-Agent Systems (AAMAS).

Noor, S. B., & Siddiqui, F. H. (2024a). Improving execution concurrency in partial‑order plans via block‑substitution. arXiv preprint, arXiv:2406.18615.

Noor, S. B., & Siddiqui, F. H. (2024b). Revisiting block deordering in finite-domain state-variable planning. AI Communications. https://doi.org/10.3233/AIC-230058

Siddiqui, F. H., & Haslum, P. (2012). Block-structured plan deordering. In AI 2012: Advances in Artificial Intelligence (pp. 803–814).

Kambhampati, S., & Kedar, S. (1994). A unified framework for explanation-based generalization of partially ordered and partially instantiated plans. Artificial Intelligence, 67(1), 29–70. https://doi.org/10.1016/0004-3702(94)90011-6

# Note on Compatibility
All core functionalities of Fast Downward are fully supported. The only change is to use `fast-downward-pd.py` instead of `fast-downward.py`.

This script serves as a wrapper around the original Fast Downward pipeline, adding support for additional modules for the `decompose` module.

For standard planning tasks, usage remains the same, except for the script name:

```bash
./fast-downward-pd.py <domain.pddl> <problem.pddl> --search "astar(blind())"
```
To learn more about fast-downward, see [FASTDOWNWARD.md](FASTDOWNWARD.md)
