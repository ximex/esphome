# io-homecontrol 1W Pairing Guide

## Overview

io-homecontrol devices use AES-128 encrypted communication. Before your ESP32 can
control a motor, it needs the **private key** and the **addresses** of each motor.

This guide explains how to extract these values by sniffing the pairing process
of an existing remote, and how the protocol works behind the scenes.

## What You Need

- ESP32 with a **CC1101** radio module (e.g., TI CC1101-based breakout boards)
- An **existing paired remote** (e.g., Situo 5 io Pure II)

> **Why CC1101?** io-homecontrol uses UART-style encoding with start/stop bits around each byte.
> The CC1101's async serial mode natively supports this framing, ensuring reliable packet reception.
> This is the only radio currently supported for full io-homecontrol compatibility.

## Step 1: Flash the Pairing Sniffer

Use a CC1101 with the following minimal configuration:

```yaml
esphome:
  name: iohc-sniffer

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
  cc1101_id: cc1101_radio
  gdo0_pin: 4
  pairing_mode: true
```

Flash this to your board and open the serial log viewer.

## Step 2: Discover Motor Addresses

Press buttons on your existing remote (up/down/stop for each channel).
The log will show each frame with addresses:

```
[io_homecontrol] RX 1W frame: order=0 len=22 ...
[io_homecontrol]   Target: 0x112233 | Source: 0xAABBCC
[io_homecontrol]   CMD: 0x00 (EXECUTE)
[io_homecontrol]   ============ DISCOVERED ============
[io_homecontrol]   Remote/Controller address: 0xAABBCC
[io_homecontrol]   Actuator/Motor address:    0x112233
[io_homecontrol]   >> EXECUTE OPEN (param=0x0000)
[io_homecontrol]   >> Cover config:  address: 0x112233
[io_homecontrol]   ======================================
```

Write down:
- **Source address** = your remote's address (same for all channels)
- **Target address** = each motor's individual address (different per channel)

If the target is `0x00003F`, the remote is broadcasting to all motors.
Press individual channel buttons to find per-motor addresses.

## Step 3: Capture the Key (Pairing Sniff)

This is the critical step. You need to trigger a pairing event while the
sniffer is running.

### 3a. Put a Motor in Pairing Mode

On your **existing paired remote**:
1. Select the channel for the motor you want to pair
2. Press and **hold the PROG button** (small recessed button on the back)
   for approximately **2 seconds**
3. The motor will **jog** (move briefly up then down) confirming it entered
   learning/pairing mode
4. The motor stays in pairing mode for about **2 minutes**

### 3b. Trigger the Key Exchange

While the motor is in pairing mode, trigger a new pairing from the same
remote (or another remote you want to pair):

1. On the remote you want to capture the key from, press **PROG briefly**
   (short press, ~0.5 seconds)
2. The remote sends three commands in sequence:
   - `CMD 0x30 (SEND_KEY)` - the encrypted private key
   - `CMD 0x2E (1W_PAIR)` - pairing confirmation with HMAC
3. The motor jogs again to confirm successful pairing

### 3c. Read the Decrypted Key from the Log

The sniffer automatically decrypts the key and logs it:

```
[io_homecontrol]   !! SEND_KEY captured !!
[io_homecontrol]   Encrypted key: 7E60491F976ADF653DB0ED785E49A201
[io_homecontrol]   Manufacturer: 0x02  Data: 0x01  Seq: 4660
[io_homecontrol]   ========================================
[io_homecontrol]   DECRYPTED PRIVATE KEY: A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6
[io_homecontrol]   ========================================
[io_homecontrol]   Use these values in your YAML config:
[io_homecontrol]     source_address: 0xAABBCC
[io_homecontrol]     key: "A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6"
[io_homecontrol]   ========================================
```

With the captured key and addresses, update your config:

