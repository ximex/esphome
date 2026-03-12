#include "milight_hub.h"
#include "light/milight_light.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <cstring>

namespace esphome::milight {

static const char *const TAG = "milight";

// ---- NRF24 register access ----

void MiLightHub::write_register_(uint8_t reg, uint8_t value) {
  this->enable();
  this->write_byte(nrf24::CMD_W_REGISTER | reg);
  this->write_byte(value);
  this->disable();
}

void MiLightHub::write_register_(uint8_t reg, const uint8_t *data, uint8_t length) {
  this->enable();
  this->write_byte(nrf24::CMD_W_REGISTER | reg);
  this->write_array(data, length);
  this->disable();
}

uint8_t MiLightHub::read_register_(uint8_t reg) {
  this->enable();
  this->write_byte(reg);
  uint8_t value = this->read_byte();
  this->disable();
  return value;
}

void MiLightHub::write_payload_(const uint8_t *data, uint8_t length) {
  this->enable();
  this->write_byte(nrf24::CMD_W_TX_PAYLOAD);
  this->write_array(data, length);
  this->disable();
}

void MiLightHub::flush_tx_() {
  this->enable();
  this->write_byte(nrf24::CMD_FLUSH_TX);
  this->disable();
}

void MiLightHub::flush_rx_() {
  this->enable();
  this->write_byte(nrf24::CMD_FLUSH_RX);
  this->disable();
}

void MiLightHub::read_payload_(uint8_t *data, uint8_t length) {
  this->enable();
  this->write_byte(nrf24::CMD_R_RX_PAYLOAD);
  this->read_array(data, length);
  this->disable();
}

void MiLightHub::pulse_ce_() {
  this->ce_pin_->digital_write(true);
  delayMicroseconds(15);  // Minimum CE high pulse width = 10µs (datasheet), 15µs with margin
  this->ce_pin_->digital_write(false);
}

void MiLightHub::wait_tx_complete_() {
  // Poll for TX_DS or MAX_RT with timeout.
  // At 1Mbps: 130µs settling + ~144µs on-air = ~274µs typical.
  // Each SPI read takes ~3µs, so 200 iterations ≈ 600µs timeout.
  for (uint16_t i = 0; i < 200; i++) {
    uint8_t status = this->read_register_(nrf24::REG_STATUS);
    if (status & (nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT)) {
      this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT);
      return;
    }
  }
  // Timeout: flush stale data and clear flags
  this->flush_tx_();
  this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT);
}

// ---- Setup ----

void MiLightHub::setup() {
  ESP_LOGD(TAG, "Setting up MiLight hub...");
  this->spi_setup();

  this->ce_pin_->setup();
  this->ce_pin_->digital_write(false);

  delay(5);  // NRF24L01+ power-on settling time (Tpd2stby = 1.5ms, margin added)

  // Reset: toggle CSN
  this->enable();
  this->disable();
  delay(5);

  // Configure NRF24L01+
  this->write_register_(nrf24::REG_EN_AA, 0x00);       // Disable auto-acknowledge
  this->write_register_(nrf24::REG_SETUP_RETR, 0x00);  // Disable auto-retransmit
  this->write_register_(nrf24::REG_EN_RXADDR, 0x01);   // Enable RX pipe 0
  this->write_register_(nrf24::REG_SETUP_AW, 0x03);    // Address width: 5 bytes
  this->write_register_(nrf24::REG_RF_SETUP, nrf24::RF_SETUP_1MBPS | static_cast<uint8_t>(this->rf24_power_level_));
  this->write_register_(nrf24::REG_FEATURE, 0x00);  // Disable dynamic payload features
  this->write_register_(nrf24::REG_DYNPD, 0x00);    // Disable dynamic payload on all pipes
  this->write_register_(nrf24::REG_CONFIG, nrf24::CONFIG_PWR_UP | nrf24::CONFIG_PRIM_TX);

  this->flush_tx_();
  this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT);

  delay(2);  // Standby-I to TX/RX mode transition (1.5ms per datasheet)

  // Verify SPI communication by reading back registers
  uint8_t setup_aw = this->read_register_(nrf24::REG_SETUP_AW);
  uint8_t rf_setup = this->read_register_(nrf24::REG_RF_SETUP);
  uint8_t config_reg = this->read_register_(nrf24::REG_CONFIG);
  ESP_LOGI(TAG, "NRF24 readback: SETUP_AW=0x%02X RF_SETUP=0x%02X CONFIG=0x%02X", setup_aw, rf_setup, config_reg);
  if (setup_aw != 0x03) {
    ESP_LOGE(TAG, "NRF24L01+ not responding! Expected SETUP_AW=0x03, got 0x%02X. Check SPI wiring.", setup_aw);
    this->mark_failed();
    return;
  }
}

