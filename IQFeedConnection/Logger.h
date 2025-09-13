#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>

class Logger {
private:
    std::ofstream log_file;
    bool logging_enabled;
    
    std::string get_timestamp() const;

public:
    Logger(const std::string& filename = "iqfeed.log", bool enabled = true);
    ~Logger();
    
    void log(const std::string& level, const std::string& message);
    void info(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);
    void success(const std::string& message);
};

#endif // LOGGER_H