```yaml
esphome:
  name: my-iohc-device

esp32:
  board: esp32
  framework:
    type: esp-idf

spi:
  clk_pin: GPIO18
  mosi_pin: GPIO23
  miso_pin: GPIO19

cc1101:
  id: cc1101_radio
  cs_pin: GPIO5
  gdo0_pin: GPIO4
  frequency: 868.95MHz

io_homecontrol:
  cc1101_id: cc1101_radio
  gdo0_pin: 4
  source_address: 0xAABBCC          # From step 2 (your remote's address)
  key: "A1B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6"  # From step 3
  tx_repeats: 4

cover:
  - platform: io_homecontrol
    name: "Living Room Blinds"
    address: 0x112233               # From step 2 (motor address)
  - platform: io_homecontrol
    name: "Bedroom Blinds"
    address: 0x445566               # From step 2 (another motor)
```

---

## Method 2: Register ESP32 as a New Independent Remote (Active Pairing)

Instead of cloning an existing remote, you can register the ESP32 as its own
controller with a self-chosen address and a freshly generated key. The motor
stores your ESP32's key alongside any existing remotes — nothing is replaced.

### What You Need

- Your **motor's address** (obtain it from Method 1 Step 2 above, or by running
  the pairing sniffer and pressing buttons on an existing remote)
- An **existing paired remote** only to put the motor into learning mode
- A **CC1101** wired up normally (no pairing sniffer mode)

### Step 1: Choose Your Own Identity

Pick any values:

```yaml
source_address: 0xDEAD01   # any unused 3-byte hex value
key: "DEADBEEF0102030405060708090A0B0C"   # 32 random hex chars (16 bytes)
```

Generate a random key on Linux/macOS:
```bash
python3 -c "import os; print(os.urandom(16).hex().upper())"
```

### Step 2: Flash Normal Configuration

```yaml
io_homecontrol:
  id: iohc_hub
  cc1101_id: cc1101_radio
  gdo0_pin: 4
  source_address: 0xDEAD01
  key: "DEADBEEF0102030405060708090A0B0C"
  tx_repeats: 4

button:
  - platform: template
    name: "Pair ESP32 to Motor"
    on_press:
      - lambda: |-
          id(iohc_hub).send_pair(0x112233);  // replace with your motor's address
```

To broadcast to all motors currently in learning mode, use `0x00003F` as the
address.

### Step 3: Execute the Pairing

1. **Put the motor in learning mode**: On your existing remote, **hold PROG
   ~2 seconds** until the motor jogs (moves briefly up then down)
2. The motor stays in learning mode for about **2 minutes**
3. **Press the "Pair ESP32 to Motor" button** in Home Assistant
4. Watch the serial log for:
   ```
   PAIRING: Sending SEND_KEY to 0x112233
   PAIRING: Sent SEND_KEY + PAIR_1W successfully
   PAIRING: If motor jogs, pairing was successful!
   ```
5. If the **motor jogs again** → pairing succeeded

### Step 4: Remove the Pairing Button

After successful pairing the button is no longer needed and can be removed from
the config. Your ESP32 now controls the motor independently with its own
address and key.

---

## Protocol Details: How 1W Pairing Works

### Encryption Architecture

io-homecontrol uses three key layers:

| Key | Purpose | How obtained |
|-----|---------|-------------|
| **Transfer Key** | Encrypts private keys during pairing | Hardcoded in all devices: `34C3466ED88F4E8E16AA473949884373` |
| **Private Key** | Per-controller key for 1W HMAC authentication | Generated by the controller, shared via CMD 0x30 |
| **Stack Key** | Manufacturing key for 2W challenge-response | Burned into 2W devices, not extractable |

### 1W Pairing Sequence

```
Remote/Controller                    Actuator/Motor
      |                                    |
      |  [User holds PROG on existing      |
      |   remote -- motor jogs briefly]    |
      |                                    |  Motor enters learning mode (~2 min)
      |                                    |
      |--- CMD 0x30 (SEND_KEY) ----------->|  (encrypted with Transfer Key)
      |    [repeated 4x, 40ms interval]    |
      |                                    |  Motor decrypts and stores private key
      |                                    |
      |--- CMD 0x2E (1W_PAIR) ------------>|  (HMAC authenticated with new key)
      |    [repeated 4x, 40ms interval]    |
      |                                    |  Motor verifies HMAC, confirms pairing
      |                                    |  Motor jogs to confirm
      |                                    |
```

### CMD 0x30 (SEND_KEY) Frame Format

