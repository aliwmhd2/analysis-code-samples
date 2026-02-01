# Analysis Code Samples

This repository contains small, self-contained code examples representative of my PhD work in experimental nuclear and particle physics.
The examples are provided to illustrate analysis logic, coding style, and software practices, rather than to reproduce full experimental results.

Input data files are not included, as they are experiment-specific and not publicly shareable.

---

## Contents

### 1. ABCD DAQ waveform extraction (C++)

**File:** `abcd_adr_waveform_exporter.cpp`

Standalone C++ utility to parse binary `.adr` files produced by the ABCD data-acquisition system and export digitized waveforms to CSV format.

Features:
- binary packet parsing at DAQ level
- single-channel or all-channel waveform export
- optional waveform limits per channel
- efficient streaming I/O without external dependencies

This example demonstrates low-level detector data handling and performance-aware C++.

---

### 2. Gamma-ray yield extraction from TOF spectra (ROOT / C++)

**File:** `root_gamma_yield_analysis.cpp`

ROOT-based analysis example to extract gamma-ray yields from time-of-flight spectra using side-band background subtraction and proper error propagation.

Features:
- TOF-based yield extraction
- background subtraction with uncertainty propagation
- neutron energy reconstruction from TOF
- ROOT histogram I/O

This code reflects typical detector-level physics analysis workflows.

---

### 3. GEANT4 simulation comparison and distance optimization (Python)

**File:** `geant4_distance_optimization.py`

Python analysis script to compare experimental gamma-ray yields with GEANT4 simulations and determine the optimal detector–source distance via chi-square minimization.

Features:
- detector-by-detector comparison of experiment and simulation
- normalization of simulations to experimental reference lines
- branching-ratio–corrected efficiency extraction
- CSV export for downstream analysis and plotting

This example illustrates simulation–experiment validation and data-driven optimization.

---

## Notes

- All examples are intentionally simplified and use non-sensitive inputs.
- The code is written to be readable and self-contained, rather than as a full production framework.
- The focus is on clarity, structure, and analysis methodology.

---

## Author

Ali F. Alwars
University of Groningen
Experimental Nuclear and Particle Physics
