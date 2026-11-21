#pragma once
#include "CanTypes.hpp"
#include "EcuStateMachine.hpp"
#include <cstddef>

class SignalTriggerConsumer {
public:
    SignalTriggerConsumer(EcuStateMachine& fsm,
                          const CanSignalDef* rules,
                          size_t ruleCount);

    void onFrameReceived(const CanFrame& frame);
    void onBusStateChanged(bool healthy);

private:
    static constexpr size_t kMaxRules = 16;

    EcuStateMachine&    fsm_;
    const CanSignalDef* rules_;
    size_t              ruleCount_;

    bool initialized_[kMaxRules] = {};
    bool lastState_[kMaxRules]   = {};
};