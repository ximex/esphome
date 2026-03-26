#include "io_homecontrol.h"
#ifdef USE_IO_HOMECONTROL_COVER
#include "cover/io_homecontrol_cover.h"
#endif
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32
#include <mbedtls/esp_config.h>
#include <mbedtls/aes.h>
#include <driver/gpio.h>
#endif

namespace esphome::io_homecontrol {

static const char *const TAG = "io_homecontrol";
#ifdef USE_ESP_IDF
static constexpr uart_port_t UART_PORT = UART_NUM_1;
#endif

void IOHomecontrol::set_key(std::initializer_list<uint8_t> key) {
  size_t copy_len = std::min(key.size(), KEY_SIZE);
  std::copy_n(key.begin(), copy_len, this->key_.begin());
  if (copy_len < KEY_SIZE) {
    std::fill(this->key_.begin() + copy_len, this->key_.end(), 0);
  }
}

void IOHomecontrol::setup() {
  // Enable UART async serial mode on CC1101 for io-homecontrol UART framing
  this->radio_->set_packet_mode(false);

#ifdef USE_ESP_IDF
  // Configure ESP32 hardware UART to read demodulated serial data from CC1101 GDO0.
  // io-homecontrol uses UART-style encoding (start bit + 8 data bits LSB-first + stop bit)
  // at 38400 baud. The ESP32 UART peripheral strips start/stop bits automatically.
  uart_config_t uart_config = {};
  uart_config.baud_rate = 38400;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  uart_param_config(UART_PORT, &uart_config);
  uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, this->gdo0_pin_, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_PORT, UART_RX_BUF_SIZE, 0, 0, nullptr, 0);
  ESP_LOGI(TAG, "UART%d configured on GPIO%u at 38400 baud for CC1101 async serial RX", UART_PORT, this->gdo0_pin_);
#endif

  // Apply initial_sequence override if provided (for hub source_address used in pairing)
  if (this->source_address_ != 0 && this->initial_sequence_.has_value()) {
    auto *entry = this->get_sequence_entry_(this->source_address_);
    if (*this->initial_sequence_ > entry->sequence) {
      entry->sequence = *this->initial_sequence_;
      entry->pref.save(&entry->sequence);
      global_preferences->sync();
    }
  }

  ESP_LOGI(TAG, "io-homecontrol hub initialized, source=0x%06X", this->source_address_);
  if (this->pairing_mode_) {
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "  PAIRING SNIFFER MODE ACTIVE");
    ESP_LOGW(TAG, "  Press buttons on your remote to");
    ESP_LOGW(TAG, "  discover addresses.");
    ESP_LOGW(TAG, "  Trigger a pairing to capture the key.");
    ESP_LOGW(TAG, "========================================");
  }
}

void IOHomecontrol::loop() {
  // Process incoming bytes from ESP32 UART (CC1101 async serial RX)
  this->process_uart_rx_();

  // Handle non-blocking TX repeats
  if (this->tx_pending_) {
    uint32_t now = millis();
    if (now - this->tx_last_send_time_ >= TX_REPEAT_DELAY_MS) {
      if (!this->transmit_serial_(this->tx_frame_.data(), this->tx_frame_len_)) {
        ESP_LOGE(TAG, "Transmit failed on repeat %u", this->tx_repeats_ - this->tx_remaining_repeats_);
        this->tx_pending_ = false;
      } else {
        this->tx_last_send_time_ = now;
        this->tx_remaining_repeats_--;
        if (this->tx_remaining_repeats_ == 0) {
          this->tx_pending_ = false;
          // Increment and persist sequence number after all repeats complete
          auto *seq_entry = this->get_sequence_entry_(this->tx_seq_address_);
          seq_entry->sequence++;
          seq_entry->pref.save(&seq_entry->sequence);
          global_preferences->sync();
        }
      }
    }
  }

  // In pairing mode, scan all 3 channels to detect 2W traffic
  if (this->pairing_mode_) {
    static constexpr uint32_t CHANNELS[NUM_CHANNELS] = {FREQ_CH1, FREQ_CH2, FREQ_CH3};
    uint32_t now = millis();
    if (now - this->last_channel_switch_ >= SCAN_DWELL_MS) {
      this->scan_channel_idx_ = (this->scan_channel_idx_ + 1) % NUM_CHANNELS;
      this->current_freq_ = CHANNELS[this->scan_channel_idx_];
      this->radio_->set_frequency(static_cast<float>(this->current_freq_));
      this->last_channel_switch_ = now;
    }
  }
}

void IOHomecontrol::dump_config() {
  if (this->pairing_mode_) {
    ESP_LOGCONFIG(TAG,
                  "io-homecontrol:\n"
                  "  Pairing Mode: %s\n"
                  "  Min RSSI: %.1f dBm",
                  TRUEFALSE(this->pairing_mode_), this->min_rssi_);
  } else {
    char hex_buf[KEY_SIZE * 2 + 1];
    format_hex_to(hex_buf, this->key_.data(), KEY_SIZE);
    ESP_LOGCONFIG(TAG,
                  "io-homecontrol:\n"
                  "  Pairing Mode: %s\n"
                  "  Source Address: 0x%06X\n"
                  "  TX Repeats: %u\n"
                  "  Min RSSI: %.1f dBm\n"
                  "  Key: %s",
                  TRUEFALSE(this->pairing_mode_), this->source_address_, this->tx_repeats_, this->min_rssi_, hex_buf);
  }
}

#ifdef USE_IO_HOMECONTROL_COVER
void IOHomecontrol::register_cover(IOHomecontrolCover *cover) { this->covers_.push_back(cover); }
#endif

