#pragma once
#include "driver/gpio.h"
#include <cstdint>

enum class RelayChannel : uint8_t {
    RELAY_1 = 0, // e.g., Auxiliary High Beam Bar
    RELAY_2 = 1, // e.g., Auxiliary Reverse / Work Light
    COUNT
};

class RelayDriver {
public:
    RelayDriver(gpio_num_t relay1Pin, gpio_num_t relay2Pin, bool activeHigh = true);

    void init();
    void setChannel(RelayChannel channel, bool on);
    void setAllOff();
    bool isChannelOn(RelayChannel channel) const;

private:
    static constexpr size_t kChannelCount = static_cast<size_t>(RelayChannel::COUNT);

    gpio_num_t pins_[kChannelCount];
    bool       states_[kChannelCount] = {};
    bool       activeHigh_;
    bool       initialized_ = false;
};