#include "OneHourDataFetcher.h"

OneHourDataFetcher::OneHourDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr)
    : HistoricalDataFetcher(conn_mgr, "1Hour") {
}

std::string OneHourDataFetcher::get_interval_code() const {
    return "3600"; // 3600 seconds = 1 hour
}

std::chrono::seconds OneHourDataFetcher::get_interval_offset() const {
    return std::chrono::seconds(3600);  // 1 hour
}