SequenceEntry *IOHomecontrol::get_sequence_entry_(uint32_t address) {
  for (auto &entry : this->sequence_entries_) {
    if (entry.address == address) {
      return &entry;
    }
  }
  // Create new entry and load from flash
  uint32_t pref_key = fnv1_hash("iohc_seq") ^ address;
  SequenceEntry new_entry{};
  new_entry.address = address;
  new_entry.sequence = 0;
  new_entry.pref = global_preferences->make_preference<uint16_t>(pref_key);
  new_entry.pref.load(&new_entry.sequence);
  if (this->sequence_entries_.size() >= MAX_SEQ_ENTRIES) {
    // Evict oldest entry (index 0) when at capacity by shifting all entries down
    ESP_LOGW(TAG, "Sequence table full (%u entries), evicting 0x%06X",
             static_cast<unsigned>(this->sequence_entries_.size()), this->sequence_entries_[0].address);
    // Shift entries down and overwrite the last slot
    for (size_t i = 1; i < this->sequence_entries_.size(); i++) {
      this->sequence_entries_[i - 1] = this->sequence_entries_[i];
    }
    this->sequence_entries_[this->sequence_entries_.size() - 1] = new_entry;
    return &this->sequence_entries_[this->sequence_entries_.size() - 1];
  }
  this->sequence_entries_.push_back(new_entry);
  return &this->sequence_entries_[this->sequence_entries_.size() - 1];
}

// ============================================================================
// UART RX processing (async serial from CC1101 GDO0)
// ============================================================================

void IOHomecontrol::process_uart_rx_() {
#ifdef USE_ESP_IDF
  uint8_t buf[64];
  int len = uart_read_bytes(UART_PORT, buf, sizeof(buf), 0);
  if (len <= 0)
    return;

  uint32_t now = millis();
  for (int i = 0; i < len; i++) {
    uint8_t byte = buf[i];

    // Timeout: reset if we've been assembling a frame for too long
    if (this->rx_state_ != RxState::WAITING_SYNC_FF && this->rx_state_ != RxState::WAITING_SYNC_33) {
      if (now - this->rx_frame_start_ > UART_RX_TIMEOUT_MS) {
        ESP_LOGD(TAG, "UART RX timeout, resetting state machine");
        this->rx_state_ = RxState::WAITING_SYNC_FF;
      }
    }

    switch (this->rx_state_) {
      case RxState::WAITING_SYNC_FF:
        if (byte == SYNC_BYTE_1) {
          this->rx_state_ = RxState::WAITING_SYNC_33;
        }
        break;

      case RxState::WAITING_SYNC_33:
        if (byte == SYNC_BYTE_2) {
          // Sync word detected — sample RSSI/LQI while signal is still present
          this->rx_rssi_ = this->radio_->read_rssi();
          this->rx_lqi_ = this->radio_->read_lqi();
          this->rx_state_ = RxState::READING_HEADER;
          this->rx_buffer_len_ = 0;
          this->rx_frame_start_ = now;
        } else if (byte == SYNC_BYTE_1) {
          // Multiple 0xFF in a row — stay in this state
        } else {
          this->rx_state_ = RxState::WAITING_SYNC_FF;
        }
        break;

      case RxState::READING_HEADER: {
        // First byte after sync is CtrlByte0 — check RSSI before committing to frame
        if (this->rx_rssi_ < this->min_rssi_) {
          ESP_LOGD(TAG, "Frame rejected: RSSI=%.1fdBm below threshold %.1fdBm (ctrl0=0x%02X)", this->rx_rssi_,
                   this->min_rssi_, byte);
          // this->rx_state_ = RxState::WAITING_SYNC_FF;
          // break;
        }

        this->rx_buffer_[this->rx_buffer_len_++] = byte;
        size_t frame_len_field = byte & CTRL0_LEN_MASK;
        this->rx_expected_len_ = frame_len_field + CTRL0_L_EXCLUDED_BYTES;

        if (this->rx_expected_len_ < MIN_FRAME_SIZE || this->rx_expected_len_ > MAX_PACKET_SIZE) {
          ESP_LOGD(TAG, "Invalid CtrlByte0 length: 0x%02X (expected %u bytes) RSSI=%.1fdBm", byte,
                   static_cast<unsigned>(this->rx_expected_len_), this->rx_rssi_);
          this->rx_state_ = RxState::WAITING_SYNC_FF;
        } else {
          this->rx_state_ = RxState::READING_FRAME;
        }
        break;
      }

      case RxState::READING_FRAME:
        this->rx_buffer_[this->rx_buffer_len_++] = byte;
        if (this->rx_buffer_len_ >= this->rx_expected_len_) {
          // Complete packet — deliver to parser
          this->parse_frame_(this->rx_buffer_.data(), this->rx_buffer_len_);
          this->rx_state_ = RxState::WAITING_SYNC_FF;
        }
        break;
    }
  }
#endif
}

// ============================================================================
// CRC-16/X.25 (init=0xFFFF, poly=0x8408 reflected, refin=true, refout=true, xorout=0xFFFF)
// ============================================================================

uint16_t IOHomecontrol::compute_crc_(const uint8_t *data, size_t len) {
  // refout=false triggers the helper's final XOR with 0xFFFF, which implements xorout=0xFFFF
  return crc16(data, static_cast<uint16_t>(len), CRC_INIT, CRC_POLY, true, false);
}

// ============================================================================
// 1W HMAC computation using AES-128-ECB
// Input: command_id byte + parameter data (NOT the full frame header)
// ============================================================================

/// Custom checksum used in the io-homecontrol HMAC IV (bytes 8-9).
/// Processes each byte of {command_id, params...} through a shift-XOR algorithm.
/// See PROTOCOL.md "1W HMAC Authentication" for algorithm details.
static void compute_hmac_checksum_(const uint8_t *data, size_t len, uint8_t &chksum1, uint8_t &chksum2) {
  chksum1 = 0;
  chksum2 = 0;
  for (size_t i = 0; i < len; i++) {
    uint8_t tmp = data[i] ^ chksum2;
    chksum2 = ((chksum1 & 0x7F) << 1) & 0xFF;
    if ((chksum1 & 0x80) == 0) {
      if (tmp >= 128) {
        chksum2 |= 1;
      }
      chksum1 = chksum2;
      chksum2 = (tmp << 1) & 0xFF;
    } else {
      if (tmp >= 128) {
        chksum2 |= 1;
      }
      chksum1 = chksum2 ^ 0x55;
      chksum2 = ((tmp << 1) ^ 0x5B) & 0xFF;
    }
  }
}

