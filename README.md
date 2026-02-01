# Analysis Code Samples

This repository contains simplified code examples representative of my PhD work
in experimental nuclear and particle physics. The examples use non-sensitive or
dummy data and are intended to demonstrate coding style and analysis approaches.

## Contents

- C++ waveform extraction from ABCD DAQ `.adr` files
- ROOT-based analysis of HPGe detector data
- Python analysis of GEANT4 simulation results


## GEANT4 Distance Optimization Example

This script compares experimental gamma-ray yields with GEANT4 simulations
to determine the optimal detector–source distance using chi-square minimization.

The code demonstrates:
- detector-by-detector analysis
- normalization of simulations to data
- branching-ratio–corrected efficiency extraction
- CSV export for downstream analysis


Note: Input data files are not included, as they are experiment-specific.
The code is provided for illustration of analysis logic and structure.
