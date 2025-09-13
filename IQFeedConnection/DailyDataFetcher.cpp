#include "DailyDataFetcher.h"

DailyDataFetcher::DailyDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr)
    : HistoricalDataFetcher(conn_mgr, "Daily") {
}

std::string DailyDataFetcher::get_interval_code() const {
    return "DAILY";
}

std::chrono::seconds DailyDataFetcher::get_interval_offset() const {
    return std::chrono::seconds(0);  // Not used for daily data
}