void IOHomecontrol::compute_1w_hmac_(const uint8_t *frame_data, size_t len, uint16_t seq, uint8_t *mac_out) {
#ifdef USE_ESP32
  // Build 16-byte IV
  // frame_data points to {command_id, param[0], param[1], ...}
  std::array<uint8_t, KEY_SIZE> iv{};

  // Bytes 0-7: first 8 bytes of command+data, padded with 0x55
  size_t copy_len = std::min(len, HMAC_IV_DATA_BYTES);
  std::memcpy(iv.data(), frame_data, copy_len);
  if (copy_len < HMAC_IV_DATA_BYTES) {
    std::memset(iv.data() + copy_len, HMAC_PADDING, HMAC_IV_DATA_BYTES - copy_len);
  }

  // Bytes 8-9: custom checksum over ALL command+data bytes
  uint8_t chksum1, chksum2;
  compute_hmac_checksum_(frame_data, len, chksum1, chksum2);
  iv[8] = chksum1;
  iv[9] = chksum2;

  // Bytes 10-11: sequence number (MSB first)
  iv[10] = (seq >> 8) & 0xFF;
  iv[11] = seq & 0xFF;

  // Bytes 12-15: padding
  std::memset(iv.data() + 12, HMAC_PADDING, 4);

  // AES-128-ECB encrypt
  std::array<uint8_t, KEY_SIZE> encrypted{};
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, this->key_.data(), KEY_SIZE * 8);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, iv.data(), encrypted.data());
  mbedtls_aes_free(&ctx);

  // MAC = first 6 bytes of encrypted result
  std::memcpy(mac_out, encrypted.data(), MAC_SIZE);
#else
  // AES not available on non-ESP32 platforms
  ESP_LOGE(TAG, "1W HMAC requires ESP32 (mbedtls)");
  std::memset(mac_out, 0, MAC_SIZE);
#endif
}

// ============================================================================
// Switch radio to 1W channel (CH2, 868.95 MHz)
// ============================================================================

void IOHomecontrol::ensure_1w_channel_() {
  if (this->current_freq_ != FREQ_CH2) {
    this->radio_->set_frequency(static_cast<float>(FREQ_CH2));
    this->current_freq_ = FREQ_CH2;
  }
}

// ============================================================================
// Async serial TX via CC1101 + ESP32 UART
// CC1101 async TX mode reads GDO0 as serial input and modulates it.
// ESP32 UART TX drives GDO0 with proper UART framing (start + 8 bits LSB-first + stop).
// ============================================================================

bool IOHomecontrol::transmit_serial_(const uint8_t *frame, size_t len) {
#ifdef USE_ESP_IDF
  // Build TX buffer: preamble (0x55 × N) + sync (0xFF 0x33) + frame bytes
  // UART hardware adds start/stop bits around each byte automatically
  static constexpr size_t TX_BUF_MAX = TX_PREAMBLE_BYTES + 2 + MAX_PACKET_SIZE;
  const size_t tx_len = TX_PREAMBLE_BYTES + 2 + len;
  std::array<uint8_t, TX_BUF_MAX> tx_buf{};
  for (size_t i = 0; i < TX_PREAMBLE_BYTES; i++) {
    tx_buf[i] = PREAMBLE_BYTE;
  }
  tx_buf[TX_PREAMBLE_BYTES] = SYNC_BYTE_1;
  tx_buf[TX_PREAMBLE_BYTES + 1] = SYNC_BYTE_2;
  std::memcpy(tx_buf.data() + TX_PREAMBLE_BYTES + 2, frame, len);

  // CC1101: enter IDLE (async serial mode PKT_FORMAT=3 is already set)
  this->radio_->go_idle();

  // Reconfigure UART TX pin to GDO0 so ESP32 UART drives the CC1101 GDO0 input
  uart_set_pin(UART_PORT, static_cast<int>(this->gdo0_pin_), UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);

  // Pre-load UART TX FIFO (returns immediately, hardware FIFO is 128 bytes)
  uart_write_bytes(UART_PORT, tx_buf.data(), static_cast<int>(tx_len));

  // CC1101: enter TX state — reads GDO0 serial input and modulates it
  this->radio_->go_tx();

  // Wait for UART TX to finish: (tx_len bytes × 10 bits/byte) / 38400 baud + margin
  uint32_t tx_time_ms = (static_cast<uint32_t>(tx_len) * 10 * 1000 / 38400) + 30;
  if (uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(tx_time_ms)) != ESP_OK) {
    ESP_LOGW(TAG, "Async TX UART timeout after %ums", tx_time_ms);
  }

  // CC1101: back to IDLE, then restore GDO0 for RX
  this->radio_->go_idle();
  // Disconnect UART TX output from GDO0 pin, then reconnect as UART RX input
  gpio_reset_pin(static_cast<gpio_num_t>(this->gdo0_pin_));
  uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, static_cast<int>(this->gdo0_pin_), UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
  this->radio_->go_rx();

  return true;
#else
  ESP_LOGE(TAG, "Async serial TX not supported on this platform");
  return false;
#endif
}

// ============================================================================
// Build and send a 1W frame
// ============================================================================

