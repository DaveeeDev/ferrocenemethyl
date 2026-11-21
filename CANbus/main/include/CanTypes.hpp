#pragma once
#include <cstddef>
#include <cstdint>

// Hardware independent CAN frame passed from CanEngine to the active consumer
struct CanFrame {
    uint32_t timestampMs;
    uint32_t id;
    uint8_t  dlc;
    bool     isExtended;
    bool     isRtr;
    uint8_t  data[8];
};

// Car agnostic logical signals used by the future rule evaluator & state machine
enum class VehicleSignal : uint8_t {
    IGNITION_ON,
    HIGH_BEAM,
    LOW_BEAM,
    REVERSE_GEAR,
    FOG_FRONT,
    FOG_REAR
};

// Schema for bit-unpacking rules defined in platform headers
struct CanSignalDef {
    VehicleSignal signal;
    uint32_t      canId;       // 0x000 = disabled / not yet mapped
    uint8_t       byteIndex;   // Payload byte [0..7]
    uint8_t       bitMask;     // Bits to inspect
    uint8_t       matchValue;  // Expected value after masking (supports 1-bit & multi-bit states)
};