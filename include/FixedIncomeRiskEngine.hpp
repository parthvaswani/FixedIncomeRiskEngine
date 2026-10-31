#ifndef FIXED_INCOME_RISK_ENGINE_HPP
#define FIXED_INCOME_RISK_ENGINE_HPP

#include <vector>
#include <string>
#include <memory>

struct CashFlow {
    double time; // in years
    double amount;
};

enum class InstrumentType {
    ZeroCouponBond,
    BulletBond,
    CorporateLiability
};

class IFixedIncomeInstrument {
public:
    virtual ~IFixedIncomeInstrument() = default;
    virtual double price(double yield) const = 0;
    virtual double modifiedDuration(double yield) const = 0;
    virtual double convexity(double yield) const = 0;
    virtual double rho(double yield) const = 0;
    virtual std::string getName() const = 0;
    virtual std::vector<CashFlow> getCashFlows() const = 0;
};

class BondInstrument : public IFixedIncomeInstrument {
private:
    std::string name_;
    std::vector<CashFlow> cashFlows_;
    InstrumentType type_;

public:
    BondInstrument(std::string name, std::vector<CashFlow> cashFlows, InstrumentType type);

    std::string getName() const override;
    std::vector<CashFlow> getCashFlows() const override;

    double price(double yield) const override;
    double modifiedDuration(double yield) const override;
    double rho(double yield) const override;
    double convexity(double yield) const override;
};

struct ShockResult {
    double shockBps;
    double actualPV;
    double linearEstimate;
    double taylorEstimate;
    double actualDeltaPct;
    double linearDeltaPct;
    double taylorDeltaPct;
};

class RateShockEngine {
public:
    static std::vector<ShockResult> evaluateShocks(
        const std::vector<std::shared_ptr<IFixedIncomeInstrument>>& portfolio, 
        double baseYield, 
        const std::vector<double>& shocksBps);

    static void printComparisonTable(const std::vector<ShockResult>& results);
};

class AssetLiabilityDecompositionEngine {
public:
    struct ALMReport {
        double benchmarkLiaPV;
        double benchmarkLiaDuration;
        double assetOverlayPV;
        double assetOverlayDuration;
        double netRiskExposurePV;
        double netDurationGap;
        double netRho;
    };

    static ALMReport analyze(
        const std::vector<std::shared_ptr<IFixedIncomeInstrument>>& benchmarkLiabilities,
        const std::vector<std::shared_ptr<IFixedIncomeInstrument>>& assetOverlay,
        double baseYield);

    static void printALMReport(const ALMReport& report);
};

#endif // FIXED_INCOME_RISK_ENGINE_HPP