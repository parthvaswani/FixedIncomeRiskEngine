#ifndef MARKET_DATA_PROVIDER_HPP
#define MARKET_DATA_PROVIDER_HPP

#include <vector>
#include <string>
#include "FixedIncomeRiskEngine.hpp"

// Abstract Market Data Provider Interface
class IMarketDataProvider {
public:
    virtual ~IMarketDataProvider() = default;
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual double fetchCurrentYield(const std::string& ticker) = 0;
    virtual std::vector<CashFlow> fetchInstrumentCashFlows(const std::string& ticker) = 0;
};

// IBKR TWS Socket Client Adapter
// Modeled after IBKR C++ API architecture (EClientSocket / EWrapper pattern)
class IBKRSocketClient : public IMarketDataProvider {
private:
    std::string host_;
    int port_;
    int clientId_;
    bool isConnected_;

public:
    IBKRSocketClient(std::string host, int port, int clientId);
    ~IBKRSocketClient() override;

    bool connect() override;
    void disconnect() override;
    
    // IBKR Socket specific routines
    bool isConnected() const;
    void requestMarketData(int reqId, const std::string& symbol);

    // Overrides from IMarketDataProvider
    double fetchCurrentYield(const std::string& ticker) override;
    std::vector<CashFlow> fetchInstrumentCashFlows(const std::string& ticker) override;
};

// Safe local fallback provider for testing without an active TWS gateway
class MockMarketDataProvider : public IMarketDataProvider {
public:
    bool connect() override { return true; }
    void disconnect() override {}
    double fetchCurrentYield(const std::string& ticker) override { return 0.045; } // 4.50%
    std::vector<CashFlow> fetchInstrumentCashFlows(const std::string& ticker) override {
        return {{1.0, 5.0}, {2.0, 105.0}};
    }
};

#endif // MARKET_DATA_PROVIDER_HPP