void MiLightHub::dump_config() {
  ESP_LOGCONFIG(TAG, "MiLight Hub:");
  LOG_PIN("  CE Pin: ", this->ce_pin_);
  ESP_LOGCONFIG(TAG, "  Packet Repeats: %u", this->packet_repeats_);
  const char *power_str = "MAX";
  switch (this->rf24_power_level_) {
    case Rf24PowerLevel::MIN:
      power_str = "MIN (-18 dBm)";
      break;
    case Rf24PowerLevel::LOW:
      power_str = "LOW (-12 dBm)";
      break;
    case Rf24PowerLevel::HIGH:
      power_str = "HIGH (-6 dBm)";
      break;
    case Rf24PowerLevel::MAX:
      power_str = "MAX (0 dBm)";
      break;
  }
  ESP_LOGCONFIG(TAG, "  RF24 Power Level: %s", power_str);
  ESP_LOGCONFIG(TAG, "  Packets Per Loop: %u", this->packet_repeats_per_loop_);
  ESP_LOGCONFIG(TAG, "  Throttle: threshold=%ums sensitivity=%u minimum=%u", this->throttle_threshold_ms_,
                this->throttle_sensitivity_, this->throttle_minimum_);
  ESP_LOGCONFIG(TAG, "  Listen: %s", YESNO(this->listen_enabled_));
  ESP_LOGCONFIG(TAG, "  RX Slots: %u", this->rx_slot_count_);
  ESP_LOGCONFIG(TAG, "  Registered Lights: %u", this->registered_light_count_);
}

// ---- Light registration for RX ----

void MiLightHub::register_light(MiLightLight *light, uint16_t device_id, uint8_t group_id, RemoteType type) {
  if (this->registered_light_count_ >= MAX_REGISTERED_LIGHTS) {
    ESP_LOGW(TAG, "Too many lights registered for RX");
    return;
  }

  uint8_t config_idx = get_radio_config_index(type);

  LightRegistration &reg = this->registered_lights_[this->registered_light_count_++];
  reg.light = light;
  reg.device_id = device_id;
  reg.group_id = group_id;
  reg.radio_config_index = config_idx;
  reg.remote_type = type;

  // Add RX slot if not already present for this radio config
  bool found = false;
  for (uint8_t i = 0; i < this->rx_slot_count_; i++) {
    if (this->rx_slots_[i].radio_config_index == config_idx) {
      found = true;
      break;
    }
  }
  if (!found && this->rx_slot_count_ < MAX_RX_CONFIGS) {
    const RadioConfig &config = get_radio_config(type);
    RxListenSlot &slot = this->rx_slots_[this->rx_slot_count_++];
    compute_nrf24_address(config, slot.nrf24_address.data());
    std::copy(config.channels, config.channels + 3, slot.channels.begin());
    slot.packet_length = config.packet_length;
    slot.radio_config_index = config_idx;
  }
}

// ---- Configure NRF24 address ----

void MiLightHub::configure_address_(const uint8_t *address) {
  // Skip if address hasn't changed
  if (this->tx_address_set_ && memcmp(this->tx_current_address_.data(), address, 5) == 0) {
    return;
  }

  this->write_register_(nrf24::REG_TX_ADDR, address, 5);
  this->write_register_(nrf24::REG_RX_ADDR_P0, address, 5);
  std::copy(address, address + 5, this->tx_current_address_.begin());
  this->tx_address_set_ = true;
}

// ---- Transmit one packet on current channel from front of queue ----

