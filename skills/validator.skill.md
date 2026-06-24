# Validator Skill

## Role
A Validator Node monitors, filters, and authenticates messages in the Zigbee mesh network. It acts as an intrusion detection/prevention system (IDS/IPS) to ensure only legitimate traffic consumes network resources.

## Responsibilities
- **Message Validation**: Verifies MAC, signatures, and session keys on incoming messages
- **Replay Protection**: Maintains a cache of recent sequence counters to detect replay attacks
- **Duplicate Filtering**: Detects and discards duplicate messages before they propagate
- **Reputation Management**: Tracks reputation scores for each node based on valid/invalid message ratio
- **Isolation**: Automatically isolates nodes that exceed failure thresholds
- **Security Logging**: Records security events with timestamps and signatures
- **Delegation**: End devices can delegate validation to their parent router to save energy
- **Batch Validation**: Groups messages sharing the same temporal key for efficient validation

## Modes of Operation

### Passive Mode
- Listens to all traffic and validates messages
- Generates security alerts without interfering with routing
- Suitable for initial deployment and network auditing

### Active Mode
- Discards invalid messages at the validation layer
- Notifies the sender of validation failures
- Only forwards validated messages to upper layers
- Can isolate malicious nodes by adding them to blacklists

## Commands
- `validator enable` - Enable validator on this node
- `validator disable` - Disable validator on this node
- `validator set-mode <passive|active>` - Set validator operation mode
- `validator set-threshold <value>` - Set reputation failure threshold
- `validator reputation [address]` - Show reputation table for all or specific node
- `validator stats` - Show validation statistics (valid/invalid rates)
- `validator blacklist` - Show current blacklist
- `validator whitelist add <address>` - Add node to whitelist
- `validator whitelist remove <address>` - Remove node from whitelist

## Validation Flow

```
Sender                    Validator                     Receiver
  |                          |                             |
  |-- Message (signed) ---->|                             |
  |                          |-- Check MAC ------------>   |
  |                          |-- Check Seq Counter ---->   |
  |                          |-- Check Duplicate ------>   |
  |                          |-- Check Reputation ---->    |
  |                          |                             |
  |                          | (If valid)                  |
  |                          |-- Forward (validated) ----> |
  |                          |                             |
  |                          | (If invalid)                |
  |                          |-- Discard -----------------|
  |                          |-- Increment failure count   |
  |                          |-- If threshold exceeded:    |
  |                          |   Notify Trust Center       |
  |<-- Validation Failed --- |   Add to blacklist          |
```

## Reputation Algorithm

```python
# Pseudocode
REPUTATION_MAX = 100
REPUTATION_MIN = 0
PENALTY_INVALID = 10
REWARD_VALID = 1
DECAY_PERIOD = 3600  # seconds
ISOLATION_THRESHOLD = 3  # consecutive failures in window

def update_reputation(node_address, message_valid):
    node = reputation_table[node_address]
    
    if message_valid:
        node.score = min(REPUTATION_MAX, node.score + REWARD_VALID)
        node.consecutive_failures = 0
    else:
        node.score = max(REPUTATION_MIN, node.score - PENALTY_INVALID)
        node.consecutive_failures += 1
        node.last_failure_time = now()
    
    # Check isolation
    if node.consecutive_failures >= ISOLATION_THRESHOLD:
        if node.score < REPUTATION_MAX / 2:
            isolate_node(node_address)
    
    # Decay old scores
    if now() - node.last_activity > DECAY_PERIOD:
        node.score = min(REPUTATION_MAX, node.score + 1)  # Gradual recovery
```

## Configuration

```ini
[validator]
enabled = true
mode = passive  # passive | active
cache_size = 1024
reputation_threshold = 30
consecutive_failures_limit = 3
batch_validation = true
delegation_enabled = true
decay_period_seconds = 3600
logging_enabled = true
alert_on_isolate = true

[validation_keys]
rotation_interval_hours = 24
key_length = 128  # AES-128
```

## Key Rotation Process

1. Trust Center generates new validation key
2. Key is distributed OTA encrypted with link key
3. Old key remains valid for GRACE_PERIOD (5 min) for in-flight messages
4. Validators switch to new key after grace period
5. Trust Center confirms rotation completion via heartbeat

## Topology Position

```
                    +---------------+
                    |   Coordinator |
                    | (Trust Center)|
                    +-------+-------+
                            |
              +-------------+-------------+
              |             |             |
        +-----+-----+ +-----+-----+ +-----+-----+
        |  Router 1  | | Validator | |  Router 3  |
        | (Validator)| | (Dedicated)| |           |
        +-----+-----+ +-----+-----+ +-----+-----+
              |             |             |
        +-----+-----+       |       +-----+-----+
        | End Device |       |       | End Device |
        | (Delegates)|       |       |           |
        +-----------+       |       +-----------+
                             |
                       +-----+-----+
                       | End Device |
                       | (Delegates)|
                       +-----------+
```

## Energy Saving Strategies

- **Delegation**: End devices delegate validation to their parent router
- **Batch Validation**: Group messages with same temporal key into a single validation operation
- **Adaptive Policies**: Strict validation during peak hours, lightweight during off-peak
- **Retransmission Reduction**: Early discard of invalid traffic reduces total airtime
