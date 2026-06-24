# End Device Skill

## Role
End Devices are leaf nodes in the Zigbee network. They are typically sensors or actuators with limited resources.

## Responsibilities
- **Data Collection**: Reads sensor data and sends to parent/router
- **Actuator Control**: Receives commands and controls actuators
- **Low Power**: Supports sleep modes for battery-powered operation
- **Minimal Routing**: Does not relay packets for other devices
- **Parent Communication**: Communicates only with its parent device

## Commands
- `network join <pan_id> <channel>` - Join an existing network
- `network leave` - Leave the network
- `device info` - Show local device information

## Configuration
```ini
[device]
role = 2  # End Device

[network]
pan_id = 0x1234
channel = 11
```

## Topology Position
```
   +-----------+
   | Coordinator|
   +-----+-----+
         |
    +----+----+
    | Router  |
    +----+----+
         |
    +----+----+
    |EndDevice|  <-- THIS DEVICE
    +---------+
```

## Sleep Mode Support
End devices can enter low-power sleep mode:
- Wake on timer
- Wake on GPIO interrupt
- Wake on radio activity
- Poll parent for pending data
