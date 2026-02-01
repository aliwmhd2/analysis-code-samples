/**
 *  root_gamma_yield_analysis.cpp
 *
 *  Example ROOT-based analysis code to extract gamma-ray yields
 *  from time-of-flight (TOF) spectra using side-band background subtraction.
 *
 *  This example is adapted from PhD analysis work on HPGe detectors
 *  and neutron-induced gamma-ray production in 28Si
 *  (1778.969 keV transition).
 *
 *  The code is intentionally simplified and uses non-sensitive inputs.
 *
 *  Author: Ali F. Alwars
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TAxis.h"
#include "TMath.h"

// ------------------------------------------------------------
// Physical constants
// ------------------------------------------------------------
namespace constants {
    constexpr double c_light      = 0.299792458;   // m/ns
    constexpr double neutron_mass = 1.674927471e-27; // kg
    constexpr double joule_to_MeV = 1.0 / 1.602176634e-13;
    constexpr double flight_path  = 99.6755;        // m
}

// ------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------

/**
 * Convert neutron TOF (ns) to kinetic energy (MeV)
 * using relativistic kinematics.
 */
double tof_ns_to_energy_MeV(double tof_ns)
{
    const double time_s = tof_ns * 1e-9;
    const double velocity = constants::flight_path / time_s;
    const double beta = velocity / 299792458.0;

    const double gamma =
        1.0 / std::sqrt(1.0 - beta * beta);

    const double energy_J =
        constants::neutron_mass *
        std::pow(299792458.0, 2) * (gamma - 1.0);

    return energy_J * constants::joule_to_MeV;
}

// ------------------------------------------------------------
// Yield extraction
// ------------------------------------------------------------

struct YieldResult {
    std::vector<double> yield;
    std::vector<double> error;
};

/**
 * Extract net gamma-ray yield per TOF bin using
 * left/right side-band background subtraction.
 */
YieldResult extract_yield(
    const TH2F* h_time_energy,
    int peak_min, int peak_max,
    int bkgL_min, int bkgL_max,
    int bkgR_min, int bkgR_max
)
{
    const int nTOF = h_time_energy->GetNbinsX();

    YieldResult result;
    result.yield.resize(nTOF, 0.0);
    result.error.resize(nTOF, 0.0);

    const double peak_width = peak_max - peak_min;
    const double bkgL_width = bkgL_max - bkgL_min;
    const double bkgR_width = bkgR_max - bkgR_min;

    for (int ib = 1; ib <= nTOF; ++ib) {

        const double gross =
            h_time_energy->Integral(ib, ib, peak_min, peak_max);

        const double bkgL =
            h_time_energy->Integral(ib, ib, bkgL_min, bkgL_max);

        const double bkgR =
            h_time_energy->Integral(ib, ib, bkgR_min, bkgR_max);

        const double net =
            gross
            - 0.5 * (peak_width / bkgL_width) * bkgL
            - 0.5 * (peak_width / bkgR_width) * bkgR;

        const double err =
            std::sqrt(
                gross +
                std::pow(0.5 * peak_width / bkgL_width, 2) * bkgL +
                std::pow(0.5 * peak_width / bkgR_width, 2) * bkgR
            );

        result.yield[ib - 1] = net;
        result.error[ib - 1] = err;
    }

    return result;
}

// ------------------------------------------------------------
// Main analysis example
// ------------------------------------------------------------

int main()
{
    // Open example ROOT file (dummy or simplified input)
    TFile input("example_time_energy.root", "READ");
    if (input.IsZombie()) {
        std::cerr << "Error: cannot open input file\n";
        return 1;
    }

    TH2F* h_time_energy =
        dynamic_cast<TH2F*>(input.Get("h_time_energy"));

    if (!h_time_energy) {
        std::cerr << "Histogram not found\n";
        return 1;
    }

    // Energy window definition (example values)
    const int peak_min = 300;
    const int peak_max = 360;
    const int bkgL_min = 240;
    const int bkgL_max = 280;
    const int bkgR_min = 380;
    const int bkgR_max = 420;

    YieldResult yield =
        extract_yield(
            h_time_energy,
            peak_min, peak_max,
            bkgL_min, bkgL_max,
            bkgR_min, bkgR_max
        );

    // Create TOF histogram
    const int nTOF = h_time_energy->GetNbinsX();
    TH1F h_yield_tof(
        "h_yield_tof",
        "Net #gamma yield vs TOF;TOF [ns];Counts",
        nTOF,
        h_time_energy->GetXaxis()->GetXmin(),
        h_time_energy->GetXaxis()->GetXmax()
    );

    for (int i = 0; i < nTOF; ++i) {
        h_yield_tof.SetBinContent(i + 1, yield.yield[i]);
        h_yield_tof.SetBinError(i + 1, yield.error[i]);
    }

    // Write output
    TFile output("gamma_yield_output.root", "RECREATE");
    h_yield_tof.Write();
    output.Close();

    std::cout << "Yield extraction finished successfully.\n";
    return 0;
}
