#include "SignalTriggerConsumer.hpp"
#include "esp_timer.h"
#include <cstdio>

SignalTriggerConsumer::SignalTriggerConsumer(EcuStateMachine& fsm,
                                             const CanSignalDef* rules,
                                             size_t ruleCount)
    : fsm_(fsm),
      rules_(rules),
      ruleCount_(ruleCount <= kMaxRules ? ruleCount : kMaxRules) {}

void SignalTriggerConsumer::onFrameReceived(const CanFrame& frame) {
    for (size_t i = 0; i < ruleCount_; ++i) {
        const auto& rule = rules_[i];

        // Skip unmapped placeholder rules (0x000) or frames shorter than byteIndex
        if (rule.canId == 0x000 || frame.id != rule.canId || frame.dlc <= rule.byteIndex) {
            continue;
        }

        const bool isActive = ((frame.data[rule.byteIndex] & rule.bitMask) == rule.matchValue);

        // Edge detection: post to FSM queue only on first read or state flip
        if (!initialized_[i] || lastState_[i] != isActive) {
            initialized_[i] = true;
            lastState_[i]   = isActive;

            EventId evtId = EventId::EVT_SIGNAL_UPDATED;
            if (rule.signal == VehicleSignal::IGNITION_ON) {
                evtId = isActive ? EventId::EVT_IGNITION_ON : EventId::EVT_IGNITION_OFF;
            }

            fsm_.postEvent({ evtId, rule.signal, isActive, frame.timestampMs });
        }
    }
}

void SignalTriggerConsumer::onBusStateChanged(bool healthy) {
    const uint32_t nowMs = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);

    if (!healthy) {
        for (size_t i = 0; i < ruleCount_; ++i) {
            initialized_[i] = false;
        }
        fsm_.postEvent({ EventId::EVT_CAN_BUS_OFF, VehicleSignal::IGNITION_ON, false, nowMs });
    } else {
        fsm_.postEvent({ EventId::EVT_CAN_HEALTHY, VehicleSignal::IGNITION_ON, false, nowMs });
    }
}