#include "drivers/hal.h"
#include "drivers/mrf24j40.h"
#include "drivers/cc2530.h"
#include "drivers/xbee.h"

namespace zigbee_mesh::drivers {

RadioDriverPtr createRadioDriver(RadioType type) {
    switch (type) {
        case RadioType::MRF24J40:
            return std::make_unique<MRF24J40Driver>();
        case RadioType::CC2530:
        case RadioType::CC2531:
            return std::make_unique<CC2530Driver>();
        case RadioType::XBee:
            return std::make_unique<XBeeDriver>();
        default:
            return nullptr;
    }
}

} // namespace zigbee_mesh::drivers
