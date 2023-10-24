#pragma once

#include <iostream>

class CLoggingInterface {
public:
    virtual void logDebug(const std::string& message) = 0;
    virtual void logInfo(const std::string& message) = 0;
    virtual void logWarning(const std::string& message) = 0;
    virtual void logError(const std::string& message, const std::string& errPrefix) = 0;
    virtual ~CLoggingInterface() {}
};