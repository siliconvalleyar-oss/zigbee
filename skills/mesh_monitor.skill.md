# Mesh Monitor Skill

## Role
Real-time monitoring and visualization of the Zigbee mesh network topology and traffic.

## Responsibilities
- **Topology Discovery**: Maps the complete network topology
- **Link Monitoring**: Tracks link quality and stability
- **Traffic Analysis**: Monitors packet flow and routing patterns
- **Visualization**: Generates ASCII or graphical topology views
- **Alerts**: Notifies on topology changes or node failures
- **History**: Records topology changes over time

## Commands
- `mesh topology` - Show current topology
- `mesh monitor` - Start real-time monitoring
- `mesh history` - Show topology change history
- `mesh scan` - Perform active network scan

## ASCII Topology View
```
=== Zigbee Mesh Topology ===

Coordinator (0x0000) [CH=11 PAN=0x1234]
├── Router 0x0001 [LQI=200 RSSI=-32dBm]
│   ├── EndDevice 0x0004 [LQI=180 RSSI=-45dBm]
│   └── EndDevice 0x0005 [LQI=165 RSSI=-52dBm]
├── Router 0x0002 [LQI=195 RSSI=-35dBm]
│   └── EndDevice 0x0006 [LQI=175 RSSI=-48dBm]
└── Router 0x0003 [LQI=190 RSSI=-38dBm]
    ├── EndDevice 0x0007 [LQI=185 RSSI=-42dBm]
    └── EndDevice 0x0008 [LQI=170 RSSI=-50dBm]

Links: 8 | Nodes: 9 | Avg Depth: 2.0
```

## Monitoring Events
- Node join/leave
- Link quality changes
- Route changes
- Network partition detection
- Orphaned node detection