bool IOHomecontrol::send_1w_frame_(uint32_t source, uint32_t target, Command cmd, const uint8_t *data,
                                   size_t data_len) {
  if (this->radio_ == nullptr) {
    ESP_LOGE(TAG, "Radio not configured");
    return false;
  }

  // 1W TX always uses CH2 (868.95 MHz) - switch if scanning is on another channel
  this->ensure_1w_channel_();

  // Frame layout for 1W:
  // [CtrlByte0][CtrlByte1][TargetAddr 3B][SourceAddr 3B][CMD 1B][Data N B][SeqNum 2B][MAC 6B][CRC16 2B]
  //
  // Total size = FRAME_HEADER_SIZE + data_len + HMAC_AUTH_SIZE + CRC_SIZE
  // CtrlByte0 length field (bits 4-0) = total_size - MIN_FRAME_SIZE (bytes beyond minimum frame)
  size_t total_size = FRAME_HEADER_SIZE + data_len + HMAC_AUTH_SIZE + CRC_SIZE;
  size_t frame_len = total_size - CTRL0_L_EXCLUDED_BYTES;  // L = total - 3 (excludes CtrlByte0 and CRC)
  if (frame_len > MAX_FRAME_LEN) {
    ESP_LOGE(TAG, "Frame too long: %u bytes", static_cast<unsigned>(total_size));
    return false;
  }
  std::array<uint8_t, MAX_PACKET_SIZE> frame{};

  // CtrlByte0: order=00 (single frame), 1W mode (bit 5=0), length
  frame[0] = frame_len & CTRL0_LEN_MASK;

  // CtrlByte1: no flags
  frame[1] = 0x00;

  // Target address (3 bytes, big-endian) — wire format: [CB0][CB1][Target 3B][Source 3B]
  frame[2] = (target >> 16) & 0xFF;
  frame[3] = (target >> 8) & 0xFF;
  frame[4] = target & 0xFF;

  // Source address (3 bytes, big-endian)
  frame[5] = (source >> 16) & 0xFF;
  frame[6] = (source >> 8) & 0xFF;
  frame[7] = source & 0xFF;

  // Command
  frame[8] = static_cast<uint8_t>(cmd);

  // Data payload
  if (data_len > 0 && data != nullptr) {
    std::memcpy(frame.data() + FRAME_HEADER_SIZE, data, data_len);
  }

  // Get sequence number for this source address
  auto *seq_entry = this->get_sequence_entry_(source);
  uint16_t seq = seq_entry->sequence;

  // Compute HMAC over command_id + data (NOT the full frame header)
  // HMAC input starts at CMD byte (last byte of header) + data payload
  size_t hmac_offset = FRAME_HEADER_SIZE - 1;  // CMD byte position
  size_t hmac_len = 1 + data_len;              // CMD + data
  uint8_t mac[MAC_SIZE];
  this->compute_1w_hmac_(frame.data() + hmac_offset, hmac_len, seq, mac);

  // Append sequence number (MSB first)
  size_t auth_offset = FRAME_HEADER_SIZE + data_len;
  frame[auth_offset] = (seq >> 8) & 0xFF;
  frame[auth_offset + 1] = seq & 0xFF;

  // Append MAC
  std::memcpy(frame.data() + auth_offset + SEQ_SIZE, mac, MAC_SIZE);

  // Compute and append CRC-16/KERMIT (LSB first)
  size_t crc_offset = total_size - CRC_SIZE;
  uint16_t crc = compute_crc_(frame.data(), crc_offset);
  frame[crc_offset] = crc & 0xFF;
  frame[crc_offset + 1] = (crc >> 8) & 0xFF;

  // Log the frame
  char hex_buf[MAX_PACKET_SIZE * 2 + 1];
  size_t hex_len = std::min(total_size, MAX_PACKET_SIZE);
  format_hex_to(hex_buf, frame.data(), hex_len);
  ESP_LOGI(TAG, "TX 1W frame: src=0x%06X -> target=0x%06X cmd=0x%02X (%s) seq=%u len=%u", source, target,
           static_cast<uint8_t>(cmd), get_command_name_(cmd), seq, static_cast<unsigned>(total_size));
  ESP_LOGD(TAG, "  TX raw: %s", hex_buf);

  // Transmit first repeat immediately, queue remaining for non-blocking delivery in loop()
  if (!this->transmit_serial_(frame.data(), total_size)) {
    ESP_LOGE(TAG, "Transmit failed on first attempt");
    return false;
  }

  uint8_t remaining = this->tx_repeats_ - 1;
  if (remaining > 0) {
    std::memcpy(this->tx_frame_.data(), frame.data(), total_size);
    this->tx_frame_len_ = total_size;
    this->tx_remaining_repeats_ = remaining;
    this->tx_last_send_time_ = millis();
    this->tx_seq_address_ = source;
    this->tx_pending_ = true;
  } else {
    // Single transmit — increment sequence immediately
    seq_entry->sequence = seq + 1;
    seq_entry->pref.save(&seq_entry->sequence);
    global_preferences->sync();
  }

  return true;
}

// ============================================================================
// Public API: Send EXECUTE command
// ============================================================================

bool IOHomecontrol::send_execute(uint32_t source_address, uint16_t main_param, uint8_t func_param_1,
                                 uint8_t func_param_2, uint32_t target_address) {
  // EXECUTE command payload:
  // [Originator 1B][ACEI 1B][MainParam 2B][FuncParam1 1B][FuncParam2 1B]
  uint8_t data[6];
  data[0] = ORIGINATOR_USER;
  data[1] = ACEI_DEFAULT;
  data[2] = (main_param >> 8) & 0xFF;
  data[3] = main_param & 0xFF;
  data[4] = func_param_1;
  data[5] = func_param_2;

  return this->send_1w_frame_(source_address, target_address, Command::EXECUTE, data, sizeof(data));
}

// ============================================================================
// Public API: Pair as new controller (SEND_KEY + PAIR_1W)
// Motor must be in learning mode (hold PROG on existing remote first)
// ============================================================================

