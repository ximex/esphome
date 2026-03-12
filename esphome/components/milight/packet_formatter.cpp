#include "packet_formatter.h"
#include <cstring>

namespace esphome::milight {

// ---- Radio configurations per remote type ----

static constexpr RadioConfig RADIO_CONFIGS[] = {
    // Index 0: RGBW
    {0x147A, 0x258B, 7, {9, 40, 71}, 0xAA, 0x05},
    // Index 1: CCT
    {0x050A, 0x55AA, 7, {4, 39, 74}, 0xAA, 0x05},
    // Index 2: RGB_CCT, FUT089, FUT091
    {0x7236, 0x1809, 9, {8, 39, 70}, 0xAA, 0x05},
    // Index 3: RGB
    {0x9AAB, 0xBCCD, 6, {3, 38, 73}, 0x55, 0x0A},
    // Index 4: FUT020
    {0x50A0, 0xAA55, 6, {6, 41, 76}, 0xAA, 0x0A},
};

uint8_t get_radio_config_index(RemoteType type) {
  switch (type) {
    case RemoteType::RGBW:
      return 0;
    case RemoteType::CCT:
      return 1;
    case RemoteType::RGB_CCT:
    case RemoteType::FUT089:
    case RemoteType::FUT091:
      return 2;
    case RemoteType::RGB:
      return 3;
    case RemoteType::FUT020:
      return 4;
    default:
      return 0;
  }
}

const RadioConfig &get_radio_config(RemoteType type) { return RADIO_CONFIGS[get_radio_config_index(type)]; }

uint8_t get_num_groups(RemoteType type) {
  switch (type) {
    case RemoteType::FUT089:
      return 8;
    case RemoteType::RGB:
    case RemoteType::FUT020:
      return 1;
    default:
      return 4;
  }
}

const char *remote_type_to_string(RemoteType type) {
  switch (type) {
    case RemoteType::RGBW:
      return "RGBW";
    case RemoteType::CCT:
      return "CCT";
    case RemoteType::RGB_CCT:
      return "RGB_CCT";
    case RemoteType::RGB:
      return "RGB";
    case RemoteType::FUT020:
      return "FUT020";
    case RemoteType::FUT089:
      return "FUT089";
    case RemoteType::FUT091:
      return "FUT091";
    default:
      return "UNKNOWN";
  }
}

// ---- PL1167 CRC ----

static constexpr uint16_t CRC_POLY = 0x8408;

uint16_t pl1167_crc(const uint8_t *data, size_t length) {
  uint16_t state = 0;
  for (size_t i = 0; i < length; i++) {
    uint8_t byte = data[i];
    for (int j = 0; j < 8; j++) {
      if ((byte ^ state) & 0x01) {
        state = (state >> 1) ^ CRC_POLY;
      } else {
        state = state >> 1;
      }
      byte >>= 1;
    }
  }
  return state;
}

uint8_t reverse_bits(uint8_t byte) {
  byte = ((byte & 0xF0) >> 4) | ((byte & 0x0F) << 4);
  byte = ((byte & 0xCC) >> 2) | ((byte & 0x33) << 2);
  byte = ((byte & 0xAA) >> 1) | ((byte & 0x55) << 1);
  return byte;
}

// ---- NRF24 address from PL1167 syncwords ----

void compute_nrf24_address(const RadioConfig &config, uint8_t *address) {
  uint16_t s0 = config.syncword0;
  uint16_t s3 = config.syncword3;
  uint8_t pre = config.preamble;
  uint8_t trail = config.trailer;

  address[4] = reverse_bits(((s0 << 4) & 0xF0) | (pre & 0x0F));
  address[3] = reverse_bits((s0 >> 4) & 0xFF);
  address[2] = reverse_bits(((s0 >> 12) & 0x0F) | ((s3 << 4) & 0xF0));
  address[1] = reverse_bits((s3 >> 4) & 0xFF);
  address[0] = reverse_bits(((s3 >> 12) & 0x0F) | ((trail << 4) & 0xF0));
}

// ---- Protocol constants ----

/// V1 protocol identifier bytes
static constexpr uint8_t RGBW_PROTOCOL_ID = 0xB0;
static constexpr uint8_t CCT_PROTOCOL_ID = 0x5A;
static constexpr uint8_t RGB_PROTOCOL_ID = 0xA4;
static constexpr uint8_t FUT020_PROTOCOL_ID = 0xA5;

/// V2 protocol identifier bytes (second byte of V2 packets)
static constexpr uint8_t V2_RGB_CCT_PROTOCOL_ID = 0x20;
static constexpr uint8_t V2_FUT089_PROTOCOL_ID = 0x25;
static constexpr uint8_t V2_FUT091_PROTOCOL_ID = 0x21;

/// RGBW command bytes
static constexpr uint8_t RGBW_CMD_DISCO_MODE = 0x0D;
static constexpr uint8_t RGBW_CMD_BRIGHTNESS = 0x0E;
static constexpr uint8_t RGBW_CMD_COLOR = 0x0F;
static constexpr uint8_t RGBW_NIGHT_FLAG = 0x10;
static constexpr uint8_t RGBW_CMD_WHITE_ALL = 0x11;

/// CCT command bytes
static constexpr uint8_t CCT_CMD_BRIGHTNESS_DOWN = 0x04;
static constexpr uint8_t CCT_CMD_BRIGHTNESS_UP = 0x0C;
static constexpr uint8_t CCT_CMD_TEMP_UP = 0x0E;    // cooler = lower mireds
static constexpr uint8_t CCT_CMD_TEMP_DOWN = 0x0F;  // warmer = higher mireds
static constexpr uint8_t CCT_HELD_FLAG = 0x80;

/// RGB command bytes
static constexpr uint8_t RGB_CMD_COLOR = 0x00;
static constexpr uint8_t RGB_CMD_OFF = 0x01;
static constexpr uint8_t RGB_CMD_ON = 0x02;
static constexpr uint8_t RGB_CMD_BRIGHTNESS_UP = 0x03;
static constexpr uint8_t RGB_CMD_BRIGHTNESS_DOWN = 0x04;
static constexpr uint8_t RGB_CMD_PAIR = 0x05;
static constexpr uint8_t RGB_CMD_MODE_UP = 0x07;

/// FUT020 command bytes
static constexpr uint8_t FUT020_CMD_COLOR = 0x00;
static constexpr uint8_t FUT020_CMD_BRIGHTNESS_DOWN = 0x01;
static constexpr uint8_t FUT020_CMD_MODE_SWITCH = 0x02;
static constexpr uint8_t FUT020_CMD_BRIGHTNESS_UP = 0x03;
static constexpr uint8_t FUT020_CMD_ON_OFF = 0x04;

/// V2 command bytes (RGB_CCT)
static constexpr uint8_t V2_RGB_CCT_CMD_ON_OFF = 0x01;
static constexpr uint8_t V2_RGB_CCT_CMD_COLOR = 0x02;
static constexpr uint8_t V2_RGB_CCT_CMD_COLOR_TEMP = 0x03;
static constexpr uint8_t V2_RGB_CCT_CMD_BRIGHTNESS = 0x04;
static constexpr uint8_t V2_RGB_CCT_CMD_EFFECT = 0x05;

/// V2 command bytes (FUT089)
static constexpr uint8_t V2_FUT089_CMD_ON_OFF = 0x01;
static constexpr uint8_t V2_FUT089_CMD_COLOR = 0x02;
static constexpr uint8_t V2_FUT089_CMD_BRIGHTNESS = 0x05;
static constexpr uint8_t V2_FUT089_CMD_EFFECT = 0x06;
static constexpr uint8_t V2_FUT089_CMD_COLOR_TEMP = 0x07;
/// FUT089 special ON/OFF argument for white mode
static constexpr uint8_t V2_FUT089_WHITE_MODE_ARG = 0x14;

/// V2 command bytes (FUT091)
static constexpr uint8_t V2_FUT091_CMD_ON_OFF = 0x01;
static constexpr uint8_t V2_FUT091_CMD_BRIGHTNESS = 0x02;
static constexpr uint8_t V2_FUT091_CMD_COLOR_TEMP = 0x03;

/// V2 scale offsets
static constexpr uint8_t V2_RGB_CCT_COLOR_OFFSET = 0x5F;
static constexpr uint8_t V2_RGB_CCT_TEMP_OFFSET = 0xCC;
static constexpr uint8_t V2_RGB_CCT_BRIGHTNESS_OFFSET = 0x8F;
static constexpr uint8_t V2_RGB_CCT_SATURATION_OFFSET = 0x0D;
static constexpr uint8_t V2_FUT091_BRIGHTNESS_OFFSET = 0x97;
static constexpr uint8_t V2_FUT091_TEMP_OFFSET = 0xC5;

/// V2 night mode flag
static constexpr uint8_t V2_NIGHT_MODE_FLAG = 0x80;
static constexpr uint8_t V2_CMD_NIGHT_MODE = 0x81;  // 0x01 | 0x80

/// RGBW encoding parameters
static constexpr uint8_t RGBW_HUE_OFFSET = 40;
static constexpr uint8_t RGBW_BRIGHTNESS_LEVELS = 25;
static constexpr uint8_t RGBW_BRIGHTNESS_OFFSET = 17;
static constexpr uint8_t FUT020_HUE_OFFSET = 0xB0;

// ---- V2 encoding ----

static constexpr uint8_t V2_OFFSETS[8][4] = {
    {0x45, 0x1F, 0x14, 0x5C},  // row 0: protocol ID
    {0x2B, 0xC9, 0xE3, 0x11},  // row 1: device ID hi
    {0x6D, 0x5F, 0x8A, 0x2B},  // row 2: device ID lo
    {0xAF, 0x03, 0x1D, 0xF3},  // row 3: command
    {0x1A, 0xE2, 0xF0, 0xD1},  // row 4: argument
    {0x04, 0xD8, 0x71, 0x42},  // row 5: sequence
    {0xAF, 0x04, 0xDD, 0x07},  // row 6: group
    {0x61, 0x13, 0x38, 0x64},  // row 7: checksum
};

static constexpr uint8_t V2_OFFSET_JUMP_START = 0x54;

/// Derive the XOR key from the raw key byte (packet[0])
static uint8_t v2_xor_key(uint8_t key) {
  const uint8_t shift = (key & 0x0F) < 0x04 ? 0 : 1;
  const uint8_t x = (((key & 0xF0) >> 4) + shift + 6) % 8;
  const uint8_t msn = (((4 + x) ^ 1) & 0x0F) << 4;
  const uint8_t lsn = ((((key & 0x0F) + 4) ^ 2) & 0x0F);
  return msn | lsn;
}

/// Get the V2 offset for a byte position (1-8) using the raw key
static uint8_t v2_offset(uint8_t byte_pos, uint8_t raw_key, uint8_t jump_start) {
  uint8_t offset = V2_OFFSETS[byte_pos - 1][raw_key % 4];
  if (jump_start > 0 && raw_key >= jump_start && raw_key < jump_start + 0x80) {
    offset += 0x80;
  }
  return offset;
}

static void v2_encode_packet(uint8_t *packet) {
  uint8_t raw_key = packet[0];
  uint8_t xor_key = v2_xor_key(raw_key);
  uint8_t sum = xor_key;

  // Encode bytes 1-7 and accumulate checksum from decoded values
  for (int i = 1; i <= 7; i++) {
    sum += packet[i];
    uint8_t s2 = v2_offset(i, raw_key, V2_OFFSET_JUMP_START);
    packet[i] = (packet[i] ^ xor_key) + s2;
  }

  // Encode checksum: s1=2, jumpStart=0
  uint8_t s2 = v2_offset(8, raw_key, 0);
  packet[8] = ((sum + 2) ^ xor_key) + s2;
}

// ---- V2 scale conversion ----

static uint8_t to_v2_scale(uint8_t value, uint8_t end_value, uint8_t interval, bool reverse = true) {
  if (reverse)
    value = 100 - value;
  return (value * interval) + end_value;
}

// ---- V2 group command argument ----

static uint8_t group_command_arg(bool is_on, uint8_t group_id, uint8_t num_groups) {
  return group_id + (is_on ? 0 : (num_groups + 1));
}

// ---- Helper: RGBW ON/OFF command byte for a group ----

static uint8_t rgbw_on_cmd(uint8_t group) { return (group == 0) ? 0x01 : (0x01 + group * 2); }
static uint8_t rgbw_off_cmd(uint8_t group) { return (group == 0) ? 0x02 : (0x02 + group * 2); }

// ---- V1 Packet Formatters ----

static uint8_t format_rgbw(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  packet[0] = RGBW_PROTOCOL_ID;
  packet[1] = (cmd.device_id >> 8) & 0xFF;
  packet[2] = cmd.device_id & 0xFF;

  uint8_t group = cmd.group_id;
  if (group > 4)
    group = 0;

  // Night mode: OFF_command | RGBW_NIGHT_FLAG
  if (cmd.night_mode) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = rgbw_off_cmd(group) | RGBW_NIGHT_FLAG;
    packet[6] = seq;
    return 7;
  }

