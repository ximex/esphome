#pragma once

/// @file
/// MiLight/LimitlessLED hub - NRF24L01+ radio interface.
///
/// Hardware references:
/// - NRF24L01+ datasheet:
/// https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf
/// - PL1167 emulation via NRF24: https://github.com/sidoh/esp8266_milight_hub

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"
#include "packet_formatter.h"

#include <array>

namespace esphome::milight {

/// NRF24L01+ register addresses
namespace nrf24 {
static constexpr uint8_t REG_CONFIG = 0x00;
static constexpr uint8_t REG_EN_AA = 0x01;
static constexpr uint8_t REG_EN_RXADDR = 0x02;
static constexpr uint8_t REG_SETUP_AW = 0x03;
static constexpr uint8_t REG_SETUP_RETR = 0x04;
static constexpr uint8_t REG_RF_CH = 0x05;
static constexpr uint8_t REG_RF_SETUP = 0x06;
static constexpr uint8_t REG_STATUS = 0x07;
static constexpr uint8_t REG_RX_PW_P0 = 0x11;
static constexpr uint8_t REG_TX_ADDR = 0x10;
static constexpr uint8_t REG_RX_ADDR_P0 = 0x0A;
static constexpr uint8_t REG_DYNPD = 0x1C;
static constexpr uint8_t REG_FEATURE = 0x1D;

static constexpr uint8_t CMD_W_REGISTER = 0x20;
static constexpr uint8_t CMD_W_TX_PAYLOAD = 0xA0;
static constexpr uint8_t CMD_R_RX_PAYLOAD = 0x61;
static constexpr uint8_t CMD_FLUSH_TX = 0xE1;
static constexpr uint8_t CMD_FLUSH_RX = 0xE2;

// CONFIG register bits
static constexpr uint8_t CONFIG_PWR_UP = 0x02;
static constexpr uint8_t CONFIG_PRIM_TX = 0x00;
static constexpr uint8_t CONFIG_PRIM_RX = 0x01;

// STATUS register bits
static constexpr uint8_t STATUS_RX_DR = 0x40;
static constexpr uint8_t STATUS_TX_DS = 0x20;
static constexpr uint8_t STATUS_MAX_RT = 0x10;

// RF_SETUP register: 1 Mbps base (bit 3 = 0), power in bits 2:1
static constexpr uint8_t RF_SETUP_1MBPS = 0x00;  // 1 Mbps, power bits cleared
static constexpr uint8_t RF_POWER_MIN = 0x00;    // -18 dBm
static constexpr uint8_t RF_POWER_LOW = 0x02;    // -12 dBm
static constexpr uint8_t RF_POWER_HIGH = 0x04;   // -6 dBm
static constexpr uint8_t RF_POWER_MAX = 0x06;    //  0 dBm
}  // namespace nrf24

/// RF24 power level options
enum class Rf24PowerLevel : uint8_t {
  MIN = nrf24::RF_POWER_MIN,
  LOW = nrf24::RF_POWER_LOW,
  HIGH = nrf24::RF_POWER_HIGH,
  MAX = nrf24::RF_POWER_MAX,
};

/// Repeat count for step-based commands (lower than normal for speed)
static constexpr uint16_t STEP_COMMAND_REPEATS = 10;

/// Maximum TX queue depth
static constexpr uint8_t TX_QUEUE_SIZE = 32;

/// Maximum unique radio configs to listen on
static constexpr uint8_t MAX_RX_CONFIGS = 5;

/// Maximum registered lights for RX dispatch
static constexpr uint8_t MAX_REGISTERED_LIGHTS = 16;

/// RX deduplication ring buffer size
static constexpr uint8_t RX_DEDUP_SIZE = 8;

/// Time to dwell on each RX channel before rotating (ms)
static constexpr uint32_t RX_DWELL_MS = 20;

/// Maximum frame size: length byte + max payload + 2 CRC bytes
static constexpr uint8_t MAX_RX_FRAME_SIZE = MAX_PACKET_SIZE + 3;

/// Single TX queue entry
struct TxEntry {
  std::array<uint8_t, MAX_PACKET_SIZE + 3> packet{};  // length + payload + CRC
  uint8_t packet_length{0};
  std::array<uint8_t, 3> channels{};
  std::array<uint8_t, 5> nrf24_address{};
  uint8_t channel_count{0};
  uint8_t current_channel{0};
  uint16_t repeats_remaining{0};
};

/// RX listen slot: one per unique radio config
struct RxListenSlot {
  std::array<uint8_t, 5> nrf24_address{};
  std::array<uint8_t, 3> channels{};
  uint8_t packet_length{0};
  uint8_t radio_config_index{0};
};

/// RX deduplication entry
struct RxDedup {
  uint16_t device_id{0};
  uint8_t sequence{0};
  uint8_t radio_config_index{0};
};

class MiLightLight;

/// Registration of a light for RX dispatch
struct LightRegistration {
  MiLightLight *light{nullptr};
  uint16_t device_id{0};
  uint8_t group_id{0};
  uint8_t radio_config_index{0};
  RemoteType remote_type{RemoteType::RGBW};
};

class MiLightHub : public Component,
                   public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING,
                                         spi::DATA_RATE_8MHZ> {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_ce_pin(GPIOPin *pin) { this->ce_pin_ = pin; }
  void set_packet_repeats(uint16_t repeats) { this->packet_repeats_ = repeats; }
  void set_listen(bool listen) { this->listen_enabled_ = listen; }
  void set_rf24_power_level(Rf24PowerLevel level) { this->rf24_power_level_ = level; }
  void set_packet_repeats_per_loop(uint8_t count) { this->packet_repeats_per_loop_ = count; }
  void set_throttle_threshold(uint32_t ms) { this->throttle_threshold_ms_ = ms; }
  void set_throttle_sensitivity(uint8_t sensitivity) { this->throttle_sensitivity_ = sensitivity; }
  void set_throttle_minimum(uint16_t minimum) { this->throttle_minimum_ = minimum; }

  /// Queue a light command for transmission with default repeat count
  void send_command(const LightCommand &cmd);

  /// Queue a light command with a specific repeat count
  void send_command(const LightCommand &cmd, uint16_t repeats);

  /// Register a light for RX dispatch
  void register_light(MiLightLight *light, uint16_t device_id, uint8_t group_id, RemoteType type);

  /// Send pair command for a specific device/group
  void send_pair(uint16_t device_id, uint8_t group_id, RemoteType type);

  /// Send unpair command for a specific device/group
  void send_unpair(uint16_t device_id, uint8_t group_id, RemoteType type);

 protected:
  /// NRF24 register access
  void write_register_(uint8_t reg, uint8_t value);
  void write_register_(uint8_t reg, const uint8_t *data, uint8_t length);
  uint8_t read_register_(uint8_t reg);
  void write_payload_(const uint8_t *data, uint8_t length);
  void read_payload_(uint8_t *data, uint8_t length);
  void flush_tx_();
  void flush_rx_();

  /// Configure NRF24 address for a queue entry
  void configure_address_(const uint8_t *address);

  /// Transmit one packet on one channel from the front queue entry
  void transmit_packet_();

  /// Pulse CE to trigger transmission
  void pulse_ce_();

  /// Wait for NRF24 transmission to complete (TX_DS flag) with timeout
  void wait_tx_complete_();

  /// Enqueue a formatted packet
  void enqueue_(const LightCommand &cmd, uint16_t repeats);

  /// RX mode management
  void enter_rx_mode_();
  void exit_rx_mode_();
  void switch_rx_channel_();
  void check_rx_packet_();
  void dispatch_rx_packet_(const uint8_t *payload, uint8_t length, uint8_t radio_config_index);
  bool is_rx_duplicate_(uint16_t device_id, uint8_t sequence, uint8_t radio_config_index);

  /// Initialize RX slots for all radio configs (sniff mode - no lights registered)
  void init_sniff_slots_();

  /// Compute effective repeat count applying throttle logic
  uint16_t throttled_repeats_();

  GPIOPin *ce_pin_{nullptr};
  uint16_t packet_repeats_{50};
  uint8_t sequence_num_{0};
  bool listen_enabled_{true};
  Rf24PowerLevel rf24_power_level_{Rf24PowerLevel::MAX};
  uint8_t packet_repeats_per_loop_{10};

  // Throttle state
  uint32_t throttle_threshold_ms_{200};
  uint8_t throttle_sensitivity_{0};
  uint16_t throttle_minimum_{3};
  uint32_t last_command_ms_{0};

  // TX queue (ring buffer)
  std::array<TxEntry, TX_QUEUE_SIZE> tx_queue_{};
  uint8_t tx_queue_head_{0};
  uint8_t tx_queue_count_{0};
  bool tx_address_set_{false};
  std::array<uint8_t, 5> tx_current_address_{};

  // RX state
  std::array<RxListenSlot, MAX_RX_CONFIGS> rx_slots_{};
  uint8_t rx_slot_count_{0};
  std::array<LightRegistration, MAX_REGISTERED_LIGHTS> registered_lights_{};
  uint8_t registered_light_count_{0};
  uint8_t rx_slot_index_{0};
  uint8_t rx_channel_index_{0};
  uint32_t rx_last_switch_ms_{0};
  bool rx_mode_active_{false};

  // RX deduplication ring buffer
  std::array<RxDedup, RX_DEDUP_SIZE> rx_dedup_{};
  uint8_t rx_dedup_index_{0};
};

}  // namespace esphome::milight
