# Consistent and tractable algorithm for Markov Network learning

This repository contains implementation of algorithms for learning Markov Netwrok classifiers and code to replicate experiments published in

V. Franc, D.Prusa, A.Yermakov: Consistent and tractable algorithm for Markov Network learning. ECML 2022.

# Installation

conda create -n ecml2022 python=3.7 numpy matplotlib pyyaml tqdm 


## Compile ADAG_SOLVER implemented in C++

Compile the ADAG_SOLVER with interface to Python:
```
cd manet/adag_solver/
build_adag_module.sh
```

Set path to the compiled library:
```
LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:ROOT_DIR/manet/adag_solver"
```
change ROOT_DIR to the directory name where the repository is cloned.


# Create datasets

There are three benchmark datasets:
1. Synthetically generated sequences generated from Hidden Markov Chain.
2. Examples of Sudoku puzzles with symbolic input.
3. Examples of Sudoku puzzles with visual inputs composed of MNIST digits.

To generate the datasets run the following scripts:

```
python create_hmc_dataset.py
python create_sudoku_dataset.py
python create_visual_sudoku_dataset.py 
```