  // Pair: send ON command (caller enqueues 5 times per spec)
  if (cmd.pair) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = rgbw_on_cmd(group);
    packet[6] = seq;
    return 7;
  }

  // Unpair: send white mode (GROUP_N_MAX_LEVEL)
  if (cmd.unpair) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    // White mode command: 0x11=all, 0x13=g1, 0x15=g2, 0x17=g3, 0x19=g4
    packet[5] = (group == 0) ? RGBW_CMD_WHITE_ALL : (RGBW_CMD_WHITE_ALL + group * 2);
    packet[6] = seq;
    return 7;
  }

  // White mode: GROUP_N_MAX_LEVEL
  if (cmd.white_mode) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = (group == 0) ? RGBW_CMD_WHITE_ALL : (RGBW_CMD_WHITE_ALL + group * 2);
    packet[6] = seq;
    return 7;
  }

  // Disco mode cycle
  if (cmd.effect_cycle) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = RGBW_CMD_DISCO_MODE;
    packet[6] = seq;
    return 7;
  }

  if (cmd.turn_on) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = rgbw_on_cmd(group);
    packet[6] = seq;
    return 7;
  }

  if (cmd.turn_off) {
    packet[3] = 0x00;
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = rgbw_off_cmd(group);
    packet[6] = seq;
    return 7;
  }

  if (cmd.has_rgb) {
    uint16_t hue = (cmd.hue + RGBW_HUE_OFFSET) % 360;
    packet[3] = (uint8_t) ((hue * 255) / 359);
    packet[4] = (0 << 3) | (group & 0x07);
    packet[5] = RGBW_CMD_COLOR;
    packet[6] = seq;
    return 7;
  }

  if (cmd.has_brightness) {
    uint8_t adjusted = (cmd.brightness * RGBW_BRIGHTNESS_LEVELS) / 100;
    uint8_t encoded = ((31 - adjusted) + RGBW_BRIGHTNESS_OFFSET) % 32;
    packet[3] = 0x00;
    packet[4] = ((encoded & 0x1F) << 3) | (group & 0x07);
    packet[5] = RGBW_CMD_BRIGHTNESS;
    packet[6] = seq;
    return 7;
  }

  // Default: ON
  packet[3] = 0x00;
  packet[4] = (0 << 3) | (group & 0x07);
  packet[5] = rgbw_on_cmd(group);
  packet[6] = seq;
  return 7;
}

