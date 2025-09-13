#ifndef FIFTEEN_MIN_DATA_FETCHER_H
#define FIFTEEN_MIN_DATA_FETCHER_H

#include "HistoricalDataFetcher.h"

class FifteenMinDataFetcher : public HistoricalDataFetcher {
protected:
    std::string get_interval_code() const override;
    std::chrono::seconds get_interval_offset() const override;

public:
    FifteenMinDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr);
};

#endif // FIFTEEN_MIN_DATA_FETCHER_H