void MiLightHub::transmit_packet_() {
  if (this->tx_queue_count_ == 0)
    return;

  TxEntry &entry = this->tx_queue_[this->tx_queue_head_];

  // Configure address if needed (first transmission of this entry)
  if (entry.current_channel == 0) {
    this->configure_address_(entry.nrf24_address.data());
  }

  // Set RF channel
  uint8_t channel = entry.channels[entry.current_channel];
  this->write_register_(nrf24::REG_RF_CH, 2 + channel);

  // Clear status, flush, write payload, transmit
  this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT);
  this->flush_tx_();
  this->write_payload_(entry.packet.data(), entry.packet_length);
  this->pulse_ce_();

  // Wait for packet to finish transmitting before returning
  this->wait_tx_complete_();

  // Advance channel
  entry.current_channel++;
  if (entry.current_channel >= entry.channel_count) {
    entry.current_channel = 0;
    entry.repeats_remaining--;
    if (entry.repeats_remaining == 0) {
      // Pop front of queue
      this->tx_queue_head_ = (this->tx_queue_head_ + 1) % TX_QUEUE_SIZE;
      this->tx_queue_count_--;
    }
  }
}

// ---- Enqueue a formatted packet ----

void MiLightHub::enqueue_(const LightCommand &cmd, uint16_t repeats) {
  if (this->tx_queue_count_ >= TX_QUEUE_SIZE) {
    ESP_LOGW(TAG, "TX queue full, dropping command");
    return;
  }

  const RadioConfig &config = get_radio_config(cmd.remote_type);

  // Format the packet
  std::array<uint8_t, MAX_PACKET_SIZE> raw_packet{};
  uint8_t raw_len = format_packet(cmd, raw_packet.data(), this->sequence_num_++);

  // Build PL1167 frame: [length] [payload...] [crc_lo] [crc_hi]
  std::array<uint8_t, MAX_PACKET_SIZE + 3> frame{};
  frame[0] = raw_len;
  for (uint8_t i = 0; i < raw_len; i++) {
    frame[1 + i] = raw_packet[i];
  }

  // Compute CRC over frame (length byte + payload)
  uint16_t crc = pl1167_crc(frame.data(), 1 + raw_len);
  frame[1 + raw_len] = crc & 0xFF;
  frame[2 + raw_len] = (crc >> 8) & 0xFF;

  uint8_t total_len = 1 + raw_len + 2;

  // Bit-reverse all bytes for PL1167 over NRF24
  for (uint8_t i = 0; i < total_len; i++) {
    frame[i] = reverse_bits(frame[i]);
  }

  // Compute NRF24 address
  uint8_t address[5];
  compute_nrf24_address(config, address);

  // Add to queue
  uint8_t tail = (this->tx_queue_head_ + this->tx_queue_count_) % TX_QUEUE_SIZE;
  TxEntry &entry = this->tx_queue_[tail];
  std::copy(frame.begin(), frame.begin() + total_len, entry.packet.begin());
  entry.packet_length = total_len;
  std::copy(config.channels, config.channels + 3, entry.channels.begin());
  std::copy(address, address + 5, entry.nrf24_address.begin());
  entry.channel_count = 3;
  entry.current_channel = 0;
  entry.repeats_remaining = repeats;

  this->tx_queue_count_++;
}

// ---- Throttle logic ----

uint16_t MiLightHub::throttled_repeats_() {
  uint32_t now = App.get_loop_component_start_time();
  uint32_t elapsed = now - this->last_command_ms_;
  this->last_command_ms_ = now;

  if (this->throttle_sensitivity_ == 0 || elapsed >= this->throttle_threshold_ms_) {
    return this->packet_repeats_;
  }

  // Scale down repeats: the faster commands arrive, the fewer repeats
  // sensitivity controls how aggressively we reduce (higher = more aggressive)
  uint32_t reduction =
      (this->throttle_threshold_ms_ - elapsed) * this->throttle_sensitivity_ / this->throttle_threshold_ms_;
  uint16_t effective = this->packet_repeats_;
  if (reduction < effective) {
    effective -= reduction;
  }
  if (effective < this->throttle_minimum_) {
    effective = this->throttle_minimum_;
  }
  if (effective != this->packet_repeats_) {
    ESP_LOGD(TAG, "Throttled repeats: %u -> %u (elapsed=%ums)", this->packet_repeats_, effective, elapsed);
  }
  return effective;
}

// ---- Send command (public API) ----

void MiLightHub::send_command(const LightCommand &cmd) { this->enqueue_(cmd, this->throttled_repeats_()); }