// ---- CCT command byte lookups ----

static constexpr uint8_t CCT_ON_CMDS[] = {0x05, 0x08, 0x0D, 0x07, 0x02};
static constexpr uint8_t CCT_OFF_CMDS[] = {0x09, 0x0B, 0x03, 0x0A, 0x06};

/// Finalize CCT packet: set sequence and compute checksum
static uint8_t cct_finalize(uint8_t *packet, uint8_t seq) {
  packet[5] = seq;
  // Checksum: sum of bytes 0-5 + 7
  uint16_t sum = 7;
  for (int i = 0; i <= 5; i++) {
    sum += packet[i];
  }
  packet[6] = sum & 0xFF;
  return 7;
}

/// Determine the CCT command byte for a given light command
static uint8_t cct_command_byte(const LightCommand &cmd, uint8_t group_idx) {
  if (cmd.night_mode)
    return CCT_OFF_CMDS[group_idx] | CCT_HELD_FLAG;
  if (cmd.pair || cmd.unpair)
    return CCT_ON_CMDS[group_idx];
  if (cmd.step_up_brightness)
    return CCT_CMD_BRIGHTNESS_UP;
  if (cmd.step_down_brightness)
    return CCT_CMD_BRIGHTNESS_DOWN;
  if (cmd.step_up_temp)
    return CCT_CMD_TEMP_DOWN;  // warmer = higher mireds
  if (cmd.step_down_temp)
    return CCT_CMD_TEMP_UP;  // cooler = lower mireds
  if (cmd.turn_on)
    return CCT_ON_CMDS[group_idx];
  if (cmd.turn_off)
    return CCT_OFF_CMDS[group_idx];
  // Default: ON
  return CCT_ON_CMDS[group_idx];
}

