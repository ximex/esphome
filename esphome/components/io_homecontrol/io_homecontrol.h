#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"
#include "esphome/components/cc1101/cc1101.h"

#include <array>
#include <initializer_list>
#include <vector>

#ifdef USE_ESP_IDF
#include "driver/uart.h"
#endif

namespace esphome::io_homecontrol {

#ifdef USE_IO_HOMECONTROL_COVER
class IOHomecontrolCover;
#endif

// --- io-homecontrol command IDs (Layer 3) ---
enum class Command : uint8_t {
  EXECUTE = 0x00,
  ACTIVATE_MODE = 0x01,
  PRIVATE_CMD = 0x03,
  PRIVATE_ANS = 0x04,
  WRITE_PRIVATE = 0x20,
  PRIVATE_ACK = 0x21,
  DISCOVER = 0x28,
  DISCOVER_ANS = 0x29,
  DISCOVER_CONFIRM = 0x2C,
  DISCOVER_CONF_ACK = 0x2D,
  PAIR_1W = 0x2E,
  SEND_KEY = 0x30,
  ASK_CHALLENGE = 0x31,
  KEY_TRANSFER = 0x32,
  KEY_TRANSFER_ACK = 0x33,
  LAUNCH_KEY_XFER = 0x34,
  REMOVE_CTRL = 0x39,
  CHALLENGE_REQ = 0x3C,
  CHALLENGE_ANS = 0x3D,
  GET_NAME = 0x50,
  GET_NAME_ANS = 0x51,
};

// --- EXECUTE command main parameter values ---
enum class MainParam : uint16_t {
  OPEN = 0x0000,
  CLOSE = 0xC800,
  STOP = 0xD200,
  MY_POS = 0xD300,
};

// --- Frame structure sizes (bytes) ---
static constexpr size_t FRAME_HEADER_SIZE =
    9;  // CtrlByte0 + CtrlByte1 + Target(3) + Source(3) + CMD  [wire: Tgt first]
static constexpr size_t ADDRESS_SIZE = 3;
static constexpr size_t CRC_SIZE = 2;
static constexpr size_t SEQ_SIZE = 2;
static constexpr size_t MAC_SIZE = 6;
static constexpr size_t HMAC_AUTH_SIZE = SEQ_SIZE + MAC_SIZE;  // Seq(2) + MAC(6) appended to 1W frames
// CtrlByte0 length field: L = total_packet_size - CTRL0_L_EXCLUDED_BYTES
// Per spec: L excludes CtrlByte0 (1 byte) and CRC (2 bytes)
static constexpr size_t CTRL0_L_EXCLUDED_BYTES = 1 + CRC_SIZE;          // = 3 (CtrlByte0 + CRC not counted in L field)
static constexpr size_t MIN_FRAME_SIZE = FRAME_HEADER_SIZE + CRC_SIZE;  // Minimum valid packet = 11
static constexpr size_t MAX_FRAME_LEN = 31;                             // Max value in CtrlByte0 length field (5 bits)
static constexpr size_t MAX_PACKET_SIZE = MAX_FRAME_LEN + CTRL0_L_EXCLUDED_BYTES;  // L_max + 3 = 34

// SEND_KEY frame: [Header 9][EncKey 16][ManID 1][Data 1][Seq 2][CRC 2]
static constexpr size_t SEND_KEY_MIN_SIZE = 31;

// --- AES-128 ---
static constexpr size_t KEY_SIZE = 16;

// --- CRC-16/KERMIT ---
static constexpr uint16_t KERMIT_INIT = 0x0000;
static constexpr uint16_t KERMIT_POLY = 0x8408;

// --- CtrlByte0 bit layout ---
static constexpr uint8_t CTRL0_2W_BIT = 0x20;    // Bit 5: 0=1W, 1=2W
static constexpr uint8_t CTRL0_LEN_MASK = 0x1F;  // Bits 4-0: frame length

// --- CtrlByte1 bit layout ---
static constexpr uint8_t CTRL1_BEACON_BIT = 0x80;  // Bit 7: use beacon/repeater
static constexpr uint8_t CTRL1_ACK_BIT = 0x10;     // Bit 4: ACK capable (2W device)

// --- HMAC IV construction ---
static constexpr uint8_t HMAC_PADDING = 0x55;
static constexpr size_t HMAC_IV_DATA_BYTES = 8;  // Max cmd+data bytes copied into HMAC IV (bytes 0-7)

// --- EXECUTE command payload fields ---
static constexpr uint8_t ORIGINATOR_USER = 0x01;
static constexpr uint8_t ACEI_DEFAULT = 0x00;  // Access Control Extension & priority Info

// --- TX timing ---
static constexpr uint32_t TX_REPEAT_DELAY_MS = 40;
// Preamble for async serial TX: 0x55 bytes (alternating bits) for AGC lock + bit sync
static constexpr size_t TX_PREAMBLE_BYTES = 10;
// Sync word bytes
static constexpr uint8_t TX_SYNC1 = 0xFF;
static constexpr uint8_t TX_SYNC2 = 0x33;

// --- Addresses ---
static constexpr uint32_t ADDR_BROADCAST = 0x00003F;

// --- Radio channels ---
static constexpr uint32_t FREQ_CH1 = 868250000;  // 2W only
static constexpr uint32_t FREQ_CH2 = 868950000;  // 1W + 2W
static constexpr uint32_t FREQ_CH3 = 869850000;  // 2W only
static constexpr size_t NUM_CHANNELS = 3;
static constexpr uint32_t SCAN_DWELL_MS = 50;  // ms per channel during scanning

// --- SEND_KEY payload fields ---
static constexpr uint8_t MANUFACTURER_SOMFY = 0x02;
static constexpr uint8_t SEND_KEY_DATA_BYTE = 0x01;  // Data field in SEND_KEY payload (purpose unknown, always 0x01)

// Universally known Transfer Key for 1W key exchange (CMD 0x30).
// Hardcoded in every io-homecontrol device worldwide. During 1W pairing, the
// controller encrypts its private key with this key before transmitting it.
// Since the Transfer Key is publicly known, any sniffer can decrypt captured
// private keys from SEND_KEY frames.
static constexpr uint8_t TRANSFER_KEY[KEY_SIZE] = {
    0x34, 0xC3, 0x46, 0x6E, 0xD8, 0x8F, 0x4E, 0x8E, 0x16, 0xAA, 0x47, 0x39, 0x49, 0x88, 0x43, 0x73,
};

// UART RX state machine for async serial mode
enum class RxState : uint8_t {
  WAITING_SYNC_FF,  // Scanning for 0xFF (preamble terminator start)
  WAITING_SYNC_33,  // Got 0xFF, expecting 0x33 (preamble terminator end)
  READING_HEADER,   // Reading CtrlByte0 to determine frame length
  READING_FRAME,    // Reading remaining frame bytes
};

// UART RX timeout: max time to receive a complete packet after sync
static constexpr uint32_t UART_RX_TIMEOUT_MS = 20;
// UART RX buffer size (must be >= MAX_PACKET_SIZE)
static constexpr int UART_RX_BUF_SIZE = 256;

// Persistent sequence number entry for a source address
struct SequenceEntry {
  uint32_t address;
  uint16_t sequence;
  ESPPreferenceObject pref;
};

class IOHomecontrol : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Radio setter for CC1101
  void set_cc1101_radio(cc1101::CC1101Component *radio) { this->radio_ = radio; }
  void set_gdo0_pin(uint8_t pin) { this->gdo0_pin_ = pin; }

