#include "RelayDriver.hpp"
#include <cstdio>

RelayDriver::RelayDriver(gpio_num_t relay1Pin, gpio_num_t relay2Pin, bool activeHigh)
    : pins_{ relay1Pin, relay2Pin }, activeHigh_(activeHigh) {}

void RelayDriver::init() {
    uint64_t pinMask = 0;
    for (size_t i = 0; i < kChannelCount; ++i) {
        if (pins_[i] != GPIO_NUM_NC) {
            pinMask |= (1ULL << static_cast<uint32_t>(pins_[i]));
        }
    }

    if (pinMask != 0) {
        gpio_config_t io_conf{};
        io_conf.pin_bit_mask = pinMask;
        io_conf.mode         = GPIO_MODE_OUTPUT;
        io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io_conf);
    }

    initialized_ = true;
    setAllOff();
}

void RelayDriver::setChannel(RelayChannel channel, bool on) {
    const size_t idx = static_cast<size_t>(channel);
    if (idx >= kChannelCount) {
        return;
    }

    if (initialized_ && states_[idx] == on) {
        return;
    }

    states_[idx] = on;

    if (pins_[idx] != GPIO_NUM_NC) {
        const uint32_t level = (on == activeHigh_) ? 1U : 0U;
        gpio_set_level(pins_[idx], level);
    }

    std::printf("   [RELAY] Channel %u -> %s\n",
                static_cast<unsigned>(idx + 1),
                on ? "ON (ENERGIZED)" : "OFF");
}

void RelayDriver::setAllOff() {
    for (size_t i = 0; i < kChannelCount; ++i) {
        states_[i] = false;
        if (pins_[i] != GPIO_NUM_NC) {
            gpio_set_level(pins_[i], activeHigh_ ? 0U : 1U);
        }
    }
    std::printf("   [RELAY] ALL CHANNELS -> OFF (SAFE)\n");
}

bool RelayDriver::isChannelOn(RelayChannel channel) const {
    const size_t idx = static_cast<size_t>(channel);
    return (idx < kChannelCount) ? states_[idx] : false;
}