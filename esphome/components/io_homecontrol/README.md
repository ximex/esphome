# io-homecontrol Component

ESPHome component for controlling **Somfy io-homecontrol** devices (roller blinds,
awnings, venetian blinds, etc.) over 868 MHz radio.

> **Protocol reference:** See [PROTOCOL.md](PROTOCOL.md) for full technical details on
> frame structure, commands, cryptography, and radio configuration.

## Features

- **Open / Close / Stop** commands for motorized covers
- **Position control** (0–100%) for intermediate positions
- **My Position** (preset position) support
- **Passive sniffing** of commands from existing remotes
- **Automatic state tracking** from sniffed commands (keeps HA in sync when
  physical remotes are used)
- **Pairing sniffer** mode to capture encryption keys from existing remotes
- **Active pairing** to register as a new controller on motors
- **Sequence number persistence** across reboots (per source address, stored in flash)
- **CC1101 radio** with UART async serial mode for proper io-homecontrol framing

## Supported Radios

**CC1101** (TI CC1101-based modules) is the only supported radio.

io-homecontrol uses UART-style encoding where each byte is transmitted as 10 bits
(1 start bit + 8 data bits + 1 stop bit). The CC1101's async serial mode handles
this framing automatically. Other radios like SX126x/SX127x lack this capability.

## Configuration

### Hub

```yaml
io_homecontrol:
  cc1101_id: my_cc1101             # Required: CC1101 radio component ID
  gdo0_pin: 4                      # Required: GPIO number of CC1101 GDO0 pin

  source_address: 0xAABBCC         # 3-byte controller address (required unless pairing_mode)
  key: "0102030405060708090A0B0C0D0E0F10"  # 16-byte AES key as hex (required unless pairing_mode)
  tx_repeats: 4                    # Number of TX repetitions per command (1-10, default: 4)
  pairing_mode: false              # Enable pairing sniffer mode (default: false)
  # initial_sequence: 1000         # Override starting sequence number (optional)
```

| Option             | Required | Default | Description                                              |
|--------------------|:--------:|:-------:|----------------------------------------------------------|
| `cc1101_id`        | **Yes**  | —       | ID of the CC1101 radio component                         |
| `gdo0_pin`         | **Yes**  | —       | GPIO number of CC1101 GDO0 pin                           |
| `source_address`   | Yes*     | —       | 3-byte address (0x000000–0xFFFFFF). Controller identity. |
| `key`              | Yes*     | —       | 16-byte AES-128 key as 32-character hex string           |
| `tx_repeats`       | No       | `4`     | Number of times each frame is transmitted (1–10)         |
| `pairing_mode`     | No       | `false` | Enable pairing sniffer mode                              |
| `initial_sequence` | No       | —       | Force a starting sequence number (overrides flash value) |

*\* Required when `pairing_mode` is `false`.*

### Cover Platform

```yaml
cover:
  - platform: io_homecontrol
    io_homecontrol_id: iohc_hub   # Hub ID (optional if only one hub)
    name: "Living Room Blinds"
    address: 0x112233              # 3-byte target motor address
```

| Option              | Required | Description                     |
|---------------------|:--------:|---------------------------------|
| `io_homecontrol_id` | No*      | Hub component ID                |
| `name`              | Yes      | Cover entity name               |
| `address`           | Yes      | 3-byte target motor address     |

*\* Required when multiple io_homecontrol hubs exist.*

Cover traits:
- **Position**: 0% (closed) to 100% (open), mapped to protocol 0xC800–0x0000
- **Stop**: Sends STOP command (0xD200)
- **Assumed state**: Position is inferred from commands, not reported by the motor

## Full Configuration Example

```yaml
esphome:
  name: my-io-homecontrol

esp32:
  board: esp32
  framework:
    type: esp-idf

logger:
  level: DEBUG

spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

cc1101:
  id: cc1101_radio
  cs_pin: GPIO5
  gdo0_pin: GPIO4
  frequency: 868.95MHz
  modulation_type: 2-FSK
  symbol_rate: 38400
  fsk_deviation: 19200

io_homecontrol:
  id: iohc_hub
  cc1101_id: cc1101_radio
  gdo0_pin: 4
  source_address: 0xAABBCC
  key: "A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6"
  tx_repeats: 4

cover:
  - platform: io_homecontrol
    io_homecontrol_id: iohc_hub
    name: "Living Room Blinds"
    address: 0x112233
  - platform: io_homecontrol
    io_homecontrol_id: iohc_hub
    name: "Bedroom Blinds"
    address: 0x445566
```

## Getting the Key and Addresses

To control io-homecontrol devices you need the **source address**, the **private
key**, and each **motor's address**. These are obtained by sniffing the pairing
process of an existing remote.

See **[PAIRING.md](PAIRING.md)** for a step-by-step guide.

**Short version:**
1. Flash with `pairing_mode: true`
2. Press buttons on your remote to discover addresses from the log output
3. Trigger a pairing on the remote while sniffer is running to capture the key
4. Copy the logged `source_address` and `key` into your config

## Sequence Number Management

Each source address has its own persistent sequence counter stored in NVS flash.
The component auto-tracks and persists counters across reboots, auto-increments
after each TX, and stays in sync when sharing an identity with a physical remote.

The `initial_sequence` option can force a higher starting value if the motor
rejects commands due to a stale counter.

> **Note on flash wear:** Sequence numbers are written to NVS on every transmitted
> command and every received packet from a tracked address. ESP32 NVS wear-leveling
> provides ~100,000 write cycles per partition. Normal use is negligible; high-traffic
> environments may want to batch writes in future.

## Architecture

```
__init__.py                      Hub Python config + code generation
io_homecontrol.h/.cpp            Hub C++ (frame building, parsing, crypto, CC1101 adapter)
cover/
  __init__.py                    Cover platform Python config
  io_homecontrol_cover.h/.cpp    Cover entity (open/close/stop/position)
PROTOCOL.md                      Full protocol technical reference
PAIRING.md                       Step-by-step pairing guide
```