static uint8_t format_cct(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  packet[0] = CCT_PROTOCOL_ID;
  packet[1] = (cmd.device_id >> 8) & 0xFF;
  packet[2] = cmd.device_id & 0xFF;
  packet[3] = cmd.group_id;

  uint8_t group_idx = (cmd.group_id > 4) ? 0 : cmd.group_id;
  packet[4] = cct_command_byte(cmd, group_idx);
  return cct_finalize(packet, seq);
}

static uint8_t format_rgb(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  packet[0] = RGB_PROTOCOL_ID;
  packet[1] = (cmd.device_id >> 8) & 0xFF;
  packet[2] = cmd.device_id & 0xFF;

  // Pair: SPEED_UP (0x05)
  if (cmd.pair) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_PAIR;
    packet[5] = seq;
    return 6;
  }

  // Unpair: 0x05 | 0x10 = 0x15
  if (cmd.unpair) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_PAIR | RGBW_NIGHT_FLAG;
    packet[5] = seq;
    return 6;
  }

  // Disco mode cycle: MODE_UP
  if (cmd.effect_cycle) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_MODE_UP;
    packet[5] = seq;
    return 6;
  }

  // Step-based brightness
  if (cmd.step_up_brightness) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_BRIGHTNESS_UP;
    packet[5] = seq;
    return 6;
  }
  if (cmd.step_down_brightness) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_BRIGHTNESS_DOWN;
    packet[5] = seq;
    return 6;
  }

  if (cmd.turn_on) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_ON;
    packet[5] = seq;
    return 6;
  }

  if (cmd.turn_off) {
    packet[3] = 0x00;
    packet[4] = RGB_CMD_OFF;
    packet[5] = seq;
    return 6;
  }

  if (cmd.has_rgb) {
    uint8_t color = (uint8_t) ((cmd.hue * 255) / 359);
    packet[3] = color;
    packet[4] = RGB_CMD_COLOR;
    packet[5] = seq;
    return 6;
  }

  // Default: ON
  packet[3] = 0x00;
  packet[4] = RGB_CMD_ON;
  packet[5] = seq;
  return 6;
}

static uint8_t format_fut020(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  packet[0] = FUT020_PROTOCOL_ID;
  packet[1] = (cmd.device_id >> 8) & 0xFF;
  packet[2] = cmd.device_id & 0xFF;

  // Pair/unpair: command BRIGHTNESS_UP
  if (cmd.pair || cmd.unpair) {
    packet[3] = 0x00;
    packet[4] = FUT020_CMD_BRIGHTNESS_UP;
    packet[5] = seq;
    return 6;
  }

  // Disco mode cycle: MODE_SWITCH
  if (cmd.effect_cycle) {
    packet[3] = 0x00;
    packet[4] = FUT020_CMD_MODE_SWITCH;
    packet[5] = seq;
    return 6;
  }

  // Step-based brightness
  if (cmd.step_up_brightness) {
    packet[3] = 0x00;
    packet[4] = FUT020_CMD_BRIGHTNESS_UP;
    packet[5] = seq;
    return 6;
  }
  if (cmd.step_down_brightness) {
    packet[3] = 0x00;
    packet[4] = FUT020_CMD_BRIGHTNESS_DOWN;
    packet[5] = seq;
    return 6;
  }

  if (cmd.turn_on || cmd.turn_off) {
    packet[3] = 0x00;
    packet[4] = FUT020_CMD_ON_OFF;
    packet[5] = seq;
    return 6;
  }

  if (cmd.has_rgb) {
    uint8_t color = (uint8_t) (((cmd.hue * 255) / 360 + FUT020_HUE_OFFSET) & 0xFF);
    packet[3] = color;
    packet[4] = FUT020_CMD_COLOR;
    packet[5] = seq;
    return 6;
  }

  // Default: ON_OFF toggle
  packet[3] = 0x00;
  packet[4] = FUT020_CMD_ON_OFF;
  packet[5] = seq;
  return 6;
}

// ---- V2 Packet Formatters ----

// Helper to set V2 common header bytes
static void v2_set_header(uint8_t *packet, uint8_t protocol_id, const LightCommand &cmd) {
  packet[0] = 0x00;  // key
  packet[1] = protocol_id;
  packet[2] = (cmd.device_id >> 8) & 0xFF;
  packet[3] = cmd.device_id & 0xFF;
}

// Helper to finalize V2 packet (set seq, group, encode)
static uint8_t v2_finalize(uint8_t *packet, uint8_t seq, uint8_t group_id) {
  packet[6] = seq;
  packet[7] = group_id;
  packet[8] = 0;  // checksum placeholder
  v2_encode_packet(packet);
  return 9;
}