void MiLightHub::send_command(const LightCommand &cmd, uint16_t repeats) { this->enqueue_(cmd, repeats); }

void MiLightHub::send_pair(uint16_t device_id, uint8_t group_id, RemoteType type) {
  LightCommand cmd{};
  cmd.device_id = device_id;
  cmd.group_id = group_id;
  cmd.remote_type = type;
  cmd.pair = true;
  // Spec requires 5 separate pair commands (each with its own sequence number)
  for (int i = 0; i < 5; i++) {
    this->enqueue_(cmd, this->packet_repeats_);
  }
}

void MiLightHub::send_unpair(uint16_t device_id, uint8_t group_id, RemoteType type) {
  // RGBW requires ON command before unpair
  if (type == RemoteType::RGBW) {
    LightCommand on_cmd{};
    on_cmd.device_id = device_id;
    on_cmd.group_id = group_id;
    on_cmd.remote_type = type;
    on_cmd.turn_on = true;
    this->enqueue_(on_cmd, this->packet_repeats_);
  }

  LightCommand cmd{};
  cmd.device_id = device_id;
  cmd.group_id = group_id;
  cmd.remote_type = type;
  cmd.unpair = true;
  // Spec requires 5 separate unpair commands (each with its own sequence number)
  for (int i = 0; i < 5; i++) {
    this->enqueue_(cmd, this->packet_repeats_);
  }
}

// ---- RX mode management ----

void MiLightHub::enter_rx_mode_() {
  if (this->rx_slot_count_ == 0)
    return;

  this->ce_pin_->digital_write(false);

  // Configure for current RX slot and channel
  const RxListenSlot &slot = this->rx_slots_[this->rx_slot_index_];

  this->write_register_(nrf24::REG_RX_ADDR_P0, slot.nrf24_address.data(), 5);
  this->write_register_(nrf24::REG_RF_CH, 2 + slot.channels[this->rx_channel_index_]);
  // Frame size: 1 (length) + payload + 2 (CRC)
  this->write_register_(nrf24::REG_RX_PW_P0, 1 + slot.packet_length + 2);

  this->flush_rx_();
  this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_RX_DR | nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT);

  // Switch to RX mode
  this->write_register_(nrf24::REG_CONFIG, nrf24::CONFIG_PWR_UP | nrf24::CONFIG_PRIM_RX);

  // CE high to enter RX
  this->ce_pin_->digital_write(true);

  this->rx_mode_active_ = true;
  this->rx_last_switch_ms_ = App.get_loop_component_start_time();

  // Read back config register to verify RX mode
  uint8_t config_reg = this->read_register_(nrf24::REG_CONFIG);
  ESP_LOGD(TAG, "Entered RX mode: CONFIG=0x%02X slot=%u ch=%u (rf_ch=%u)", config_reg, this->rx_slot_index_,
           this->rx_channel_index_, slot.channels[this->rx_channel_index_]);
}

void MiLightHub::exit_rx_mode_() {
  this->ce_pin_->digital_write(false);

  // Switch back to TX mode
  this->write_register_(nrf24::REG_CONFIG, nrf24::CONFIG_PWR_UP | nrf24::CONFIG_PRIM_TX);
  this->flush_rx_();

  this->rx_mode_active_ = false;
  this->tx_address_set_ = false;  // force address re-setup on next TX
}

void MiLightHub::switch_rx_channel_() {
  this->ce_pin_->digital_write(false);

  // Advance channel, then slot
  this->rx_channel_index_++;
  if (this->rx_channel_index_ >= 3) {
    this->rx_channel_index_ = 0;
    this->rx_slot_index_ = (this->rx_slot_index_ + 1) % this->rx_slot_count_;

    // Slot changed: reconfigure address and payload width
    const RxListenSlot &slot = this->rx_slots_[this->rx_slot_index_];
    this->write_register_(nrf24::REG_RX_ADDR_P0, slot.nrf24_address.data(), 5);
    this->write_register_(nrf24::REG_RX_PW_P0, 1 + slot.packet_length + 2);
  }

  const RxListenSlot &slot = this->rx_slots_[this->rx_slot_index_];
  this->write_register_(nrf24::REG_RF_CH, 2 + slot.channels[this->rx_channel_index_]);

  this->flush_rx_();
  this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_RX_DR | nrf24::STATUS_TX_DS | nrf24::STATUS_MAX_RT);

  this->ce_pin_->digital_write(true);
  this->rx_last_switch_ms_ = App.get_loop_component_start_time();
}