  void set_source_address(uint32_t address) { this->source_address_ = address; }
  void set_key(std::initializer_list<uint8_t> key);
  void set_tx_repeats(uint8_t repeats) { this->tx_repeats_ = repeats; }
  void set_pairing_mode(bool mode) { this->pairing_mode_ = mode; }
  void set_initial_sequence(uint16_t seq) { this->initial_sequence_ = seq; }

#ifdef USE_IO_HOMECONTROL_COVER
  void register_cover(IOHomecontrolCover *cover);
#endif

  /// Send EXECUTE command (CMD 0x00) with main parameter.
  /// source_address identifies the controller channel paired to the motor.
  /// target_address is typically ADDR_BROADCAST.
  bool send_execute(uint32_t source_address, uint16_t main_param, uint8_t func_param_1 = 0x00,
                    uint8_t func_param_2 = 0x00, uint32_t target_address = ADDR_BROADCAST);

  /// Pair as a new controller: sends SEND_KEY + PAIR_1W
  /// Motor must be in learning mode (hold PROG on existing remote first)
  bool send_pair(uint32_t target_address = ADDR_BROADCAST);

 protected:
  /// CRC-16/KERMIT helpers
  static uint16_t compute_crc_(const uint8_t *data, size_t len);
  /// 1W HMAC computation (AES-128-ECB)
  void compute_1w_hmac_(const uint8_t *frame_data, size_t len, uint16_t seq, uint8_t *mac_out);

