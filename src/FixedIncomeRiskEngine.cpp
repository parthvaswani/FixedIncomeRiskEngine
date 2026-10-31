#include "FixedIncomeRiskEngine.hpp"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <utility>

// ----------------------------------------------------------------------------
// BondInstrument Implementation
// ----------------------------------------------------------------------------

BondInstrument::BondInstrument(std::string name, std::vector<CashFlow> cashFlows, InstrumentType type)
    : name_(std::move(name)), cashFlows_(std::move(cashFlows)), type_(type) {}

std::string BondInstrument::getName() const { return name_; }
std::vector<CashFlow> BondInstrument::getCashFlows() const { return cashFlows_; }

double BondInstrument::price(double yield) const {
    double pv = 0.0;
    for (const auto& cf : cashFlows_) {
        pv += cf.amount / std::pow(1.0 + yield, cf.time);
    }
    return pv;
}

double BondInstrument::modifiedDuration(double yield) const {
    double p = price(yield);
    if (p == 0.0) return 0.0;
    
    double weightedSum = 0.0;
    for (const auto& cf : cashFlows_) {
        weightedSum += (cf.time * cf.amount) / std::pow(1.0 + yield, cf.time + 1.0);
    }
    return weightedSum / p;
}

double BondInstrument::rho(double yield) const {
    double dP_dy = 0.0;
    for (const auto& cf : cashFlows_) {
        dP_dy -= (cf.time * cf.amount) / std::pow(1.0 + yield, cf.time + 1.0);
    }
    return dP_dy;
}

double BondInstrument::convexity(double yield) const {
    double p = price(yield);
    if (p == 0.0) return 0.0;

    double sum = 0.0;
    for (const auto& cf : cashFlows_) {
        sum += (cf.time * (cf.time + 1.0) * cf.amount) / std::pow(1.0 + yield, cf.time);
    }
    return sum / (p * std::pow(1.0 + yield, 2.0));
}

// ----------------------------------------------------------------------------
// RateShockEngine Implementation
// ----------------------------------------------------------------------------

std::vector<ShockResult> RateShockEngine::evaluateShocks(
    const std::vector<std::shared_ptr<IFixedIncomeInstrument>>& portfolio, 
    double baseYield, 
    const std::vector<double>& shocksBps) 
{
    double basePrice = 0.0;
    double weightedDuration = 0.0;
    double weightedConvexity = 0.0;

    for (const auto& inst : portfolio) {
        double p = inst->price(baseYield);
        basePrice += p;
        weightedDuration += inst->modifiedDuration(baseYield) * p;
        weightedConvexity += inst->convexity(baseYield) * p;
    }

    weightedDuration /= basePrice;
    weightedConvexity /= basePrice;

    std::vector<ShockResult> results;
    for (double bps : shocksBps) {
        double dy = bps / 10000.0;
        
        double shockedPrice = 0.0;
        for (const auto& inst : portfolio) {
            shockedPrice += inst->price(baseYield + dy);
        }

        double deltaP_actual = shockedPrice - basePrice;
        double actualDeltaPct = (deltaP_actual / basePrice) * 100.0;

        double linearDeltaPct = (-weightedDuration * dy) * 100.0;
        double linearEstimate = basePrice * (1.0 - weightedDuration * dy);

        double taylorDeltaPct = (-weightedDuration * dy + 0.5 * weightedConvexity * std::pow(dy, 2.0)) * 100.0;
        double taylorEstimate = basePrice * (1.0 - weightedDuration * dy + 0.5 * weightedConvexity * std::pow(dy, 2.0));

        results.push_back({
            bps, shockedPrice, linearEstimate, taylorEstimate, 
            actualDeltaPct, linearDeltaPct, taylorDeltaPct
        });
    }

    return results;
}

