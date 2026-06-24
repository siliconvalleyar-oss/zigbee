# Router Skill

## Role
A Router extends the network range by relaying messages between devices. It cannot form a network but can join as a router.

## Responsibilities
- **Packet Relay**: Forwards packets between devices to extend network range
- **Route Maintenance**: Maintains routing tables for efficient packet delivery
- **Child Management**: Accepts end devices as children
- **Network Discovery**: Responds to network scans and device announcements
- **Heartbeat**: Periodically announces its presence to the network

## Commands
- `network join <pan_id> <channel>` - Join an existing network as router
- `network leave` - Leave the network
- `device list` - List connected devices
- `mesh topology` - View current topology
- `mesh routes` - View routing table

## Configuration
```ini
[device]
role = 1  # Router

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
    | Router  |  <-- THIS DEVICE
    +----+----+
    |         |
+---+---+ +---+---+
|EndDev | |EndDev |
+-------+ +-------+
```