```
Byte  0:     CtrlByte0 (order[7:6] | S_flag[5] | frame_len[4:0])
                       S_flag: 0=1W (no suffix), 1=2W (8-byte suffix)
Byte  1:     CtrlByte1 = 0x00
Bytes 2-4:   Target broadcast address (e.g., 0x00003F)
Bytes 5-7:   Source address (controller)
Byte  8:     CMD = 0x30
Bytes 9-24:  Encrypted private key (16 bytes)
Byte  25:    Manufacturer ID (0x02 = Somfy)
Byte  26:    Data byte (0x01)
Bytes 27-28: Sequence number (MSB first)
Bytes 29-30: CRC-16/KERMIT (LSB first)
```

**No HMAC** is appended - the motor doesn't have the key yet.

### Key Encryption (CMD 0x30)

The private key is encrypted using AES-128-CFB128 which, for a single
16-byte block, simplifies to:

```
IV = source_address repeated to fill 16 bytes
    Example for addr 0xABCDEF:
    IV = [AB CD EF AB CD EF AB CD EF AB CD EF AB CD EF AB]

encrypted_key = AES_ECB(Transfer_Key, IV) XOR private_key
```

Decryption is the same operation (XOR is self-inverse):
```
private_key = AES_ECB(Transfer_Key, IV) XOR encrypted_key
```

This is why sniffing the pairing is so effective: the Transfer Key is
universally known, and the source address is visible in the frame.

### CMD 0x2E (1W_PAIR) Frame Format

```
Byte  0:     CtrlByte0 (order[7:6] | S_flag[5]=0 | frame_len[4:0])
Byte  1:     CtrlByte1 = 0x00
Bytes 2-4:   Target broadcast address
Bytes 5-7:   Source address (controller)
Byte  8:     CMD = 0x2E
Byte  9:     Data = 0x00
Bytes 10-11: Sequence number (MSB first)
Bytes 12-17: HMAC (first 6 bytes of AES-128-ECB encrypted IV)
Bytes 18-19: CRC-16/KERMIT (LSB first)
```

The HMAC proves the controller possesses the key it just transmitted.

### 1W HMAC Computation

For every authenticated 1W frame:

```
1. IV construction (16 bytes):
   [first 8 bytes of {cmd, data...}, padded with 0x55]
   [2-byte custom checksum over all {cmd, data...} bytes]
   [2-byte sequence number, MSB first]
   [4 bytes 0x55 padding]

2. Encrypt: AES-128-ECB(private_key, IV) -> 16 bytes

3. HMAC = first 6 bytes of the encrypted result
```

### CMD 0x39 (REMOVE_CTRL)

Same frame format as CMD 0x2E but with command byte 0x39.
Used to remove a controller's key from a motor. Authenticated
with the existing key (must be sent before the motor forgets it).

### Normal Operation: CMD 0x00 (EXECUTE)

After pairing, the controller sends EXECUTE commands:

```
Bytes 9+:   [Originator 1B][ACEI 1B][MainParam 2B][FP1 1B][FP2 1B]
            [Sequence 2B][HMAC 6B]

Main Parameter values:
  0x0000 = Fully open (UP)
  0xC800 = Fully closed (DOWN)
  0xD200 = Stop
  0xD300 = My position (preset)
  0x0001-0xC7FF = Intermediate position (percentage)
```

**Troubleshooting:**

**No packets received:**
- Verify CC1101 is wired correctly (SPI CLK/MOSI/MISO, chip select, GDO0)
- Check frequency is set to 868.95 MHz (CH2, the 1W channel)
- Verify the remote is within range (1-2 meters)
- Check serial logs for any CC1101 initialization errors

**CRC/Framing errors:**
- CC1101 UART async mode handles UART framing automatically
- If still seeing errors, verify CC1101 clock and symbol rate configuration
- Ensure GDO0 pin is correctly configured for serial data output

**SEND_KEY not captured:**
- The key exchange only happens during pairing
- You must first put the motor in learning mode (hold PROG 2 sec on existing remote)
- Then trigger the pairing (short press PROG on the remote whose key you want)
- The sniffer must be running during the entire pairing process