static uint8_t format_rgb_cct(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  uint8_t num_groups = 4;
  v2_set_header(packet, V2_RGB_CCT_PROTOCOL_ID, cmd);

  // Night mode: command V2_CMD_NIGHT_MODE, arg = groupCommandArg(OFF, groupId)
  if (cmd.night_mode) {
    packet[4] = V2_CMD_NIGHT_MODE;
    packet[5] = group_command_arg(false, cmd.group_id, num_groups);
    return v2_finalize(packet, seq, cmd.group_id);
  }

  // Pair: ON for this group
  if (cmd.pair) {
    packet[4] = V2_RGB_CCT_CMD_ON_OFF;
    packet[5] = group_command_arg(true, cmd.group_id, num_groups);
    return v2_finalize(packet, seq, cmd.group_id);
  }

  // Unpair: ON for group 0 (all)
  if (cmd.unpair) {
    packet[4] = V2_RGB_CCT_CMD_ON_OFF;
    packet[5] = group_command_arg(true, 0, num_groups);
    return v2_finalize(packet, seq, 0);
  }

  // Disco effect
  if (cmd.has_effect) {
    packet[4] = V2_RGB_CCT_CMD_EFFECT;
    packet[5] = cmd.effect;
    return v2_finalize(packet, seq, cmd.group_id);
  }

  if (cmd.turn_on || cmd.turn_off) {
    packet[4] = V2_RGB_CCT_CMD_ON_OFF;
    packet[5] = group_command_arg(cmd.turn_on, cmd.group_id, num_groups);
  } else if (cmd.has_rgb) {
    packet[4] = V2_RGB_CCT_CMD_COLOR;
    uint8_t hue_byte = (uint8_t) ((cmd.hue * 255) / 359);
    packet[5] = V2_RGB_CCT_COLOR_OFFSET + hue_byte;
  } else if (cmd.has_saturation) {
    // Saturation shares cmd 0x04 with brightness but uses SATURATION_OFFSET
    packet[4] = V2_RGB_CCT_CMD_BRIGHTNESS;
    packet[5] = V2_RGB_CCT_SATURATION_OFFSET + cmd.saturation;
  } else if (cmd.has_color_temp) {
    packet[4] = V2_RGB_CCT_CMD_COLOR_TEMP;
    packet[5] = to_v2_scale(cmd.color_temp, V2_RGB_CCT_TEMP_OFFSET, 2);
  } else if (cmd.has_brightness) {
    packet[4] = V2_RGB_CCT_CMD_BRIGHTNESS;
    packet[5] = V2_RGB_CCT_BRIGHTNESS_OFFSET + cmd.brightness;
  } else {
    packet[4] = V2_RGB_CCT_CMD_ON_OFF;
    packet[5] = group_command_arg(true, cmd.group_id, num_groups);
  }

  return v2_finalize(packet, seq, cmd.group_id);
}

static uint8_t format_fut089(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  uint8_t num_groups = 8;
  v2_set_header(packet, V2_FUT089_PROTOCOL_ID, cmd);

  // Night mode
  if (cmd.night_mode) {
    packet[4] = V2_CMD_NIGHT_MODE;
    packet[5] = group_command_arg(false, cmd.group_id, num_groups);
    return v2_finalize(packet, seq, cmd.group_id);
  }

  // Pair
  if (cmd.pair) {
    packet[4] = V2_FUT089_CMD_ON_OFF;
    packet[5] = group_command_arg(true, cmd.group_id, num_groups);
    return v2_finalize(packet, seq, cmd.group_id);
  }

  // Unpair: ON for group 0
  if (cmd.unpair) {
    packet[4] = V2_FUT089_CMD_ON_OFF;
    packet[5] = group_command_arg(true, 0, num_groups);
    return v2_finalize(packet, seq, 0);
  }

  // Disco effect
  if (cmd.has_effect) {
    packet[4] = V2_FUT089_CMD_EFFECT;
    packet[5] = cmd.effect;
    return v2_finalize(packet, seq, cmd.group_id);
  }

  if (cmd.white_mode) {
    packet[4] = V2_FUT089_CMD_ON_OFF;
    packet[5] = V2_FUT089_WHITE_MODE_ARG;
    return v2_finalize(packet, seq, cmd.group_id);
  }

  if (cmd.turn_on || cmd.turn_off) {
    packet[4] = V2_FUT089_CMD_ON_OFF;
    packet[5] = group_command_arg(cmd.turn_on, cmd.group_id, num_groups);
  } else if (cmd.has_rgb) {
    packet[4] = V2_FUT089_CMD_COLOR;
    packet[5] = (uint8_t) ((cmd.hue * 255) / 359);
  } else if (cmd.has_saturation) {
    // Saturation shares cmd 0x07 with kelvin; bulb disambiguates by mode (color mode = saturation)
    packet[4] = V2_FUT089_CMD_COLOR_TEMP;
    packet[5] = 100 - cmd.saturation;
  } else if (cmd.has_color_temp) {
    packet[4] = V2_FUT089_CMD_COLOR_TEMP;
    packet[5] = 100 - cmd.color_temp;
  } else if (cmd.has_brightness) {
    packet[4] = V2_FUT089_CMD_BRIGHTNESS;
    packet[5] = cmd.brightness;
  } else {
    packet[4] = V2_FUT089_CMD_ON_OFF;
    packet[5] = group_command_arg(true, cmd.group_id, num_groups);
  }

  return v2_finalize(packet, seq, cmd.group_id);
}

static uint8_t format_fut091(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  uint8_t num_groups = 4;
  v2_set_header(packet, V2_FUT091_PROTOCOL_ID, cmd);

  // Night mode
  if (cmd.night_mode) {
    packet[4] = V2_CMD_NIGHT_MODE;
    packet[5] = group_command_arg(false, cmd.group_id, num_groups);
    return v2_finalize(packet, seq, cmd.group_id);
  }

  // Pair
  if (cmd.pair) {
    packet[4] = V2_FUT091_CMD_ON_OFF;
    packet[5] = group_command_arg(true, cmd.group_id, num_groups);
    return v2_finalize(packet, seq, cmd.group_id);
  }

  // Unpair: ON for group 0
  if (cmd.unpair) {
    packet[4] = V2_FUT091_CMD_ON_OFF;
    packet[5] = group_command_arg(true, 0, num_groups);
    return v2_finalize(packet, seq, 0);
  }

  if (cmd.turn_on || cmd.turn_off) {
    packet[4] = V2_FUT091_CMD_ON_OFF;
    packet[5] = group_command_arg(cmd.turn_on, cmd.group_id, num_groups);
  } else if (cmd.has_color_temp) {
    packet[4] = V2_FUT091_CMD_COLOR_TEMP;
    packet[5] = to_v2_scale(cmd.color_temp, V2_FUT091_TEMP_OFFSET, 2, false);
  } else if (cmd.has_brightness) {
    packet[4] = V2_FUT091_CMD_BRIGHTNESS;
    packet[5] = to_v2_scale(cmd.brightness, V2_FUT091_BRIGHTNESS_OFFSET, 2);
  } else {
    packet[4] = V2_FUT091_CMD_ON_OFF;
    packet[5] = group_command_arg(true, cmd.group_id, num_groups);
  }

  return v2_finalize(packet, seq, cmd.group_id);
}