void RateShockEngine::printComparisonTable(const std::vector<ShockResult>& results) {
    std::cout << "\n=========================================================================================================\n";
    std::cout << "                        NON-LINEAR RATE SHOCK & TAYLOR APPROXIMATION COMPARISON                            \n";
    std::cout << "=========================================================================================================\n";
    std::cout << std::setw(10) << "Shock (bps)" 
              << std::setw(16) << "Actual PV ($)" 
              << std::setw(18) << "Linear Est ($)" 
              << std::setw(18) << "Taylor Est ($)" 
              << std::setw(14) << "Actual (%)" 
              << std::setw(14) << "Linear (%)" 
              << std::setw(14) << "Taylor (%)" << "\n";
    std::cout << "---------------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::cout << std::setw(10) << std::showpos << r.shockBps << std::noshowpos
                  << std::setw(16) << std::fixed << std::setprecision(2) << r.actualPV
                  << std::setw(18) << r.linearEstimate
                  << std::setw(18) << r.taylorEstimate
                  << std::setw(13) << std::showpos << r.actualDeltaPct << "%"
                  << std::setw(13) << r.linearDeltaPct << "%"
                  << std::setw(13) << r.taylorDeltaPct << "%"
                  << std::noshowpos << "\n";
    }
    std::cout << "=========================================================================================================\n";
}

// ----------------------------------------------------------------------------
// AssetLiabilityDecompositionEngine Implementation
// ----------------------------------------------------------------------------

AssetLiabilityDecompositionEngine::ALMReport AssetLiabilityDecompositionEngine::analyze(
    const std::vector<std::shared_ptr<IFixedIncomeInstrument>>& benchmarkLiabilities,
    const std::vector<std::shared_ptr<IFixedIncomeInstrument>>& assetOverlay,
    double baseYield) 
{
    double liaPV = 0.0;
    double liaDurationWeighted = 0.0;
    double liaRho = 0.0;

    for (const auto& lia : benchmarkLiabilities) {
        double p = lia->price(baseYield);
        liaPV += p;
        liaDurationWeighted += lia->modifiedDuration(baseYield) * p;
        liaRho += lia->rho(baseYield);
    }
    double liaDuration = liaDurationWeighted / liaPV;

    double assetPV = 0.0;
    double assetDurationWeighted = 0.0;
    double assetRho = 0.0;

    for (const auto& asset : assetOverlay) {
        double p = asset->price(baseYield);
        assetPV += p;
        assetDurationWeighted += asset->modifiedDuration(baseYield) * p;
        assetRho += asset->rho(baseYield);
    }
    double assetDuration = assetDurationWeighted / assetPV;

    double netPV = assetPV - liaPV;
    double netRho = assetRho - liaRho;
    double netDuration = (assetDuration * assetPV - liaDuration * liaPV) / (assetPV != 0 ? assetPV : 1.0);

    return {liaPV, liaDuration, assetPV, assetDuration, netPV, netDuration, netRho};
}

void AssetLiabilityDecompositionEngine::printALMReport(const ALMReport& report) {
    std::cout << "\n=====================================================================\n";
    std::cout << "                ASSET-LIABILITY DECOMPOSITION REPORT                 \n";
    std::cout << "=====================================================================\n";
    std::cout << std::left << std::setw(35) << "Metric" << std::right << std::setw(25) << "Value" << "\n";
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(35) << "Benchmark Liability PV ($):" << std::right << std::setw(25) << std::fixed << std::setprecision(2) << report.benchmarkLiaPV << "\n";
    std::cout << std::left << std::setw(35) << "Benchmark Liability Duration:" << std::right << std::setw(25) << std::setprecision(4) << report.benchmarkLiaDuration << " yrs\n";
    std::cout << std::left << std::setw(35) << "Asset Overlay PV ($):" << std::right << std::setw(25) << std::setprecision(2) << report.assetOverlayPV << "\n";
    std::cout << std::left << std::setw(35) << "Asset Overlay Duration:" << std::right << std::setw(25) << std::setprecision(4) << report.assetOverlayDuration << " yrs\n";
    std::cout << "---------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(35) << "Net Portfolio Surplus/Deficit ($):" << std::right << std::setw(25) << std::showpos << report.netRiskExposurePV << std::noshowpos << "\n";
    std::cout << std::left << std::setw(35) << "Net Duration Gap:" << std::right << std::setw(25) << std::showpos << report.netDurationGap << std::noshowpos << " yrs\n";
    std::cout << std::left << std::setw(35) << "Net Rho (dP/dy):" << std::right << std::setw(25) << std::showpos << report.netRho << std::noshowpos << "\n";
    std::cout << "=====================================================================\n";
}