bool IOHomecontrol::send_pair(uint32_t target_address) {
  if (this->radio_ == nullptr) {
    ESP_LOGE(TAG, "Radio not configured");
    return false;
  }

  // Ensure we're on the 1W channel
  this->ensure_1w_channel_();

  // Step 1: Encrypt our key using the Transfer Key
  // decrypt_1w_key_ is XOR-based (self-inverse), so it works for encryption too:
  //   encrypted = AES_ECB(TransferKey, IV_from_addr) XOR plaintext_key
  uint8_t source_addr_3b[ADDRESS_SIZE] = {
      static_cast<uint8_t>((this->source_address_ >> 16) & 0xFF),
      static_cast<uint8_t>((this->source_address_ >> 8) & 0xFF),
      static_cast<uint8_t>(this->source_address_ & 0xFF),
  };
  std::array<uint8_t, KEY_SIZE> encrypted_key{};
  decrypt_1w_key_(source_addr_3b, this->key_.data(), encrypted_key.data());

  // Step 2: Build SEND_KEY frame (CMD 0x30, NO HMAC)
  // [CB0][CB1][Target 3B][Source 3B][0x30][EncKey 16B][ManID][Data][Seq 2B][CRC 2B]
  std::array<uint8_t, SEND_KEY_MIN_SIZE> frame{};

  // L = total - 3 (excludes CtrlByte0 and CRC)
  constexpr size_t send_key_frame_len = SEND_KEY_MIN_SIZE - CTRL0_L_EXCLUDED_BYTES;            // 31 - 3 = 28
  frame[0] = (send_key_frame_len & CTRL0_LEN_MASK) | CTRL0_2W_BIT | (3 << CTRL0_ORDER_SHIFT);  // 2W mode, order=3
  frame[1] = 0x00;

  // Target address — wire format: [CB0][CB1][Target 3B][Source 3B]
  frame[2] = (target_address >> 16) & 0xFF;
  frame[3] = (target_address >> 8) & 0xFF;
  frame[4] = target_address & 0xFF;

  // Source address
  frame[5] = source_addr_3b[0];
  frame[6] = source_addr_3b[1];
  frame[7] = source_addr_3b[2];

  // Command
  frame[8] = static_cast<uint8_t>(Command::SEND_KEY);

  // Encrypted key (16 bytes)
  std::memcpy(frame.data() + FRAME_HEADER_SIZE, encrypted_key.data(), KEY_SIZE);

  // Manufacturer ID + Data byte
  frame[FRAME_HEADER_SIZE + KEY_SIZE] = MANUFACTURER_SOMFY;
  frame[FRAME_HEADER_SIZE + KEY_SIZE + 1] = SEND_KEY_DATA_BYTE;

  // Sequence number (MSB first)
  uint16_t seq = this->get_sequence_entry_(this->source_address_)->sequence;
  frame[FRAME_HEADER_SIZE + KEY_SIZE + 2] = (seq >> 8) & 0xFF;
  frame[FRAME_HEADER_SIZE + KEY_SIZE + 3] = seq & 0xFF;

  // CRC-16/KERMIT (LSB first)
  uint16_t crc = compute_crc_(frame.data(), SEND_KEY_MIN_SIZE - CRC_SIZE);
  frame[SEND_KEY_MIN_SIZE - 2] = crc & 0xFF;
  frame[SEND_KEY_MIN_SIZE - 1] = (crc >> 8) & 0xFF;

  // Transmit SEND_KEY with repeats on both 2W channels (CH3 + CH2).
  // The remote transmits pairing frames on multiple channels — the motor may only
  // listen on specific channels during learning mode.
  // Blocking delays here are acceptable — pairing is a rare manual operation.
  ESP_LOGW(TAG, "PAIRING: Sending SEND_KEY to 0x%06X (seq=%u)", target_address, seq);

  static constexpr uint32_t PAIR_CHANNELS[] = {FREQ_CH3, FREQ_CH2};
  for (uint32_t ch : PAIR_CHANNELS) {
    this->radio_->set_frequency(static_cast<float>(ch));
    this->current_freq_ = ch;
    for (uint8_t i = 0; i < this->tx_repeats_; i++) {
      if (!this->transmit_serial_(frame.data(), frame.size())) {
        ESP_LOGE(TAG, "SEND_KEY transmit failed on repeat %u", i);
        return false;
      }
      if (i < this->tx_repeats_ - 1) {
        delay(TX_REPEAT_DELAY_MS);
      }
    }
  }

  // Increment sequence for SEND_KEY
  auto *own_entry = this->get_sequence_entry_(this->source_address_);
  own_entry->sequence++;
  own_entry->pref.save(&own_entry->sequence);
  global_preferences->sync();

  // Brief pause between SEND_KEY and PAIR_1W
  delay(TX_REPEAT_DELAY_MS);

  // Step 3: Build PAIR_1W frame manually (CMD 0x2E) WITH HMAC.
  // Must use 2W bit + order=3 in ctrl0 to match what the remote sends —
  // the motor rejects pairing frames with 1W/order=0 ctrl0.
  uint8_t pair_data[1] = {0x00};
  size_t pair_total = FRAME_HEADER_SIZE + sizeof(pair_data) + HMAC_AUTH_SIZE + CRC_SIZE;
  size_t pair_frame_len = pair_total - CTRL0_L_EXCLUDED_BYTES;
  std::array<uint8_t, MAX_PACKET_SIZE> pair_frame{};

  // CtrlByte0: 2W mode, order=3 (matching remote's pairing frames)
  pair_frame[0] = (pair_frame_len & CTRL0_LEN_MASK) | CTRL0_2W_BIT | (3 << CTRL0_ORDER_SHIFT);
  pair_frame[1] = 0x00;

  // Target address
  pair_frame[2] = (target_address >> 16) & 0xFF;
  pair_frame[3] = (target_address >> 8) & 0xFF;
  pair_frame[4] = target_address & 0xFF;

  // Source address
  pair_frame[5] = source_addr_3b[0];
  pair_frame[6] = source_addr_3b[1];
  pair_frame[7] = source_addr_3b[2];

  // Command
  pair_frame[8] = static_cast<uint8_t>(Command::PAIR_1W);

  // Data
  pair_frame[FRAME_HEADER_SIZE] = 0x00;

  // HMAC over CMD + data
  uint16_t pair_seq = own_entry->sequence;
  uint8_t mac[MAC_SIZE];
  this->compute_1w_hmac_(pair_frame.data() + FRAME_HEADER_SIZE - 1, 1 + sizeof(pair_data), pair_seq, mac);

  // Sequence number (MSB first)
  size_t auth_off = FRAME_HEADER_SIZE + sizeof(pair_data);
  pair_frame[auth_off] = (pair_seq >> 8) & 0xFF;
  pair_frame[auth_off + 1] = pair_seq & 0xFF;

  // MAC
  std::memcpy(pair_frame.data() + auth_off + SEQ_SIZE, mac, MAC_SIZE);

  // CRC-16/KERMIT (LSB first)
  size_t crc_off = pair_total - CRC_SIZE;
  uint16_t pair_crc = compute_crc_(pair_frame.data(), crc_off);
  pair_frame[crc_off] = pair_crc & 0xFF;
  pair_frame[crc_off + 1] = (pair_crc >> 8) & 0xFF;

  // Log the frame
  char hex_buf[MAX_PACKET_SIZE * 2 + 1];
  format_hex_to(hex_buf, pair_frame.data(), pair_total);
  ESP_LOGI(TAG, "TX PAIR_1W frame: src=0x%06X -> target=0x%06X seq=%u len=%u", this->source_address_, target_address,
           pair_seq, static_cast<unsigned>(pair_total));
  ESP_LOGD(TAG, "  TX raw: %s", hex_buf);

  // Transmit PAIR_1W on both channels with repeats
  for (uint32_t ch : PAIR_CHANNELS) {
    this->radio_->set_frequency(static_cast<float>(ch));
    this->current_freq_ = ch;
    for (uint8_t i = 0; i < this->tx_repeats_; i++) {
      if (!this->transmit_serial_(pair_frame.data(), pair_total)) {
        ESP_LOGE(TAG, "PAIR_1W transmit failed on repeat %u", i);
        return false;
      }
      if (i < this->tx_repeats_ - 1) {
        delay(TX_REPEAT_DELAY_MS);
      }
    }
  }

  // Increment sequence for PAIR_1W
  own_entry->sequence++;
  own_entry->pref.save(&own_entry->sequence);
  global_preferences->sync();

  // Return to CH2 for normal operation
  this->radio_->set_frequency(static_cast<float>(FREQ_CH2));
  this->current_freq_ = FREQ_CH2;

  ESP_LOGW(TAG, "PAIRING: Sent SEND_KEY + PAIR_1W successfully");
  ESP_LOGW(TAG, "PAIRING: If motor jogs, pairing was successful!");

  return true;
}

