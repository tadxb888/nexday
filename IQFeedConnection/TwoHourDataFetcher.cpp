#include "TwoHourDataFetcher.h"

TwoHourDataFetcher::TwoHourDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr)
    : HistoricalDataFetcher(conn_mgr, "2Hour") {
}

std::string TwoHourDataFetcher::get_interval_code() const {
    return "7200"; // 7200 seconds = 2 hours (officially supported by IQFeed)
}

std::chrono::seconds TwoHourDataFetcher::get_interval_offset() const {
    return std::chrono::seconds(7200);  // 2 hours
}