// ---- Public format_packet dispatcher ----

uint8_t format_packet(const LightCommand &cmd, uint8_t *packet, uint8_t seq) {
  memset(packet, 0, MAX_PACKET_SIZE);
  switch (cmd.remote_type) {
    case RemoteType::RGBW:
      return format_rgbw(cmd, packet, seq);
    case RemoteType::CCT:
      return format_cct(cmd, packet, seq);
    case RemoteType::RGB:
      return format_rgb(cmd, packet, seq);
    case RemoteType::FUT020:
      return format_fut020(cmd, packet, seq);
    case RemoteType::RGB_CCT:
      return format_rgb_cct(cmd, packet, seq);
    case RemoteType::FUT089:
      return format_fut089(cmd, packet, seq);
    case RemoteType::FUT091:
      return format_fut091(cmd, packet, seq);
    default:
      return 0;
  }
}

// ---- V2 decoding ----

static bool v2_decode_packet(uint8_t *packet) {
  uint8_t raw_key = packet[0];
  uint8_t xor_key = v2_xor_key(raw_key);

  // Decode data bytes 1-7 (with jump start)
  for (int i = 1; i <= 7; i++) {
    uint8_t s2 = v2_offset(i, raw_key, V2_OFFSET_JUMP_START);
    packet[i] = (packet[i] - s2) ^ xor_key;
  }

  // Decode checksum byte 8 (jumpStart=0 to match encode)
  uint8_t s2 = v2_offset(8, raw_key, 0);
  packet[8] = (packet[8] - s2) ^ xor_key;

  // Verify checksum: sum = xor_key + decoded bytes 1-7, encoded with s1=2
  uint8_t sum = xor_key;
  for (int i = 1; i <= 7; i++) {
    sum += packet[i];
  }
  return packet[8] == (uint8_t) (sum + 2);
}

// ---- V1 packet decoders ----

static bool decode_rgbw(const uint8_t *packet, uint8_t length, ReceivedCommand &out) {
  if (length != 7)
    return false;
  if ((packet[0] & 0xF0) != RGBW_PROTOCOL_ID)
    return false;

  out.remote_type = RemoteType::RGBW;
  out.device_id = (packet[1] << 8) | packet[2];
  out.group_id = packet[4] & 0x07;
  out.sequence = packet[6];

  uint8_t cmd = packet[5];

  // Night mode: OFF command | RGBW_NIGHT_FLAG
  if (cmd == (rgbw_off_cmd(out.group_id) | RGBW_NIGHT_FLAG)) {
    out.night_mode = true;
    return true;
  }

  // White mode commands: 0x11=all, 0x13=g1, 0x15=g2, 0x17=g3, 0x19=g4
  if (cmd == RGBW_CMD_WHITE_ALL || cmd == (RGBW_CMD_WHITE_ALL + 2) || cmd == (RGBW_CMD_WHITE_ALL + 4) ||
      cmd == (RGBW_CMD_WHITE_ALL + 6) || cmd == (RGBW_CMD_WHITE_ALL + 8)) {
    out.white_mode = true;
    return true;
  }

  // Disco mode cycle
  if (cmd == RGBW_CMD_DISCO_MODE) {
    out.has_effect = true;
    out.effect = 0;
    return true;
  }

  // Color command
  if (cmd == RGBW_CMD_COLOR) {
    out.has_hue = true;
    uint16_t raw_hue = (static_cast<uint16_t>(packet[3]) * 359) / 255;
    out.hue = (raw_hue + 360 - RGBW_HUE_OFFSET) % 360;
    return true;
  }

  // Brightness command
  if (cmd == RGBW_CMD_BRIGHTNESS) {
    out.has_brightness = true;
    uint8_t encoded = (packet[4] >> 3) & 0x1F;
    uint8_t adjusted = (31 - ((encoded - RGBW_BRIGHTNESS_OFFSET + 32) % 32));
    out.brightness = (adjusted * 100) / RGBW_BRIGHTNESS_LEVELS;
    if (out.brightness > 100)
      out.brightness = 100;
    return true;
  }

  // ON commands: 0x01=all, 0x03=g1, 0x05=g2, 0x07=g3, 0x09=g4
  if (cmd == rgbw_on_cmd(out.group_id)) {
    out.is_on = true;
    return true;
  }

  // OFF commands: 0x02=all, 0x04=g1, 0x06=g2, 0x08=g3, 0x0A=g4
  if (cmd == rgbw_off_cmd(out.group_id)) {
    out.is_off = true;
    return true;
  }

  return true;
}