void MiLightHub::check_rx_packet_() {
  uint8_t status = this->read_register_(nrf24::REG_STATUS);
  if (!(status & nrf24::STATUS_RX_DR))
    return;

  const RxListenSlot &slot = this->rx_slots_[this->rx_slot_index_];
  uint8_t frame_size = 1 + slot.packet_length + 2;

  ESP_LOGV(TAG, "RX_DR on slot %u ch %u (config_idx=%u)", this->rx_slot_index_, this->rx_channel_index_,
           slot.radio_config_index);

  // Drain all packets from the 3-level RX FIFO
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    // Read the frame
    uint8_t frame[MAX_RX_FRAME_SIZE];
    this->read_payload_(frame, frame_size);

    // Clear RX_DR flag
    this->write_register_(nrf24::REG_STATUS, nrf24::STATUS_RX_DR);

    // Bit-reverse all bytes (PL1167 uses opposite bit order)
    for (uint8_t i = 0; i < frame_size; i++) {
      frame[i] = reverse_bits(frame[i]);
    }

    // Validate: first byte should be the payload length
    if (frame[0] != slot.packet_length) {
      ESP_LOGD(TAG, "RX frame length mismatch: got %u, expected %u", frame[0], slot.packet_length);
    } else {
      // Validate PL1167 CRC
      uint16_t crc = pl1167_crc(frame, 1 + slot.packet_length);
      uint16_t received_crc = frame[1 + slot.packet_length] | (frame[2 + slot.packet_length] << 8);
      if (crc != received_crc) {
        ESP_LOGD(TAG, "RX CRC mismatch: computed=0x%04X received=0x%04X", crc, received_crc);
      } else {
        // Extract payload (skip length byte)
        this->dispatch_rx_packet_(frame + 1, slot.packet_length, slot.radio_config_index);
      }
    }

    // Check if FIFO has more packets
    status = this->read_register_(nrf24::REG_STATUS);
    if (!(status & nrf24::STATUS_RX_DR))
      break;
  }
}

bool MiLightHub::is_rx_duplicate_(uint16_t device_id, uint8_t sequence, uint8_t radio_config_index) {
  for (uint8_t i = 0; i < RX_DEDUP_SIZE; i++) {
    if (this->rx_dedup_[i].device_id == device_id && this->rx_dedup_[i].sequence == sequence &&
        this->rx_dedup_[i].radio_config_index == radio_config_index) {
      return true;
    }
  }
  // Add to ring buffer
  RxDedup &entry = this->rx_dedup_[this->rx_dedup_index_];
  entry.device_id = device_id;
  entry.sequence = sequence;
  entry.radio_config_index = radio_config_index;
  this->rx_dedup_index_ = (this->rx_dedup_index_ + 1) % RX_DEDUP_SIZE;
  return false;
}

