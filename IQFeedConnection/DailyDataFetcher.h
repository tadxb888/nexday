#ifndef DAILY_DATA_FETCHER_H
#define DAILY_DATA_FETCHER_H

#include "HistoricalDataFetcher.h"

class DailyDataFetcher : public HistoricalDataFetcher {
protected:
    std::string get_interval_code() const override;
    std::chrono::seconds get_interval_offset() const override;

public:
    DailyDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr);
};

#endif // DAILY_DATA_FETCHER_H