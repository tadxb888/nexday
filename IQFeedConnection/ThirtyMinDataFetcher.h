#ifndef THIRTY_MIN_DATA_FETCHER_H
#define THIRTY_MIN_DATA_FETCHER_H

#include "HistoricalDataFetcher.h"

class ThirtyMinDataFetcher : public HistoricalDataFetcher {
protected:
    std::string get_interval_code() const override;
    std::chrono::seconds get_interval_offset() const override;

public:
    ThirtyMinDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr);
};

#endif // THIRTY_MIN_DATA_FETCHER_H