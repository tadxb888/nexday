#ifndef ONE_HOUR_DATA_FETCHER_H
#define ONE_HOUR_DATA_FETCHER_H

#include "HistoricalDataFetcher.h"

class OneHourDataFetcher : public HistoricalDataFetcher {
protected:
    std::string get_interval_code() const override;
    std::chrono::seconds get_interval_offset() const override;

public:
    OneHourDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr);
};

#endif // ONE_HOUR_DATA_FETCHER_H