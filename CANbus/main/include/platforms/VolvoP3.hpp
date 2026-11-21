#pragma once
#include "CanTypes.hpp"

// ============================================================================
// PLATFORM CONFIGURATION: Volvo P3 (V70 / XC70 / S80 / XC60)
// Pure constexpr data in Flash. Edit ONLY this file to adapt to another car.
// ============================================================================
namespace PlatformConfig {
    inline constexpr const char* kName    = "Volvo P3";
    inline constexpr bool        k125Kbps = true;  // true = 125 kbps MS-CAN (Pins 3 & 11)
                                                   // false = 500 kbps HS-CAN (Pins 6 & 14)

    // Placeholder rules for future rule evaluation (0x000 = ignored until mapped)
    inline constexpr CanSignalDef kSignals[] = {
        { VehicleSignal::IGNITION_ON,  0x000, 0, 0x01, 0x01 },
        { VehicleSignal::HIGH_BEAM,    0x000, 2, 0x04, 0x04 },
        { VehicleSignal::REVERSE_GEAR, 0x000, 1, 0x10, 0x10 },
    };

    inline constexpr size_t kSignalCount = sizeof(kSignals) / sizeof(kSignals[0]);
}