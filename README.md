
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

## Compilation

To build, from the top-level directory, run:

```bash
./build.py
```

This creates the default build `release` in the directory `builds`. For information on alternative builds (e.g. `debug`) and further options, call
`./build.py --help`. 
For more information, see [BUILD.md](BUILD.md).

## 📄 Publications and References
This project is based on and extends work published in the following papers:


Noor, S. B., & Siddiqui, F. H. (2025). Improving plan execution flexibility using block substitution. In Proceedings of the International Conference on Autonomous Agents and Multi-Agent Systems (AAMAS).

Noor, S. B., & Siddiqui, F. H. (2024a). Improving execution concurrency in partial‑order plans via block‑substitution. arXiv preprint, arXiv:2406.18615.

Noor, S. B., & Siddiqui, F. H. (2024b). Revisiting block deordering in finite-domain state-variable planning. AI Communications. https://doi.org/10.3233/AIC-230058

Siddiqui, F. H., & Haslum, P. (2012). Block-structured plan deordering. In AI 2012: Advances in Artificial Intelligence (pp. 803–814).

Kambhampati, S., & Kedar, S. (1994). A unified framework for explanation-based generalization of partially ordered and partially instantiated plans. Artificial Intelligence, 67(1), 29–70. https://doi.org/10.1016/0004-3702(94)90011-6


# Fast Downward

Fast Downward is a domain-independent classical planning system.

Copyright 2003-2025 Fast Downward contributors (see below).

For further information:
- Fast Downward website: <https://www.fast-downward.org>
- Report a bug or file an issue: <https://issues.fast-downward.org>
- Fast Downward mailing list: <https://groups.google.com/forum/#!forum/fast-downward>
- Fast Downward main repository: <https://github.com/aibasel/downward>

## Scientific experiments

We recommend to use the [latest release](https://github.com/aibasel/downward/releases/latest) instead of the tip of the main branch.
The [Downward Lab](https://lab.readthedocs.io/en/stable/) Python package helps running Fast Downward experiments.
Our separate [benchmark repository](https://github.com/aibasel/downward-benchmarks) contains a collection of planning tasks.

## Supported software versions

The planner is mainly developed under Linux; and all of its features should work with no restrictions under this platform.
The planner should compile and run correctly on macOS, but we cannot guarantee that it works as well as under Linux.
The same comment applies for Windows, where additionally some diagnostic features (e.g., reporting peak memory usage when the planner is terminated by a signal) are not supported.
Setting time and memory limits and running portfolios is not supported under Windows either.

This version of Fast Downward has been tested with the following software versions:

| OS           | Python | C++ compiler                                                     | CMake |
| ------------ | ------ | ---------------------------------------------------------------- | ----- |
| Ubuntu 24.04 | 3.10   | GCC 14, Clang 18                                                 | 3.30  |
| Ubuntu 22.04 | 3.10   | GCC 12, Clang 15                                                 | 3.30  |
| macOS 14     | 3.10   | AppleClang 15                                                    | 3.30  |
| macOS 13     | 3.10   | AppleClang 15                                                    | 3.30  |
| Windows 10   | 3.8    | Visual Studio Enterprise 2019 (MSVC 19.29) and 2022 (MSVC 19.41) | 3.30  |

We test LP support with CPLEX 22.1.1 and SoPlex 7.1.1. On Ubuntu we
test both CPLEX and SoPlex. On Windows we currently only test CPLEX,
and on macOS we do not test LP solvers (yet).

## Build instructions

See [BUILD.md](BUILD.md).


## Contributors

The following list includes all people that actively contributed to
Fast Downward, i.e., all people that appear in some commits in Fast
Downward's history (see below for a history on how Fast Downward
emerged) or people that influenced the development of such commits.
Currently, this list is sorted by the last year the person has been
active, and in case of ties, by the earliest year the person started
contributing, and finally by last name.

- 2003-2024 Malte Helmert
- 2008-2016, 2018-2024 Gabriele Roeger
- 2010-2024 Jendrik Seipp
- 2010-2011, 2013-2024 Silvan Sievers
- 2012-2024 Florian Pommerening
- 2013, 2015-2024 Salomé Eriksson
- 2018-2024 Patrick Ferber
- 2021-2024 Clemens Büchner
- 2022-2024 Remo Christen
- 2023-2024 Simon Dold
- 2023-2024 Claudia S. Grundke
- 2024 Martín Pozo
- 2024 Tanja Schindler
- 2024 David Speck
- 2015, 2021-2023 Thomas Keller
- 2018-2020, 2023 Augusto B. Corrêa
- 2023 Victor Paléologue
- 2023 Emanuele Tirendi
- 2021-2022 Dominik Drexler
- 2016-2020 Cedric Geissmann
- 2017-2020 Guillem Francès
- 2020 Rik de Graaff
- 2015-2019 Manuel Heusner
- 2017 Daniel Killenberger
- 2016 Yusra Alkhazraji
- 2016 Martin Wehrle
- 2014-2015 Patrick von Reth
- 2009-2014 Erez Karpas
- 2014 Robert P. Goldman
- 2010-2012 Andrew Coles
- 2010, 2012 Patrik Haslum
- 2003-2011 Silvia Richter
- 2009-2011 Emil Keyder
- 2010-2011 Moritz Gronbach
- 2010-2011 Manuela Ortlieb
- 2011 Vidal Alcázar Saiz
- 2011 Michael Katz
- 2011 Raz Nissim
- 2010 Moritz Goebelbecker
- 2007-2009 Matthias Westphal
- 2009 Christian Muise


## History

The current version of Fast Downward is the merger of three different
projects:

- the original version of Fast Downward developed by Malte Helmert
  and Silvia Richter
- LAMA, developed by Silvia Richter and Matthias Westphal based on
  the original Fast Downward
- FD-Tech, a modified version of Fast Downward developed by Erez
  Karpas and Michael Katz based on the original code

In addition to these three main sources, the codebase incorporates
code and features from numerous branches of the Fast Downward codebase
developed for various research papers. The main contributors to these
branches are Malte Helmert, Gabi Röger and Silvia Richter.


## License

```
Fast Downward is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

Fast Downward is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
```