// ============================================================================
// 1W key decryption from captured CMD 0x30 (SEND_KEY) frame
// Transfer Key encrypts the private key using AES-128-CFB128.
// For a single 16-byte block this reduces to: enc_key = AES_ECB(TK, IV) XOR key
// So: key = AES_ECB(TK, IV) XOR enc_key
// IV = source address repeated to fill 16 bytes
// ============================================================================

void IOHomecontrol::decrypt_1w_key_(const uint8_t *source_addr_3b, const uint8_t *enc_key_16b, uint8_t *out_key_16b) {
#ifdef USE_ESP32
  // Build IV: source address repeated to fill 16 bytes
  // Pattern for 3-byte addr [A B C]: [A B C A B C A B C A B C A B C A]
  std::array<uint8_t, KEY_SIZE> iv{};
  for (size_t i = 0; i < KEY_SIZE - 1; i++) {
    iv[i] = source_addr_3b[i % ADDRESS_SIZE];
  }
  iv[KEY_SIZE - 1] = source_addr_3b[0];

  // AES-128-ECB encrypt IV with Transfer Key to get keystream
  std::array<uint8_t, KEY_SIZE> keystream{};
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);
  mbedtls_aes_setkey_enc(&ctx, TRANSFER_KEY, KEY_SIZE * 8);
  mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, iv.data(), keystream.data());
  mbedtls_aes_free(&ctx);

  // XOR to recover plaintext key
  for (size_t i = 0; i < KEY_SIZE; i++) {
    out_key_16b[i] = keystream[i] ^ enc_key_16b[i];
  }
#else
  ESP_LOGE(TAG, "Key decryption requires ESP32 (mbedtls)");
  std::memset(out_key_16b, 0, KEY_SIZE);
#endif
}

// ============================================================================
// Packet reception and parsing
// ============================================================================