  /// Process bytes from ESP32 UART (async serial RX from CC1101 GDO0)
  void process_uart_rx_();

  /// Switch radio to 1W channel (CH2) if not already there
  void ensure_1w_channel_();

  /// Build and transmit a 1W frame
  bool send_1w_frame_(uint32_t source, uint32_t target, Command cmd, const uint8_t *data, size_t data_len);

  /// Transmit raw frame bytes using CC1101 async serial TX + ESP32 UART TX.
  /// Prepends preamble (0x55 × N) and sync word (0xFF 0x33) automatically.
  /// Each byte is UART-encoded (start bit + 8 bits LSB-first + stop bit) by the hardware.
  bool transmit_serial_(const uint8_t *frame, size_t len);

  /// Parse a received io-homecontrol frame
  void parse_frame_(const uint8_t *packet, size_t packet_size);

  /// Decrypt a 1W private key from a captured CMD 0x30 (SEND_KEY) frame
  static void decrypt_1w_key_(const uint8_t *source_addr_3b, const uint8_t *enc_key_16b, uint8_t *out_key_16b);

  /// Get human-readable command name
  static const char *get_command_name_(Command cmd);

  /// Get human-readable main parameter name
  static const char *get_param_name_(uint16_t param);

  /// Find or create a sequence entry for a source address (loads from flash on first access)
  SequenceEntry *get_sequence_entry_(uint32_t address);

  cc1101::CC1101Component *radio_{nullptr};
#ifdef USE_IO_HOMECONTROL_COVER
  std::vector<IOHomecontrolCover *> covers_;
#endif

  std::array<uint8_t, KEY_SIZE> key_{};
  uint32_t source_address_{0};
  optional<uint16_t> initial_sequence_{};
  uint8_t tx_repeats_{4};
  bool pairing_mode_{false};

  // Channel scanning state (active in pairing mode)
  uint8_t scan_channel_idx_{0};
  uint32_t last_channel_switch_{0};
  uint32_t current_freq_{FREQ_CH2};

  // TX repeat state machine (non-blocking repeats in loop())
  bool tx_pending_{false};
  uint8_t tx_remaining_repeats_{0};
  uint32_t tx_last_send_time_{0};
  size_t tx_frame_len_{0};
  std::array<uint8_t, MAX_PACKET_SIZE> tx_frame_{};
  uint32_t tx_seq_address_{0};  // Source address for sequence increment after all repeats

  // UART RX state (async serial from CC1101 GDO0)
  uint8_t gdo0_pin_{0};
  RxState rx_state_{RxState::WAITING_SYNC_FF};
  std::array<uint8_t, MAX_PACKET_SIZE> rx_buffer_{};
  size_t rx_buffer_len_{0};
  size_t rx_expected_len_{0};
  uint32_t rx_frame_start_{0};
  float rx_rssi_{0.0f};  // RSSI sampled at sync word detection
  uint8_t rx_lqi_{0};    // LQI sampled at sync word detection

  // Grows per unique source address seen. In pairing mode on busy networks,
  // consider limiting to prevent unbounded growth.
  std::vector<SequenceEntry> sequence_entries_;
};

}  // namespace esphome::io_homecontrol
