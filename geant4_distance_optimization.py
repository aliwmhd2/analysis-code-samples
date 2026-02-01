"""
geant4_distance_optimization.py

Determine the optimal detector–source distance by comparing experimental
gamma-ray yields with GEANT4 simulations using chi-square minimization.

This script is adapted from PhD analysis work and uses simplified,
non-sensitive input files for demonstration purposes.

Author: Ali F. Alwars
"""

import glob
import copy
import csv
import os
import numpy as np

# ------------------------------------------------------------
# Physics inputs
# ------------------------------------------------------------

BRANCHING_RATIOS = {
    121.78: 28.53 / 100.0,
    244.70: 7.55  / 100.0,
    344.28: 26.59 / 100.0,
    411.12: 2.237 / 100.0,
    443.96: 2.827 / 100.0,
    778.90: 12.93 / 100.0,
    867.37: 4.23  / 100.0,
    964.08: 14.51 / 100.0,
    1112.10: 13.67 / 100.0,
    1408.00: 20.87 / 100.0,
    1528.10: 0.279 / 100.0,
}

EXCLUDED_ENERGIES = {
    121.78, 244.70, 344.28, 411.12, 443.96, 1528.10
}

# Absolute scaling factor (example value)
SOURCE_ACTIVITY = 0.23136426
N_PRIMARIES = 2.0e9


# ------------------------------------------------------------
# I/O utilities
# ------------------------------------------------------------

def load_experimental_data(pattern="EXP_det*.txt"):
    data = {}
    for fname in sorted(glob.glob(pattern)):
        det = int(fname.split("det")[1].split(".")[0])
        data[det] = np.loadtxt(fname)
    return data


def load_simulation_data(pattern="it0_netcount_*cm_det*.txt"):
    data = {}
    for fname in sorted(glob.glob(pattern)):
        parts = fname.split("_")
        distance = float(parts[2].replace("cm", ""))
        det = int(parts[3].replace("det", "").replace(".txt", ""))
        data.setdefault(distance, {})[det] = np.loadtxt(fname)
    return data


# ------------------------------------------------------------
# Analysis utilities
# ------------------------------------------------------------

def chi_square(exp_data, sim_data, excluded):
    chi2 = 0.0
    for energy, exp_val in exp_data:
        if energy in excluded or exp_val <= 0:
            continue
        idx = np.argmin(np.abs(sim_data[:, 0] - energy))
        sim_val = sim_data[idx, 1]
        chi2 += (exp_val - sim_val) ** 2 / exp_val
    return chi2


def compute_normalization(exp_data, sim_data, ref_energy, ref_distance):
    ratios = []
    for det in exp_data:
        exp_counts = exp_data[det]
        sim_counts = sim_data[ref_distance][det]

        ie = np.argmin(np.abs(exp_counts[:, 0] - ref_energy))
        is_ = np.argmin(np.abs(sim_counts[:, 0] - ref_energy))

        if exp_counts[ie, 1] > 0:
            ratios.append(sim_counts[is_, 1] / exp_counts[ie, 1])

    return np.nanmean(ratios)


def efficiency(counts, energy):
    br = BRANCHING_RATIOS.get(round(energy, 2))
    if br is None or br == 0:
        return None
    scale = 1.0 / (SOURCE_ACTIVITY * N_PRIMARIES * br)
    return counts * scale


# ------------------------------------------------------------
# Main workflow
# ------------------------------------------------------------

def main():

    exp_data = load_experimental_data()
    sim_data = load_simulation_data()

    # --- Normalize simulation ---
    ref_energy = 1408.0
    ref_distance = 17.5

    norm = compute_normalization(exp_data, sim_data, ref_energy, ref_distance)
    sim_norm = copy.deepcopy(sim_data)

    for d in sim_norm:
        for det in sim_norm[d]:
            sim_norm[d][det][:, 1] /= norm

    # --- Find optimal distance per detector ---
    results = {}

    for det in exp_data:
        chi2_vals = {}
        for d in sim_norm:
            chi2_vals[d] = chi_square(
                exp_data[det], sim_norm[d][det], EXCLUDED_ENERGIES
            )
        best_d = min(chi2_vals, key=chi2_vals.get)
        results[det] = (best_d, chi2_vals[best_d])

    # --- Write efficiency CSVs ---
    os.makedirs("efficiency_csv", exist_ok=True)

    for det, (best_d, chi2_val) in results.items():

        exp = exp_data[det]
        sim = sim_norm[best_d][det]

        outname = f"efficiency_csv/Efficiency_det{det}_dist{best_d:.2f}.csv"
        with open(outname, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([
                f"Detector {det}",
                f"Optimal distance = {best_d:.2f} cm",
                f"Chi2 = {chi2_val:.3f}"
            ])
            writer.writerow([
                "Energy [MeV]",
                "Efficiency EXP",
                "Efficiency SIM",
                "Delta [%]"
            ])

            for energy, exp_val in exp:
                if energy in EXCLUDED_ENERGIES:
                    continue

                idx = np.argmin(np.abs(sim[:, 0] - energy))
                sim_val = sim[idx, 1]

                exp_eff = efficiency(exp_val, energy)
                sim_eff = efficiency(sim_val, energy)

                if exp_eff is None:
                    continue

                delta = 100.0 * (sim_eff - exp_eff) / exp_eff
                writer.writerow([
                    f"{energy/1000.0:.3f}",
                    f"{exp_eff:.6e}",
                    f"{sim_eff:.6e}",
                    f"{delta:.3f}"
                ])

        print(f"✓ Wrote {outname}")


if __name__ == "__main__":
    main()