void MiLightHub::dispatch_rx_packet_(const uint8_t *payload, uint8_t length, uint8_t radio_config_index) {
  ESP_LOGV(TAG, "RX raw [%u]: %02X %02X %02X %02X %02X %02X %02X %02X %02X", length, payload[0],
           length > 1 ? payload[1] : 0, length > 2 ? payload[2] : 0, length > 3 ? payload[3] : 0,
           length > 4 ? payload[4] : 0, length > 5 ? payload[5] : 0, length > 6 ? payload[6] : 0,
           length > 7 ? payload[7] : 0, length > 8 ? payload[8] : 0);

  ReceivedCommand rx{};
  if (!decode_packet(payload, length, radio_config_index, rx)) {
    ESP_LOGD(TAG, "RX decode failed for config_idx=%u", radio_config_index);
    return;
  }

  // Deduplicate
  if (this->is_rx_duplicate_(rx.device_id, rx.sequence, radio_config_index)) {
    return;
  }

  // Build decoded command description
  const char *action = "unknown";
  if (rx.is_on)
    action = "ON";
  else if (rx.is_off)
    action = "OFF";
  else if (rx.night_mode)
    action = "night_mode";
  else if (rx.white_mode)
    action = "white_mode";
  else if (rx.has_effect)
    action = "effect";
  else if (rx.effect_speed_up)
    action = "speed_up";
  else if (rx.effect_speed_down)
    action = "speed_down";
  else if (rx.has_brightness)
    action = "brightness";
  else if (rx.has_color_temp)
    action = "color_temp";
  else if (rx.has_hue)
    action = "color";
  else if (rx.step_up_brightness)
    action = "step_bright_up";
  else if (rx.step_down_brightness)
    action = "step_bright_down";
  else if (rx.step_up_temp)
    action = "step_temp_up";
  else if (rx.step_down_temp)
    action = "step_temp_down";

  ESP_LOGI(TAG, "RX: type=%s dev=0x%04X group=%u seq=%u cmd=%s", remote_type_to_string(rx.remote_type), rx.device_id,
           rx.group_id, rx.sequence, action);

  // Sync TX sequence counter with physical remote to avoid sequence conflicts.
  // Bulbs may ignore packets with sequence numbers they've already seen.
  for (uint8_t i = 0; i < this->registered_light_count_; i++) {
    if (this->registered_lights_[i].device_id == rx.device_id) {
      // Jump ahead of the remote's sequence number
      uint8_t next_seq = rx.sequence + 1;
      // Use modular arithmetic comparison to handle wraparound
      if (static_cast<int8_t>(next_seq - this->sequence_num_) > 0) {
        ESP_LOGD(TAG, "Syncing TX seq %u -> %u from RX", this->sequence_num_, next_seq);
        this->sequence_num_ = next_seq;
      }
      break;
    }
  }

  // Dispatch to matching registered lights
  for (uint8_t i = 0; i < this->registered_light_count_; i++) {
    const LightRegistration &reg = this->registered_lights_[i];
    if (reg.radio_config_index != radio_config_index)
      continue;
    // For V2 types sharing radio config index 2, also check remote_type
    if (reg.remote_type != rx.remote_type)
      continue;
    if (reg.device_id != rx.device_id)
      continue;
    // group_id 0 in received command = all groups; otherwise must match
    if (rx.group_id != 0 && reg.group_id != 0 && reg.group_id != rx.group_id)
      continue;
    reg.light->apply_received_command(rx);
  }
}

// ---- Sniff mode: listen on all radio configs without registered lights ----

void MiLightHub::init_sniff_slots_() {
  // One RX slot per unique radio config (indices 0-4)
  static constexpr RemoteType SNIFF_TYPES[] = {
      RemoteType::RGBW, RemoteType::CCT, RemoteType::RGB_CCT, RemoteType::RGB, RemoteType::FUT020,
  };
  for (auto type : SNIFF_TYPES) {
    const RadioConfig &config = get_radio_config(type);
    RxListenSlot &slot = this->rx_slots_[this->rx_slot_count_++];
    compute_nrf24_address(config, slot.nrf24_address.data());
    std::copy(config.channels, config.channels + 3, slot.channels.begin());
    slot.packet_length = config.packet_length;
    slot.radio_config_index = get_radio_config_index(type);
  }
  ESP_LOGI(TAG, "Sniff mode: listening on all %u radio configs", this->rx_slot_count_);
}

// ---- Loop: TX + RX ----

void MiLightHub::loop() {
  if (this->tx_queue_count_ > 0) {
    // TX has priority
    if (this->rx_mode_active_) {
      this->exit_rx_mode_();
    }
    for (int i = 0; i<this->packet_repeats_per_loop_ &&this->tx_queue_count_> 0; i++) {
      this->transmit_packet_();
    }
  } else if (this->listen_enabled_) {
    // No TX pending: do RX
    // If no lights registered, enter sniff mode (listen on all radio configs)
    if (this->rx_slot_count_ == 0 && this->registered_light_count_ == 0) {
      this->init_sniff_slots_();
    }
    if (this->rx_slot_count_ > 0) {
      if (!this->rx_mode_active_) {
        this->enter_rx_mode_();
      }
      this->check_rx_packet_();
      if (App.get_loop_component_start_time() - this->rx_last_switch_ms_ >= RX_DWELL_MS) {
        this->switch_rx_channel_();
      }
    }
  }
}

}  // namespace esphome::milight
