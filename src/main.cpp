#include "FixedIncomeRiskEngine.hpp"
#include "MarketDataProvider.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>

int main() {
    std::cout << "=====================================================================\n";
    std::cout << "        MULTI-ASSET FIXED-INCOME ENGINE + IBKR SOCKET ADAPTER        \n";
    std::cout << "=====================================================================\n\n";

    // Initialize IBKR Socket Client with configurable parameters (safe for public repos)
    // Production TWS paper trading port: 7497, Live port: 7496 (Gateway: 4002/4001)
    std::string ibkrHost = "127.0.0.1";
    int ibkrPort = 7497; 
    int clientId = 1;

    auto socketClient = std::make_unique<IBKRSocketClient>(ibkrHost, ibkrPort, clientId);

    // Connect to TWS Socket
    if (socketClient->connect()) {
        double liveYield = socketClient->fetchCurrentYield("US_TREASURY_5Y");
        std::cout << "-> Live Yield Streamed from IBKR Socket: " << std::fixed << std::setprecision(3) << (liveYield * 100.0) << "%\n\n";
    }

    // Fallback or baseline yield environment
    double baseYield = 0.045; 

    // Define Portfolio Instruments
    auto zeroBond = std::make_shared<BondInstrument>(
        "Zero-Coupon Bond (5Y)", 
        std::vector<CashFlow>{{5.0, 100.0}}, 
        InstrumentType::ZeroCouponBond
    );

    auto bulletBond = std::make_shared<BondInstrument>(
        "5Y Bullet Treasury (4% cpn)", 
        std::vector<CashFlow>{
            {1.0, 4.0}, {2.0, 4.0}, {3.0, 4.0}, {4.0, 4.0}, {5.0, 104.0}
        }, 
        InstrumentType::BulletBond
    );

    auto corpLiability = std::make_shared<BondInstrument>(
        "Corporate Liability / Bond (10Y 6% cpn)", 
        std::vector<CashFlow>{
            {1.0, 6.0}, {2.0, 6.0}, {3.0, 6.0}, {4.0, 6.0}, {5.0, 6.0},
            {6.0, 6.0}, {7.0, 6.0}, {8.0, 6.0}, {9.0, 6.0}, {10.0, 106.0}
        }, 
        InstrumentType::CorporateLiability
    );

    // 1. Core Portfolio Pricing & Sensitivity Display
    std::vector<std::shared_ptr<IFixedIncomeInstrument>> portfolio = {zeroBond, bulletBond, corpLiability};

    std::cout << "=====================================================================\n";
    std::cout << "              MODULE 1: CORE BOND & PORTFOLIO PRICING                \n";
    std::cout << "=====================================================================\n";
    for (const auto& inst : portfolio) {
        std::cout << "Instrument: " << inst->getName() << "\n";
        std::cout << "  - Baseline PV (y=4.5%): $" << std::fixed << std::setprecision(2) << inst->price(baseYield) << "\n";
        std::cout << "  - Modified Duration:    " << std::setprecision(4) << inst->modifiedDuration(baseYield) << " yrs\n";
        std::cout << "  - Convexity:            " << std::setprecision(4) << inst->convexity(baseYield) << "\n";
        std::cout << "  - Rho (dP/dy):          " << std::setprecision(2) << inst->rho(baseYield) << "\n\n";
    }

    // 2. Non-Linear Convexity & Rate-Shock Module
    std::vector<double> shocks = {-200.0, -100.0, -50.0, -25.0, 25.0, 50.0, 100.0, 200.0};
    auto shockResults = RateShockEngine::evaluateShocks(portfolio, baseYield, shocks);
    RateShockEngine::printComparisonTable(shockResults);

    // 3. Asset-Liability Decomposition Module (Benchmark Liability vs. Asset Overlay)
    std::vector<std::shared_ptr<IFixedIncomeInstrument>> benchmarkLiabilities = { corpLiability };
    std::vector<std::shared_ptr<IFixedIncomeInstrument>> assetOverlay = { zeroBond, bulletBond };

    auto almReport = AssetLiabilityDecompositionEngine::analyze(benchmarkLiabilities, assetOverlay, baseYield);
    AssetLiabilityDecompositionEngine::printALMReport(almReport);

    // Graceful socket disconnect
    socketClient->disconnect();

    return 0;
}