void IOHomecontrol::parse_frame_(const uint8_t *packet, size_t packet_size) {
  if (packet_size < MIN_FRAME_SIZE) {
    ESP_LOGW(TAG, "Packet too short: %u bytes", static_cast<unsigned>(packet_size));
    return;
  }

  // Extract actual frame size from CtrlByte0 length field.
  // Per spec: L (bits 4:0) = total_bytes - 3 (excludes CtrlByte0 and 2-byte CRC)
  uint8_t ctrl0 = packet[0];
  size_t actual_size = (ctrl0 & CTRL0_LEN_MASK) + CTRL0_L_EXCLUDED_BYTES;

  if (actual_size < MIN_FRAME_SIZE || actual_size > packet_size) {
    ESP_LOGW(TAG, "Invalid frame length (ctrl0=0x%02X, actual=%u, packet=%u)", ctrl0,
             static_cast<unsigned>(actual_size), static_cast<unsigned>(packet_size));
    return;
  }

  // Log raw packet with signal quality (RSSI/LQI sampled at sync word detection)
  char raw_dbg[MAX_PACKET_SIZE * 2 + 1];
  format_hex_to(raw_dbg, packet, std::min(packet_size, MAX_PACKET_SIZE));
  ESP_LOGD(TAG, "parse_frame: packet_size=%u ctrl0=0x%02X actual_size=%u RSSI=%.1fdBm LQI=%u raw=%s",
           static_cast<unsigned>(packet_size), ctrl0, static_cast<unsigned>(actual_size), this->rx_rssi_, this->rx_lqi_,
           raw_dbg);

  // Verify CRC-16/X.25 using actual frame size (not the radio-delivered packet size)
  size_t crc_offset = actual_size - CRC_SIZE;
  uint16_t computed_crc = compute_crc_(packet, crc_offset);
  uint16_t received_crc = packet[crc_offset] | (packet[crc_offset + 1] << 8);
  if (computed_crc != received_crc) {
    ESP_LOGW(TAG, "CRC mismatch (computed=0x%04X received=0x%04X) RSSI=%.1fdBm", computed_crc, received_crc,
             this->rx_rssi_);
    return;
  }

  // Parse control bytes
  uint8_t ctrl1 = packet[1];

  uint8_t order = (ctrl0 >> CTRL0_ORDER_SHIFT) & CTRL0_ORDER_MASK;  // Logging only; multi-frame reassembly not yet impl
  bool is_1w = !(ctrl0 & CTRL0_2W_BIT);

  bool use_beacon = ctrl1 & CTRL1_BEACON_BIT;
  bool ack_capable = ctrl1 & CTRL1_ACK_BIT;

  // Addresses (3 bytes each, big-endian): [CB0][CB1][Target 3B][Source 3B][CMD]...
  uint32_t target = ((uint32_t) packet[2] << 16) | ((uint32_t) packet[3] << 8) | packet[4];
  uint32_t source = ((uint32_t) packet[5] << 16) | ((uint32_t) packet[6] << 8) | packet[7];

  // Command — cast to enum early for clean comparisons
  auto cmd = static_cast<Command>(packet[8]);

  // Data payload bounds: everything between CMD and CRC (using actual frame size)
  size_t data_start = FRAME_HEADER_SIZE;
  size_t data_end = crc_offset;

  // SEND_KEY has NO HMAC - payload: [EncKey 16B][ManID 1B][Data 1B][Seq 2B]
  // Other 1W frames have: [payload][Seq 2B][HMAC 6B]
  bool has_hmac = is_1w && cmd != Command::SEND_KEY;
  if (has_hmac && data_end - data_start >= HMAC_AUTH_SIZE) {
    data_end -= HMAC_AUTH_SIZE;
  }

  // Determine channel name from current frequency
  const char *ch_name = "CH2";
  if (this->current_freq_ == FREQ_CH1)
    ch_name = "CH1";
  else if (this->current_freq_ == FREQ_CH3)
    ch_name = "CH3";

  bool is_own_frame = (source == this->source_address_);
  ESP_LOGI(TAG, "RX %s frame on %s (%.2f MHz): order=%u size=%u beacon=%s ack=%s%s", is_1w ? "1W" : "2W", ch_name,
           (float) this->current_freq_ / 1e6f, order, static_cast<unsigned>(actual_size), use_beacon ? "yes" : "no",
           ack_capable ? "yes" : "no", is_own_frame ? " [OWN-LOOPBACK]" : "");
  ESP_LOGI(TAG, "  Target: 0x%06X | Source: 0x%06X", target, source);
  ESP_LOGI(TAG, "  CMD: 0x%02X (%s)", static_cast<uint8_t>(cmd), get_command_name_(cmd));

  // ---- Pairing mode: prominent address and key logging ----
  if (this->pairing_mode_) {
    ESP_LOGW(TAG, "  ============ DISCOVERED ============");
    ESP_LOGW(TAG, "  Remote/Controller address: 0x%06" PRIX32, source);
    if (target != ADDR_BROADCAST) {
      ESP_LOGW(TAG, "  Actuator/Motor address:    0x%06" PRIX32, target);
    } else {
      ESP_LOGW(TAG, "  Target: BROADCAST (0x%06" PRIX32 ")", target);
    }

    // SEND_KEY - decrypt and log the private key
    // Frame: [CB0][CB1][Tgt 3][Src 3][0x30][EncKey 16][ManID 1][Data 1][Seq 2][CRC 2]
    if (cmd == Command::SEND_KEY && packet_size >= SEND_KEY_MIN_SIZE) {
      const uint8_t *enc_key = packet + FRAME_HEADER_SIZE;
      uint8_t manufacturer_id = packet[FRAME_HEADER_SIZE + KEY_SIZE];
      uint8_t data_byte = packet[FRAME_HEADER_SIZE + KEY_SIZE + 1];
      uint16_t seq = (packet[FRAME_HEADER_SIZE + KEY_SIZE + 2] << 8) | packet[FRAME_HEADER_SIZE + KEY_SIZE + 3];

      char enc_hex[KEY_SIZE * 2 + 1];
      format_hex_to(enc_hex, enc_key, KEY_SIZE);
      ESP_LOGW(TAG, "  !! SEND_KEY captured !!");
      ESP_LOGW(TAG, "  Encrypted key: %s", enc_hex);
      ESP_LOGW(TAG, "  Manufacturer: 0x%02X  Data: 0x%02X  Seq: %u", manufacturer_id, data_byte, seq);

      // Decrypt the key using source address from frame (offset 5 = after CB0+CB1+Target)
      std::array<uint8_t, KEY_SIZE> decrypted_key{};
      decrypt_1w_key_(packet + 5, enc_key, decrypted_key.data());

      char key_hex[KEY_SIZE * 2 + 1];
      format_hex_to(key_hex, decrypted_key.data(), KEY_SIZE);

      ESP_LOGW(TAG, "  ========================================");
      ESP_LOGW(TAG, "  DECRYPTED PRIVATE KEY: %s", key_hex);
      ESP_LOGW(TAG, "  ========================================");
      ESP_LOGW(TAG, "  Use these values in your YAML config:");
      ESP_LOGW(TAG, "    source_address: 0x%06" PRIX32, source);
      ESP_LOGW(TAG, "    key: \"%s\"", key_hex);
      ESP_LOGW(TAG, "  ========================================");
    }

    // PAIR_1W - log pairing confirmation
    if (cmd == Command::PAIR_1W) {
      ESP_LOGW(TAG, "  >> 1W_PAIR confirmation from 0x%06" PRIX32, source);
      ESP_LOGW(TAG, "  >> This controller is now paired with the motor");
    }

    // REMOVE_CTRL - log controller removal
    if (cmd == Command::REMOVE_CTRL) {
      ESP_LOGW(TAG, "  >> REMOVE_CTRL from 0x%06" PRIX32, source);
      ESP_LOGW(TAG, "  >> Controller key removed from motor");
    }

    // EXECUTE - log as address discovery
    if (cmd == Command::EXECUTE && data_end - data_start >= 4) {
      uint16_t main_param = (packet[data_start + 2] << 8) | packet[data_start + 3];
      ESP_LOGW(TAG, "  >> EXECUTE %s (param=0x%04X)", get_param_name_(main_param), main_param);
      ESP_LOGW(TAG, "  >> Cover config:  address: 0x%06" PRIX32, source);
    }

    ESP_LOGW(TAG, "  ======================================");
  }

  // ---- Normal mode: detailed payload logging ----
  if (!this->pairing_mode_ && data_end > data_start) {
    size_t data_len = data_end - data_start;
    char hex_buf[MAX_PACKET_SIZE * 2 + 1];
    size_t hex_len = std::min(data_len, MAX_PACKET_SIZE);
    format_hex_to(hex_buf, packet + data_start, hex_len);
    ESP_LOGI(TAG, "  Data: %s", hex_buf);

    // Parse EXECUTE command details
    if (cmd == Command::EXECUTE && data_len >= 4) {
      uint8_t originator = packet[data_start];
      uint8_t acei = packet[data_start + 1];
      uint16_t main_param = (packet[data_start + 2] << 8) | packet[data_start + 3];

      ESP_LOGI(TAG, "  >> EXECUTE: originator=0x%02X acei=0x%02X param=0x%04X (%s)", originator, acei, main_param,
               get_param_name_(main_param));

      if (main_param > 0 && main_param <= static_cast<uint16_t>(MainParam::CLOSE) &&
          main_param != static_cast<uint16_t>(MainParam::STOP)) {
        float pct = (float) main_param / (float) static_cast<uint16_t>(MainParam::CLOSE) * 100.0f;
        ESP_LOGI(TAG, "  >> Position: %.1f%%", pct);
      }
    }
  }

#ifdef USE_IO_HOMECONTROL_COVER
  // Update matching cover entities from any sniffed EXECUTE command (pairing or normal mode)
  if (cmd == Command::EXECUTE && data_end - data_start >= 4) {
    uint16_t main_param = (packet[data_start + 2] << 8) | packet[data_start + 3];
    for (auto *c : this->covers_) {
      if (c->has_address(source)) {
        c->update_from_sniffed(source, main_param);
      }
    }
  }
#endif

  // Log 1W authentication data and auto-track sequence number.
  // Note: HMAC is not verified on RX — passive sniffing only needs sequence tracking
  // to prevent replay attacks. Verification would require knowing every sender's key.
  if (has_hmac) {
    uint16_t seq = (packet[data_end] << 8) | packet[data_end + 1];
    char mac_hex[MAC_SIZE * 2 + 1];
    format_hex_to(mac_hex, packet + data_end + SEQ_SIZE, MAC_SIZE);
    ESP_LOGI(TAG, "  1W Auth: seq=%u MAC=%s", seq, mac_hex);

    // Persist sequence number for all sniffed source addresses.
    // Handle uint16_t wrap-around: if the stored sequence is far ahead of the received one,
    // the remote likely wrapped from 65535 to 0. Accept the new value in that case.
    auto *seq_entry = this->get_sequence_entry_(source);
    uint16_t old_seq = seq_entry->sequence;
    bool seq_advanced = (seq >= seq_entry->sequence) || (seq_entry->sequence > SEQ_WRAP_HIGH && seq < SEQ_WRAP_LOW);
    if (seq_advanced) {
      seq_entry->sequence = seq + 1;
      seq_entry->pref.save(&seq_entry->sequence);
      global_preferences->sync();
      ESP_LOGI(TAG, "  Seq UPDATE 0x%06X: rx=%u old=%u -> next=%u", source, seq, old_seq, seq_entry->sequence);
    } else {
      ESP_LOGW(TAG, "  Seq REJECTED 0x%06X: rx=%u <= stored=%u (replay?)", source, seq, old_seq);
    }
  } else {
    // No HMAC — can't read actual sequence. If source is already tracked, increment by 1
    // so we know at least one more command was sent (guards against replaying old 1W frames).
    // Only do this for already-known sources — do not create new entries for unknown devices.
    for (auto &entry : this->sequence_entries_) {
      if (entry.address == source && entry.sequence > 0) {
        uint16_t old_seq = entry.sequence;
        entry.sequence++;
        entry.pref.save(&entry.sequence);
        global_preferences->sync();
        ESP_LOGD(TAG, "  Seq BUMP (no-HMAC) 0x%06X: %u -> %u", source, old_seq, entry.sequence);
        break;
      }
    }
  }

  // Log raw hex at debug level (only the actual frame, not radio noise)
  char raw_hex[MAX_PACKET_SIZE * 2 + 1];
  size_t raw_len = std::min(actual_size, MAX_PACKET_SIZE);
  format_hex_to(raw_hex, packet, raw_len);
  ESP_LOGD(TAG, "  Raw: %s", raw_hex);
}

