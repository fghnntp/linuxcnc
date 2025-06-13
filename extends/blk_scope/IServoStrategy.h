#ifndef _ISERVO_STRATEGY_H_
#define _ISERVO_STRATEGY_H_
#include <iostream>
#include <memory>
#include <unordered_map>

class IServoStrategy {
public:
    virtual ~IServoStrategy() = default;
    virtual void Initialize() = 0;
    virtual void Step() = 0;
    virtual double GetAct() = 0;
    virtual void SetCmd(double& cmd) = 0;
};

#endif