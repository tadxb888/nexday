#ifndef TWO_HOUR_DATA_FETCHER_H
#define TWO_HOUR_DATA_FETCHER_H

#include "HistoricalDataFetcher.h"

class TwoHourDataFetcher : public HistoricalDataFetcher {
protected:
    std::string get_interval_code() const override;
    std::chrono::seconds get_interval_offset() const override;

public:
    TwoHourDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr);
};

#endif // TWO_HOUR_DATA_FETCHER_H