const char *IOHomecontrol::get_command_name_(Command cmd) {
  switch (cmd) {
    case Command::EXECUTE:
      return "EXECUTE";
    case Command::ACTIVATE_MODE:
      return "ACTIVATE_MODE";
    case Command::PRIVATE_CMD:
      return "PRIVATE_CMD";
    case Command::PRIVATE_ANS:
      return "PRIVATE_ANS";
    case Command::WRITE_PRIVATE:
      return "WRITE_PRIVATE";
    case Command::PRIVATE_ACK:
      return "PRIVATE_ACK";
    case Command::DISCOVER:
      return "DISCOVER";
    case Command::DISCOVER_ANS:
      return "DISCOVER_ANS";
    case Command::DISCOVER_CONFIRM:
      return "DISCOVER_CONFIRM";
    case Command::DISCOVER_CONF_ACK:
      return "DISCOVER_CONF_ACK";
    case Command::PAIR_1W:
      return "1W_PAIR";
    case Command::SEND_KEY:
      return "SEND_KEY";
    case Command::ASK_CHALLENGE:
      return "ASK_CHALLENGE";
    case Command::KEY_TRANSFER:
      return "KEY_TRANSFER";
    case Command::KEY_TRANSFER_ACK:
      return "KEY_TRANSFER_ACK";
    case Command::LAUNCH_KEY_XFER:
      return "LAUNCH_KEY_XFER";
    case Command::REMOVE_CTRL:
      return "REMOVE_CTRL";
    case Command::CHALLENGE_REQ:
      return "CHALLENGE_REQ";
    case Command::CHALLENGE_ANS:
      return "CHALLENGE_ANS";
    case Command::GET_NAME:
      return "GET_NAME";
    case Command::GET_NAME_ANS:
      return "GET_NAME_ANS";
    default:
      return "UNKNOWN";
  }
}

const char *IOHomecontrol::get_param_name_(uint16_t param) {
  switch (static_cast<MainParam>(param)) {
    case MainParam::OPEN:
      return "OPEN";
    case MainParam::CLOSE:
      return "CLOSE";
    case MainParam::STOP:
      return "STOP";
    case MainParam::MY_POS:
      return "MY_POS";
    default:
      if (param > static_cast<uint16_t>(MainParam::OPEN) && param < static_cast<uint16_t>(MainParam::CLOSE))
        return "POSITION";
      return "UNKNOWN";
  }
}

}  // namespace esphome::io_homecontrol
