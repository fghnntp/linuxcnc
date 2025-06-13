#ifndef _SERVO_STRATEGY_FACTORY_H_
#define _SERVO_STRATEGY_FACTORY_H_

#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "IServoStrategy.h"

#ifdef _WIN32
    #define EXPORT_LIBRARY_API __declspec(dllexport)
#else
    #define EXPORT_LIBRARY_API
#endif

class EXPORT_LIBRARY_API ServoStrategyFactory {
private:

public:
    ServoStrategyFactory() = default;
    ~ServoStrategyFactory() = default;
    ServoStrategyFactory& operator=(const ServoStrategyFactory&) = delete;
    ServoStrategyFactory(const ServoStrategyFactory&) = delete;
    IServoStrategy* CreateStrategy(std::string servoId);
    std::string GetVersion();
};
#endif // !_SERVO_STRATEGY_FACTORY_H_