static bool decode_cct(const uint8_t *packet, uint8_t length, ReceivedCommand &out) {
  if (length != 7)
    return false;
  if (packet[0] != CCT_PROTOCOL_ID)
    return false;

  out.remote_type = RemoteType::CCT;
  out.device_id = (packet[1] << 8) | packet[2];
  out.group_id = packet[3];
  out.sequence = packet[5];

  // Verify checksum
  uint16_t sum = 7;
  for (int i = 0; i <= 5; i++) {
    sum += packet[i];
  }
  if ((sum & 0xFF) != packet[6])
    return false;

  uint8_t cmd = packet[4] & 0x7F;  // strip held flag
  bool held = (packet[4] & CCT_HELD_FLAG) != 0;

  uint8_t group_idx = (out.group_id > 4) ? 0 : out.group_id;

  // Night mode: OFF command with held flag
  if (held && cmd == CCT_OFF_CMDS[group_idx]) {
    out.night_mode = true;
    return true;
  }

  // Step-based brightness
  if (cmd == CCT_CMD_BRIGHTNESS_UP) {
    out.step_up_brightness = true;
    return true;
  }
  if (cmd == CCT_CMD_BRIGHTNESS_DOWN) {
    out.step_down_brightness = true;
    return true;
  }

  // Step-based temperature
  if (cmd == CCT_CMD_TEMP_DOWN) {
    out.step_up_temp = true;
    return true;
  }
  if (cmd == CCT_CMD_TEMP_UP) {
    out.step_down_temp = true;
    return true;
  }

  // ON/OFF by matching command tables
  if (cmd == CCT_ON_CMDS[group_idx]) {
    out.is_on = true;
    return true;
  }
  if (cmd == CCT_OFF_CMDS[group_idx]) {
    out.is_off = true;
    return true;
  }

  return true;
}

static bool decode_rgb(const uint8_t *packet, uint8_t length, ReceivedCommand &out) {
  if (length != 6)
    return false;
  if (packet[0] != RGB_PROTOCOL_ID)
    return false;

  out.remote_type = RemoteType::RGB;
  out.device_id = (packet[1] << 8) | packet[2];
  out.group_id = 0;
  out.sequence = packet[5];

  uint8_t cmd = packet[4];

  if (cmd == RGB_CMD_ON) {
    out.is_on = true;
  } else if (cmd == RGB_CMD_OFF) {
    out.is_off = true;
  } else if (cmd == RGB_CMD_BRIGHTNESS_UP) {
    out.step_up_brightness = true;
  } else if (cmd == RGB_CMD_BRIGHTNESS_DOWN) {
    out.step_down_brightness = true;
  } else if (cmd == RGB_CMD_MODE_UP) {
    out.has_effect = true;
    out.effect = 0;
  } else if (cmd == RGB_CMD_COLOR) {
    out.has_hue = true;
    out.hue = (static_cast<uint16_t>(packet[3]) * 359) / 255;
  }

  return true;
}

static bool decode_fut020(const uint8_t *packet, uint8_t length, ReceivedCommand &out) {
  if (length != 6)
    return false;
  if (packet[0] != FUT020_PROTOCOL_ID)
    return false;

  out.remote_type = RemoteType::FUT020;
  out.device_id = (packet[1] << 8) | packet[2];
  out.group_id = 0;
  out.sequence = packet[5];

  uint8_t cmd = packet[4];

  if (cmd == FUT020_CMD_ON_OFF) {
    // ON_OFF toggle - treat as ON (we can't know the current state)
    out.is_on = true;
  } else if (cmd == FUT020_CMD_BRIGHTNESS_UP) {
    out.step_up_brightness = true;
  } else if (cmd == FUT020_CMD_BRIGHTNESS_DOWN) {
    out.step_down_brightness = true;
  } else if (cmd == FUT020_CMD_MODE_SWITCH) {
    out.has_effect = true;
    out.effect = 0;
  } else if (cmd == FUT020_CMD_COLOR) {
    out.has_hue = true;
    out.hue = (static_cast<uint16_t>((packet[3] - FUT020_HUE_OFFSET) & 0xFF) * 360) / 255;
  }

  return true;
}

// ---- V2 packet decoders ----

static void decode_rgb_cct_v2(uint8_t cmd, uint8_t arg, ReceivedCommand &out) {
  out.remote_type = RemoteType::RGB_CCT;
  uint8_t num_groups = 4;

  if (cmd == V2_RGB_CCT_CMD_ON_OFF) {
    // ON/OFF: determine from argument
    if (arg <= num_groups) {
      out.is_on = true;
      out.group_id = arg;
    } else {
      out.is_off = true;
      out.group_id = arg - (num_groups + 1);
    }
  } else if (cmd == V2_RGB_CCT_CMD_COLOR) {
    out.has_hue = true;
    uint8_t hue_byte = arg - V2_RGB_CCT_COLOR_OFFSET;
    out.hue = (static_cast<uint16_t>(hue_byte) * 359) / 255;
  } else if (cmd == V2_RGB_CCT_CMD_COLOR_TEMP) {
    out.has_color_temp = true;
    // Reverse: arg = to_v2_scale(value, V2_RGB_CCT_TEMP_OFFSET, 2) = (100-value)*2 + V2_RGB_CCT_TEMP_OFFSET
    int raw = (static_cast<int>(arg) - V2_RGB_CCT_TEMP_OFFSET) / 2;
    out.color_temp = static_cast<uint8_t>(100 - (raw < 0 ? 0 : (raw > 100 ? 100 : raw)));
  } else if (cmd == V2_RGB_CCT_CMD_BRIGHTNESS) {
    out.has_brightness = true;
    int raw = static_cast<int>(arg) - V2_RGB_CCT_BRIGHTNESS_OFFSET;
    out.brightness = static_cast<uint8_t>(raw < 0 ? 0 : (raw > 100 ? 100 : raw));
  } else if (cmd == V2_RGB_CCT_CMD_EFFECT) {
    out.has_effect = true;
    out.effect = arg;
  }
}

/// FUT089 special arg values in ON_OFF command (0x01)
static constexpr uint8_t FUT089_ARG_SPEED_UP = 0x12;
static constexpr uint8_t FUT089_ARG_SPEED_DOWN = 0x13;
static constexpr uint8_t FUT089_ARG_WHITE_MODE = 0x14;

