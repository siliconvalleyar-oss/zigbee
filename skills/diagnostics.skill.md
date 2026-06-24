# Diagnostics Skill

## Role
Diagnostics monitors network health, performance, and troubleshoots issues.

## Responsibilities
- **Network Statistics**: Tracks packets sent/received/failed
- **Link Quality**: Monitors LQI and RSSI values
- **Route Analysis**: Analyzes routing table health
- **Node Health**: Checks device reachability and responsiveness
- **Performance Metrics**: Calculates packet delivery ratio (PDR)
- **Troubleshooting**: Identifies network issues and suggests fixes

## Commands
- `mesh diagnostics` - Show comprehensive diagnostics
- `mesh topology` - Display network topology with metrics
- `mesh routes` - Show routing table with health info
- `device info <addr>` - Detailed device diagnostics

## Metrics Tracked
- **PDR (Packet Delivery Ratio)**: Percentage of successful packets
- **LQI (Link Quality Indicator)**: 0-255 signal quality metric
- **RSSI (Received Signal Strength)**: Signal strength in dBm
- **Hop Count**: Number of hops to reach destination
- **Route Age**: Time since route was last updated
- **Uptime**: Network operational time

## Diagnostic Report
```
=== Zigbee Mesh Diagnostics ===
Network: PAN=0x1234, CH=11, Role=Coordinator
Uptime: 2h 30m 15s
Devices: 5 (2 routers, 3 end devices)
Routes: 8 active
PDR: 98.5%
Avg LQI: 185
Avg RSSI: -45 dBm

Top Issues:
  - Node 0x0003: High error rate (15%)
  - Route to 0x0007: High hop count (6)
```
