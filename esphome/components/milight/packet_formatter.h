#pragma once

/// @file
/// MiLight/LimitlessLED packet formatter and decoder.
///
/// Protocol references:
/// - NRF24L01+ datasheet:
/// https://www.sparkfun.com/datasheets/Components/SMD/nRF24L01Pluss_Preliminary_Product_Specification_v1_0.pdf
/// - MiLight protocol reverse engineering: https://github.com/henryk/openmili
/// - PL1167 emulation via NRF24: https://github.com/sidoh/esp8266_milight_hub

#include <array>
#include <cstdint>
#include <cstring>

namespace esphome::milight {

/// Remote type identifiers
enum class RemoteType : uint8_t {
  RGBW,
  CCT,
  RGB_CCT,
  RGB,
  FUT020,
  FUT089,
  FUT091,
};

/// Maximum packet size across all remote types (V2 = 9 bytes)
static constexpr uint8_t MAX_PACKET_SIZE = 9;

/// Radio configuration for a remote type
struct RadioConfig {
  uint16_t syncword0;
  uint16_t syncword3;
  uint8_t packet_length;
  uint8_t channels[3];
  uint8_t preamble;
  uint8_t trailer;
};

/// Get radio config for a remote type
const RadioConfig &get_radio_config(RemoteType type);

/// Number of groups supported by a remote type
uint8_t get_num_groups(RemoteType type);

/// Get a human-readable string for a remote type
const char *remote_type_to_string(RemoteType type);

/// Light command to send to bulb
struct LightCommand {
  uint16_t device_id{0};
  uint8_t group_id{0};
  RemoteType remote_type{RemoteType::RGBW};

  // Basic state
  bool turn_on{false};
  bool turn_off{false};

  // Absolute value commands (V1 RGBW, all V2 types)
  bool has_brightness{false};
  bool has_color_temp{false};
  bool has_rgb{false};
  bool has_saturation{false};
  uint8_t brightness{0};    // 0-100
  uint8_t color_temp{0};    // 0-100 (cold to warm)
  uint16_t hue{0};          // 0-359
  uint8_t saturation{100};  // 0-100

  // Special commands
  bool night_mode{false};
  bool pair{false};
  bool unpair{false};
  bool white_mode{false};  // RGBW: switch to white output mode

  // Disco/effect modes
  bool has_effect{false};
  uint8_t effect{0};         // 0-8 for V2 direct select
  bool effect_cycle{false};  // V1: cycle to next disco mode

  // Step-based commands (CCT, RGB, FUT020)
  bool step_up_brightness{false};
  bool step_down_brightness{false};
  bool step_up_temp{false};
  bool step_down_temp{false};
};

/// Format a light command into a packet for the given remote type.
/// Returns the number of bytes written to `packet`.
uint8_t format_packet(const LightCommand &cmd, uint8_t *packet, uint8_t seq);

// ---- Packet decoding (RX) ----

/// Decoded command received from a physical remote
struct ReceivedCommand {
  uint16_t device_id{0};
  uint8_t group_id{0};
  RemoteType remote_type{RemoteType::RGBW};
  uint8_t sequence{0};

  bool is_on{false};
  bool is_off{false};

  bool has_brightness{false};
  uint8_t brightness{0};  // 0-100

  bool has_color_temp{false};
  uint8_t color_temp{0};  // 0-100

  bool has_hue{false};
  uint16_t hue{0};  // 0-359

  bool night_mode{false};
  bool white_mode{false};

  bool has_effect{false};
  uint8_t effect{0};

  // Effect speed (FUT089 S+/S- buttons)
  bool effect_speed_up{false};
  bool effect_speed_down{false};

  // Step-based commands (CCT, RGB, FUT020)
  bool step_up_brightness{false};
  bool step_down_brightness{false};
  bool step_up_temp{false};
  bool step_down_temp{false};
};

/// Decode a raw packet (after PL1167 deframing) into a ReceivedCommand.
/// radio_config_index identifies which radio config received it.
/// Returns true if decode succeeded.
bool decode_packet(const uint8_t *packet, uint8_t length, uint8_t radio_config_index, ReceivedCommand &out);

/// Get the radio config index for a remote type
uint8_t get_radio_config_index(RemoteType type);

// ---- PL1167 CRC ----

/// Compute PL1167 CRC-CCITT over data
uint16_t pl1167_crc(const uint8_t *data, size_t length);

/// Reverse bits in a byte
uint8_t reverse_bits(uint8_t byte);

// ---- NRF24 address computation ----

/// Compute the 5-byte NRF24 address from PL1167 syncwords
void compute_nrf24_address(const RadioConfig &config, uint8_t *address);

}  // namespace esphome::milight
