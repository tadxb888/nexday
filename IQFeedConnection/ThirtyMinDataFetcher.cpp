#include "ThirtyMinDataFetcher.h"

ThirtyMinDataFetcher::ThirtyMinDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr)
    : HistoricalDataFetcher(conn_mgr, "30Min") {
}

std::string ThirtyMinDataFetcher::get_interval_code() const {
    return "1800"; // 1800 seconds = 30 minutes
}

std::chrono::seconds ThirtyMinDataFetcher::get_interval_offset() const {
    return std::chrono::seconds(1800);  // 30 minutes
}