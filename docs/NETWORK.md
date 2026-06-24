# Network Protocol

## IEEE 802.15.4 Frame Structure

```
┌──────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ Frame Control│ Sequence │ Dest PAN │ Dest Addr│ Src Addr │ Payload  │
│   (2 bytes)  │ (1 byte) │ (2 bytes)│ (2-8 B)  │ (2-8 B)  │ (var)    │
└──────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

## NWK Frame Structure

```
┌──────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ Frame Control│  Dest    │  Source  │  Radius  │ Sequence │ Payload  │
│   (2 bytes)  │ (2 bytes)│ (2 bytes)│ (1 byte) │ (1 byte) │ (var)    │
└──────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

## APS Frame Structure

```
┌──────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ Frame Control│ Endpoint │ Cluster  │ Profile  │ Counter  │ Payload  │
│   (1 byte)   │ (1 byte) │ (2 bytes)│ (2 bytes)│ (1 byte) │ (var)    │
└──────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
```

## Network Formation

```
1. Coordinator selects channel and PAN ID
2. Coordinator starts network (PAN ID + channel)
3. Coordinator opens network for joining
4. Routers scan and join network
5. End devices join through routers
6. Network stabilizes
```

## Routing Protocol

### Route Discovery
```
Source                    Destination
  |                          |
  |-- Route Request -------->|
  |   (broadcast)            |
  |                    Forward Route Request
  |                          |
  |<-- Route Reply ----------|
  |   (unicast via source    |
  |    route)                |
```

### Route Maintenance
- Routes age out after timeout
- Broken routes trigger rediscovery
- Alternative routes used during repair
- RREP sent for alternate paths found

## Neighbor Table

| Field | Size | Description |
|-------|------|-------------|
| Extended Address | 8B | IEEE address |
| Short Address | 2B | Network address |
| LQI | 1B | Link quality (0-255) |
| RSSI | 1B | Signal strength (dBm) |
| Depth | 1B | Network depth |
| Age | 1B | Time since last activity |

## Device Types

### Coordinator
- Forms the network
- Manages security
- Assigns addresses
- Maintains binding table

### Router
- Extends network range
- Forwards packets
- Accepts end devices
- Maintains routes

### End Device
- Leaf node only
- Communicates with parent
- Low power operation
- No routing capability
