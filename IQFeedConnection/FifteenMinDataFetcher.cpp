#include "FifteenMinDataFetcher.h"

FifteenMinDataFetcher::FifteenMinDataFetcher(std::shared_ptr<IQFeedConnectionManager> conn_mgr)
    : HistoricalDataFetcher(conn_mgr, "15Min") {
}

std::string FifteenMinDataFetcher::get_interval_code() const {
    return "900"; // 900 seconds = 15 minutes
}

std::chrono::seconds FifteenMinDataFetcher::get_interval_offset() const {
    return std::chrono::seconds(900);  // 15 minutes
}