static void decode_fut089_v2(uint8_t cmd, uint8_t arg, ReceivedCommand &out) {
  out.remote_type = RemoteType::FUT089;
  uint8_t num_groups = 8;

  if (cmd == V2_FUT089_CMD_ON_OFF) {
    // Special buttons encoded as arg values above the ON/OFF range
    if (arg == FUT089_ARG_SPEED_UP) {
      out.effect_speed_up = true;
      out.group_id = 0;
      return;
    }
    if (arg == FUT089_ARG_SPEED_DOWN) {
      out.effect_speed_down = true;
      out.group_id = 0;
      return;
    }
    if (arg == FUT089_ARG_WHITE_MODE) {
      out.white_mode = true;
      out.group_id = 0;
      return;
    }
    // ON: arg 0-8, OFF: arg 9-17
    if (arg <= num_groups) {
      out.is_on = true;
      out.group_id = arg;
    } else {
      out.is_off = true;
      out.group_id = arg - (num_groups + 1);
    }
  } else if (cmd == V2_FUT089_CMD_COLOR) {
    out.has_hue = true;
    out.hue = (static_cast<uint16_t>(arg) * 359) / 255;
  } else if (cmd == V2_FUT089_CMD_BRIGHTNESS) {
    out.has_brightness = true;
    out.brightness = arg > 100 ? 100 : arg;
  } else if (cmd == V2_FUT089_CMD_EFFECT) {
    out.has_effect = true;
    out.effect = arg;
  } else if (cmd == V2_FUT089_CMD_COLOR_TEMP) {
    out.has_color_temp = true;
    int raw = 100 - static_cast<int>(arg);
    out.color_temp = static_cast<uint8_t>(raw < 0 ? 0 : (raw > 100 ? 100 : raw));
  }
}

static void decode_fut091_v2(uint8_t cmd, uint8_t arg, ReceivedCommand &out) {
  out.remote_type = RemoteType::FUT091;
  uint8_t num_groups = 4;

  if (cmd == V2_FUT091_CMD_ON_OFF) {
    if (arg <= num_groups) {
      out.is_on = true;
      out.group_id = arg;
    } else {
      out.is_off = true;
      out.group_id = arg - (num_groups + 1);
    }
  } else if (cmd == V2_FUT091_CMD_BRIGHTNESS) {
    out.has_brightness = true;
    // Reverse: arg = to_v2_scale(value, V2_FUT091_BRIGHTNESS_OFFSET, 2) = (100-value)*2 + V2_FUT091_BRIGHTNESS_OFFSET
    int raw = (static_cast<int>(arg) - V2_FUT091_BRIGHTNESS_OFFSET) / 2;
    out.brightness = static_cast<uint8_t>(100 - (raw < 0 ? 0 : (raw > 100 ? 100 : raw)));
  } else if (cmd == V2_FUT091_CMD_COLOR_TEMP) {
    out.has_color_temp = true;
    // Reverse: arg = to_v2_scale(value, V2_FUT091_TEMP_OFFSET, 2, false) = value*2 + V2_FUT091_TEMP_OFFSET
    int raw = (static_cast<int>(arg) - V2_FUT091_TEMP_OFFSET) / 2;
    out.color_temp = static_cast<uint8_t>(raw < 0 ? 0 : (raw > 100 ? 100 : raw));
  }
}

static bool decode_v2_common(uint8_t *packet, uint8_t length, ReceivedCommand &out) {
  if (length != 9)
    return false;

  // Decrypt in place
  if (!v2_decode_packet(packet))
    return false;

  out.device_id = (packet[2] << 8) | packet[3];
  out.sequence = packet[6];
  out.group_id = packet[7];

  uint8_t protocol_id = packet[1];
  uint8_t cmd = packet[4];
  uint8_t arg = packet[5];
  bool night = (cmd & V2_NIGHT_MODE_FLAG) != 0;
  cmd &= 0x7F;

  if (night) {
    out.night_mode = true;
    // Still need to set remote_type based on protocol_id
    if (protocol_id == V2_RGB_CCT_PROTOCOL_ID) {
      out.remote_type = RemoteType::RGB_CCT;
    } else if (protocol_id == V2_FUT089_PROTOCOL_ID) {
      out.remote_type = RemoteType::FUT089;
    } else if (protocol_id == V2_FUT091_PROTOCOL_ID) {
      out.remote_type = RemoteType::FUT091;
    } else {
      return false;
    }
    return true;
  }

  if (protocol_id == V2_RGB_CCT_PROTOCOL_ID) {
    decode_rgb_cct_v2(cmd, arg, out);
  } else if (protocol_id == V2_FUT089_PROTOCOL_ID) {
    decode_fut089_v2(cmd, arg, out);
  } else if (protocol_id == V2_FUT091_PROTOCOL_ID) {
    decode_fut091_v2(cmd, arg, out);
  } else {
    return false;
  }

  return true;
}

// ---- Public decode_packet dispatcher ----

bool decode_packet(const uint8_t *packet, uint8_t length, uint8_t radio_config_index, ReceivedCommand &out) {
  out = ReceivedCommand{};

  switch (radio_config_index) {
    case 0:
      return decode_rgbw(packet, length, out);
    case 1:
      return decode_cct(packet, length, out);
    case 2: {
      // V2: need to decode in place, so copy first
      uint8_t buf[MAX_PACKET_SIZE];
      uint8_t copy_len = length > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : length;
      memcpy(buf, packet, copy_len);
      return decode_v2_common(buf, length, out);
    }
    case 3:
      return decode_rgb(packet, length, out);
    case 4:
      return decode_fut020(packet, length, out);
    default:
      return false;
  }
}

}  // namespace esphome::milight
