#include "daikin_altherma.h"
#include <algorithm>
#include "esphome/core/log.h"

namespace esphome::daikin_altherma {

static const char *const TAG = "daikin_altherma";

void DaikinAltherma::setup() { ESP_LOGCONFIG(TAG, "Setting up Daikin Altherma..."); }

void DaikinAltherma::dump_config() {
  ESP_LOGCONFIG(TAG,
                "Daikin Altherma:"
                "\n  Registers polled: %d"
                "\n  Response timeout: %ums",
                REGISTERS_.size(), RESPONSE_TIMEOUT_MS);
#ifdef USE_SENSOR
  // Register 0x10
  LOG_SENSOR("  ", "0x10[6:7] Target Evaporator Temperature", this->target_evaporator_temperature_sensor_);
  LOG_SENSOR("  ", "0x10[8:9] Target Cond. Temperature", this->target_cond_temperature_sensor_);
  // Register 0x20
  LOG_SENSOR("  ", "0x20[0:1] Outdoor Air Temperature", this->outdoor_air_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[2:3] Outdoor Heat Exchanger Temperature",
             this->r20_outdoor_heat_exchanger_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[4:5] Discharge Pipe Temperature", this->discharge_pipe_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[6:7] Suction Pipe Temperature", this->suction_pipe_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[8:9] Outdoor Heat Exchanger Mid Temperature",
             this->outdoor_heat_exchanger_mid_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[10:11] Liquid Pipe Temperature", this->r20_liquid_pipe_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[12:13] High Pressure", this->r20_high_pressure_sensor_);
  LOG_SENSOR("  ", "0x20[14:15] Low Pressure", this->r20_low_pressure_sensor_);
  LOG_SENSOR("  ", "Condensing Temperature (R32 sat. from high pressure)", this->condensing_temperature_sensor_);
  LOG_SENSOR("  ", "Evaporating Temperature (R32 sat. from low pressure)", this->evaporating_temperature_sensor_);
  LOG_SENSOR("  ", "0x20[16] Unknown", this->r20_16_unknown_sensor_);
  // Register 0x21
  LOG_SENSOR("  ", "0x21[0:1] INV Primary Current", this->inv_primary_current_sensor_);
  LOG_SENSOR("  ", "0x21[2:3] INV Secondary Current", this->inv_secondary_current_sensor_);
  LOG_SENSOR("  ", "0x21[4:5] INV Fin Temperature", this->inv_fin_temperature_sensor_);
  LOG_SENSOR("  ", "0x21[12:13] Unknown", this->r21_1213_unknown_sensor_);
  // Register 0x30
  LOG_SENSOR("  ", "0x30[0] Compressor Frequency", this->compressor_frequency_sensor_);
  LOG_SENSOR("  ", "0x30[1] Fan Speed", this->fan_speed_sensor_);
  LOG_SENSOR("  ", "0x30[3:4] Expansion Valve Pulses", this->expansion_valve_pulses_sensor_);
  LOG_SENSOR("  ", "0x30[5] Unknown", this->r30_05_unknown_sensor_);
  // Register 0x60
  LOG_SENSOR("  ", "0x60[1] Indoor Unit Address", this->r60_indoor_unit_address_sensor_);
  LOG_SENSOR("  ", "0x60[3] Indoor Error Code", this->indoor_error_code_sensor_);
  LOG_SENSOR("  ", "0x60[4] Indoor Error Detailed Code", this->indoor_error_detailed_code_sensor_);
  LOG_SENSOR("  ", "0x60[5] Indoor Error Type", this->indoor_error_type_sensor_);
  LOG_SENSOR("  ", "0x60[6] Indoor Unit Capacity", this->indoor_unit_capacity_sensor_);
  LOG_SENSOR("  ", "0x60[7:8] DHW Tank Setpoint", this->dhw_tank_setpoint_sensor_);
  LOG_SENSOR("  ", "0x60[9:10] Leaving Water Setpoint", this->r60_leaving_water_setpoint_sensor_);
  LOG_SENSOR("  ", "0x60[13] Indoor Option Code", this->indoor_option_code_sensor_);
  LOG_SENSOR("  ", "0x60[14] Indoor Software Version", this->indoor_software_version_sensor_);
  LOG_SENSOR("  ", "0x60[15] Indoor EEPROM Version", this->indoor_eeprom_version_sensor_);
  LOG_SENSOR("  ", "0x60[16] I/U EEPROM Version", this->iu_eeprom_version_sensor_);
  // Register 0x61
  LOG_SENSOR("  ", "0x61[1] Indoor Unit Address", this->r61_indoor_unit_address_sensor_);
  LOG_SENSOR("  ", "0x61[2:3] Leaving Water Temperature Before BUH (R1T)",
             this->leaving_water_temperature_before_buh_sensor_);
  LOG_SENSOR("  ", "0x61[4:5] Leaving Water Temperature After BUH (R2T)",
             this->leaving_water_temperature_after_buh_sensor_);
  LOG_SENSOR("  ", "0x61[6:7] Refrigerant Liquid Temperature (R3T)", this->refrigerant_liquid_temperature_sensor_);
  LOG_SENSOR("  ", "0x61[8:9] Inlet Water Temperature (R4T)", this->inlet_water_temperature_sensor_);
  LOG_SENSOR("  ", "0x61[10:11] DHW Tank Temperature (R5T)", this->dhw_tank_temperature_sensor_);
  LOG_SENSOR("  ", "0x61[12:13] Room Temperature", this->room_temperature_sensor_);
  LOG_SENSOR("  ", "0x61[14:15] External Ambient Temperature (R6T)", this->external_ambient_temperature_sensor_);
  // Register 0x62
  LOG_SENSOR("  ", "0x62[1] Indoor Unit Address", this->r62_indoor_unit_address_sensor_);
  LOG_SENSOR("  ", "0x62[3:4] Leaving Water Setpoint", this->r62_leaving_water_setpoint_sensor_);
  LOG_SENSOR("  ", "0x62[5:6] Room Temperature Setpoint", this->room_temperature_setpoint_sensor_);
  LOG_SENSOR("  ", "0x62[9:10] Flow Rate", this->flow_rate_sensor_);
  LOG_SENSOR("  ", "0x62[11] Water Pressure", this->water_pressure_sensor_);
  LOG_SENSOR("  ", "0x62[12] Pump Speed", this->pump_speed_sensor_);
  // Register 0x63
  LOG_SENSOR("  ", "0x63[1] Indoor Unit Address", this->r63_indoor_unit_address_sensor_);
  // Register 0x64
  LOG_SENSOR("  ", "0x64[1] Indoor Unit Address", this->r64_indoor_unit_address_sensor_);
  LOG_SENSOR("  ", "0x64[7:8] Unknown", this->r64_0708_unknown_sensor_);
  LOG_SENSOR("  ", "0x64[9] Unknown Pump", this->r64_09_unknown_pump_sensor_);
  LOG_SENSOR("  ", "0x64[12] Unknown", this->r64_12_unknown_sensor_);
  // Register 0x65
  LOG_SENSOR("  ", "0x65[1] Indoor Unit Address", this->r65_indoor_unit_address_sensor_);
  LOG_SENSOR("  ", "0x65[2:3] Outlet Water HX Temperature", this->outlet_water_hx_temperature_sensor_);
  // Register 0xA0
  LOG_SENSOR("  ", "0xA0[0:1] Suction Temperature", this->suction_temperature_sensor_);
  LOG_SENSOR("  ", "0xA0[2:3] Outdoor Heat Exchanger Temperature",
             this->ra0_outdoor_heat_exchanger_temperature_sensor_);
  LOG_SENSOR("  ", "0xA0[4:5] Liquid Pipe Temperature", this->ra0_liquid_pipe_temperature_sensor_);
  LOG_SENSOR("  ", "0xA0[6:7] Pressure", this->ra0_pressure_sensor_);
  LOG_SENSOR("  ", "0xA0[8:9] Expansion Valve 3", this->expansion_valve_3_sensor_);
  LOG_SENSOR("  ", "0xA0[14:15] Compressor Port Temperature", this->compressor_port_temperature_sensor_);
  // Register 0xA1
  LOG_SENSOR("  ", "0xA1[4] Unknown", this->ra1_04_unknown_sensor_);
  LOG_SENSOR("  ", "0xA1[5:6] Unknown", this->ra1_0506_unknown_sensor_);
#endif
#ifdef USE_BINARY_SENSOR
  // Register 0x10
  LOG_BINARY_SENSOR("  ", "0x10[1] bit7 Thermostat ON", this->thermostat_on_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit6 Restart Standby", this->restart_standby_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit5 Startup Control", this->startup_control_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit4 Defrost Operation", this->defrost_operation_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit3 Oil Return Operation", this->oil_return_operation_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit2 Pressure Equalizing", this->pressure_equalizing_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit1 Demand Signal", this->demand_signal_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[1] bit0 Low Noise Control", this->low_noise_control_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x10[11] bit7 HP Drop Control", this->hp_drop_control_binary_sensor_);
  // Register 0x30
  LOG_BINARY_SENSOR("  ", "0x30[11] bit7 Four Way Valve", this->four_way_valve_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x30[13] bit7 Hot gas bypass valve (Y3S)", this->hot_gas_bypass_valve_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x30[13] bit6 LP bypass valve (Y2S)", this->y2s_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x30[13] bit5 Y3S", this->y3s_binary_sensor_);
  // Register 0x60
  LOG_BINARY_SENSOR("  ", "0x60[0] bit7 Data Enabled", this->r60_data_enabled_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[2] bit3 Indoor Thermostat ON", this->indoor_thermostat_on_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[2] bit2 Freeze Protection", this->freeze_protection_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[2] bit1 Silent Mode", this->silent_mode_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[2] bit0 Freeze Prot. Water Piping",
                    this->freeze_protection_water_piping_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[11] bit6 Thermal Protector (Q1L) BUH", this->thermal_protector_q1l_buh_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[11] bit5 Thermal Protector BSH", this->thermal_protector_bsh_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[11] bit4 External Heat Source", this->external_heat_source_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit7 2|3 Way Valve", this->two_three_way_valve_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit6 3|4 Way Valve", this->three_four_way_valve_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit5 Booster Heater", this->booster_heater_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit4 Backup Heater Step 1", this->backup_heater_step1_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit3 Backup Heater Step 2", this->backup_heater_step2_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit2 Bottom Plate Heater", this->bottom_plate_heater_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit1 Water Pump", this->water_pump_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x60[12] bit0 Solar Pump", this->solar_pump_binary_sensor_);
  // Register 0x61
  LOG_BINARY_SENSOR("  ", "0x61[0] bit7 Data Enabled", this->r61_data_enabled_binary_sensor_);
  // Register 0x62
  LOG_BINARY_SENSOR("  ", "0x62[0] bit7 Data Enabled", this->r62_data_enabled_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x62[2] bit4 Powerful DHW", this->powerful_dhw_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x62[2] bit3 Space Heating", this->space_heating_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x62[8] bit7 Unknown", this->r62_08_bit7_unknown_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x62[8] bit6 Unknown", this->r62_08_bit6_unknown_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x62[8] bit5 Unknown", this->r62_08_bit5_unknown_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "0x62[8] bit0 Space H Operation Output", this->space_h_operation_output_binary_sensor_);
  // Register 0x63
  LOG_BINARY_SENSOR("  ", "0x63[0] bit7 Data Enabled", this->r63_data_enabled_binary_sensor_);
  // Register 0x64
  LOG_BINARY_SENSOR("  ", "0x64[0] bit7 Data Enabled", this->r64_data_enabled_binary_sensor_);
  // Register 0x65
  LOG_BINARY_SENSOR("  ", "0x65[0] bit7 Data Enabled", this->r65_data_enabled_binary_sensor_);
#endif
#ifdef USE_TEXT_SENSOR
  // Register 0x10
  LOG_TEXT_SENSOR("  ", "0x10[0] Operation Mode", this->operation_mode_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x10[4] Error Type", this->error_type_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x10[5] Error Code", this->error_code_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x10[11] Flags", this->r10_11_flags_text_sensor_);
  // Register 0x30
  LOG_TEXT_SENSOR("  ", "0x30[11] Flags", this->r30_11_flags_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x30[13] Flags", this->r30_13_flags_text_sensor_);
  // Register 0x60
  LOG_TEXT_SENSOR("  ", "0x60[2] Indoor Operation Mode", this->indoor_operation_mode_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x60[11] Flags", this->r60_11_flags_text_sensor_);
  // Register 0x62
  LOG_TEXT_SENSOR("  ", "0x62[2] Flags", this->r62_02_flags_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x62[7] Flags", this->r62_07_flags_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x62[8] Flags", this->r62_08_flags_text_sensor_);
  // Register 0x63
  LOG_TEXT_SENSOR("  ", "0x63[2:7] Indoor EEPROM", this->indoor_eeprom_text_sensor_);
  // Raw register dumps
  LOG_TEXT_SENSOR("  ", "0x00 Raw", this->raw_0x00_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x10 Raw", this->raw_0x10_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x11 Raw", this->raw_0x11_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x20 Raw", this->raw_0x20_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x21 Raw", this->raw_0x21_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x30 Raw", this->raw_0x30_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x60 Raw", this->raw_0x60_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x61 Raw", this->raw_0x61_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x62 Raw", this->raw_0x62_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x63 Raw", this->raw_0x63_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x64 Raw", this->raw_0x64_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0x65 Raw", this->raw_0x65_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0xA0 Raw", this->raw_0xa0_text_sensor_);
  LOG_TEXT_SENSOR("  ", "0xA1 Raw", this->raw_0xa1_text_sensor_);
#endif
}

void DaikinAltherma::update() {
  if (this->state_ != State::IDLE) {
    ESP_LOGW(TAG, "Previous polling cycle not finished, skipping");
    return;
  }

  // Flush any stale data from UART buffer
  while (this->available()) {
    uint8_t b;
    this->read_byte(&b);
  }

  this->register_index_ = 0;
  this->rx_pos_ = 0;
  this->send_request_(REGISTERS_[0]);
}

void DaikinAltherma::loop() {
  if (this->state_ != State::WAITING_RESPONSE)
    return;

  // Check timeout
  if (millis() - this->last_send_ms_ > RESPONSE_TIMEOUT_MS) {
    ESP_LOGW(TAG, "Timeout waiting for register 0x%02X", static_cast<uint8_t>(REGISTERS_[this->register_index_]));
    this->rx_pos_ = 0;
    this->advance_register_();
    return;
  }

  // Read all available bytes in batches to reduce UART call overhead
  size_t avail = this->available();
  uint8_t buf[32];
  while (avail > 0) {
    size_t to_read = std::min(avail, sizeof(buf));
    if (!this->read_array(buf, to_read)) {
      break;
    }
    avail -= to_read;

    for (size_t i = 0; i < to_read; i++) {
      uint8_t byte = buf[i];

      // Wait for 0x40 start marker
      if (this->rx_pos_ == 0 && byte != 0x40) {
        continue;
      }

      // Prevent buffer overflow
      if (this->rx_pos_ >= this->rx_buf_.size()) {
        ESP_LOGW(TAG, "RX buffer overflow, resetting");
        this->rx_pos_ = 0;
        this->advance_register_();
        return;
      }

      this->rx_buf_[this->rx_pos_++] = byte;

      // Need at least 3 bytes (marker, reg_id, length) to know frame size
      if (this->rx_pos_ < 3)
        continue;

      // Total frame length = length_field + 2 (marker + reg_id)
      uint8_t frame_len = this->rx_buf_[2] + 2;

      // Sanity check frame length
      if (frame_len > this->rx_buf_.size()) {
        ESP_LOGW(TAG, "Invalid frame length %d, resetting", frame_len);
        this->rx_pos_ = 0;
        this->advance_register_();
        return;
      }

      // Wait for complete frame
      if (this->rx_pos_ < frame_len)
        continue;

        // Complete frame received - Log frame
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
      char rx_hex[format_hex_pretty_size(sizeof(this->rx_buf_))];
      ESP_LOGV(TAG, "RX: [%s]", format_hex_pretty_to(rx_hex, this->rx_buf_.data(), frame_len, ':'));
#endif

      // Validate checksum
      uint8_t expected_crc = checksum_(this->rx_buf_.data(), frame_len - 1);
      uint8_t actual_crc = this->rx_buf_[frame_len - 1];

      if (expected_crc != actual_crc) {
        ESP_LOGW(TAG, "Checksum mismatch for register 0x%02X: expected 0x%02X, got 0x%02X", this->rx_buf_[1],
                 expected_crc, actual_crc);
      } else {
        Register reg_id = static_cast<Register>(this->rx_buf_[1]);
        Register expected_reg = REGISTERS_[this->register_index_];

        if (reg_id != expected_reg) {
          ESP_LOGW(TAG, "Unexpected register 0x%02X, expected 0x%02X", static_cast<uint8_t>(reg_id),
                   static_cast<uint8_t>(expected_reg));
        } else {
          // Data starts at offset 3, length = length_field - 2
          uint8_t data_len = this->rx_buf_[2] - 2;
          this->parse_register_(reg_id, this->rx_buf_.data() + 3, data_len);
          ESP_LOGV(TAG, "Parsed register 0x%02X (%d data bytes)", static_cast<uint8_t>(reg_id), data_len);
        }
      }

      this->rx_pos_ = 0;
      this->advance_register_();
      return;
    }
  }
}

void DaikinAltherma::send_request_(Register reg) {
  // Protocol I request: {0x03, 0x40, register_id, CRC}
  uint8_t request[4] = {0x03, 0x40, static_cast<uint8_t>(reg), 0x00};
  request[3] = checksum_(request, 3);

#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  char tx_hex[format_hex_pretty_size(sizeof(request))];
  ESP_LOGV(TAG, "TX: [%s]", format_hex_pretty_to(tx_hex, request, sizeof(request), ':'));
#endif

  this->write_array(request, sizeof(request));
  this->flush();

  this->state_ = State::WAITING_RESPONSE;
  this->last_send_ms_ = millis();
  this->rx_pos_ = 0;
}

void DaikinAltherma::advance_register_() {
  this->register_index_++;
  if (this->register_index_ >= REGISTERS_.size()) {
    this->state_ = State::IDLE;
    this->register_index_ = 0;
  } else {
    this->send_request_(REGISTERS_[this->register_index_]);
  }
}

uint8_t DaikinAltherma::checksum_(const uint8_t *data, uint8_t len) {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return ~sum;
}

std::string DaikinAltherma::format_hex_masked_(const uint8_t *data, uint8_t len,
                                               std::initializer_list<uint8_t> mask_indices) {
  std::string result;
  result.reserve(len * 3);  // "XX:" per byte
  for (uint8_t i = 0; i < len; i++) {
    if (i > 0)
      result += ':';
    bool masked = false;
    for (uint8_t idx : mask_indices) {
      if (idx == i) {
        masked = true;
        break;
      }
    }
    if (masked) {
      result += "XX";
    } else {
      static const char *const HEX_CHARS = "0123456789ABCDEF";
      result += HEX_CHARS[data[i] >> 4];
      result += HEX_CHARS[data[i] & 0x0F];
    }
  }
  return result;
}

std::string DaikinAltherma::format_bits_masked_(uint8_t value, std::initializer_list<uint8_t> known_bits) {
  // Format byte as binary string with known bits masked as 'X'
  // known_bits: list of bit indices (0-7) that are known and will be shown as 'X'
  std::string result;
  result.reserve(8);
  for (int i = 7; i >= 0; i--) {
    bool masked = false;
    for (uint8_t bit : known_bits) {
      if (bit == i) {
        masked = true;
        break;
      }
    }
    if (masked) {
      result += 'X';
    } else {
      result += (value & (1 << i)) ? '1' : '0';
    }
  }
  return result;
}

std::string DaikinAltherma::format_indoor_eeprom_(const uint8_t *data) {
  // Indoor unit EEPROM number, e.g. "1709433-12C". data points at 0x63[2]:
  // data[0:4] are BCD digit pairs, data[5] is the revision letter (1=A, 2=B, 3=C, ...).
  char rev = (data[5] >= 1 && data[5] <= 26) ? static_cast<char>('A' + data[5] - 1) : '?';
  // 9 nibbles (each 0-15, so up to 2 chars) + '-' + revision char + null terminator
  char buf[21];
  snprintf(buf, sizeof(buf), "%d%d%d%d%d%d%d-%d%d%c", data[0] & 0x0F, data[1] >> 4, data[1] & 0x0F, data[2] >> 4,
           data[2] & 0x0F, data[3] >> 4, data[3] & 0x0F, data[4] >> 4, data[4] & 0x0F, rev);
  return buf;
}

float DaikinAltherma::decode_int16_div10_(const uint8_t *data) {
  // Little-endian signed 16-bit, divided by 10
  int16_t raw = static_cast<int16_t>(data[0] | (data[1] << 8));
  return raw * 0.1f;
}

float DaikinAltherma::decode_press2temp_r32_(const uint8_t *data) {
  // ESPAltherma converter 405: pressure (int16 LE * 0.1 bar) -> R32 saturation temperature (degC)
  // via 6th-order polynomial. Returns NAN when no valid pressure (compressor off / register zeroed).
  double p = decode_int16_div10_(data);
  if (p <= 0.0)
    return NAN;
  // Horner form of the ESPAltherma R32 polynomial
  double t =
      ((((((-2.6989493795556E-07 * p + 4.26383417104661E-05) * p - 0.00262978346547749) * p + 0.0805858127503585) * p -
         1.31924457284073) *
            p +
        13.4157368435437) *
           p -
       51.1813342993155);
  return static_cast<float>(t);
}

float DaikinAltherma::decode_fixed_point_le_(const uint8_t *data) {
  // Fixed-point: integer=data[1], fractional=(data[0]&0x7F)/256; {0x00,0x80}=N/A
  if (data[0] == 0x00 && data[1] == 0x80)
    return NAN;
  return data[1] + (data[0] & 0x7F) / 256.0f;
}

float DaikinAltherma::decode_fixed_point_signed_le_x10_(const uint8_t *data) {
  // Signed Q8.8 LE fixed-point × 10; {0x00,0x80}=N/A
  // Format: int16 = data[1]<<8|data[0], value = int16/256.0 * 10.0
  if (data[0] == 0x00 && data[1] == 0x80)
    return NAN;
  return static_cast<int16_t>(data[0] | (data[1] << 8)) / 256.0f * 10.0f;
}

const char *DaikinAltherma::decode_0x10_operation_mode_(uint8_t mode_byte) {
  // R0x10[0] operation mode index decoding
  static const char *const MODES[] = {
      "Fan Only",          // 0
      "Heating",           // 1
      "Cooling",           // 2
      "Auto",              // 3
      "Ventilation",       // 4
      "Auto Cool",         // 5
      "Auto Heat",         // 6
      "Dry",               // 7
      "Aux.",              // 8
      "Cooling Storage",   // 9
      "Heating Storage",   // 10
      "UseStrdThrm(cl)1",  // 11
      "UseStrdThrm(cl)2",  // 12
      "UseStrdThrm(cl)3",  // 13
      "UseStrdThrm(cl)4",  // 14
      "UseStrdThrm(ht)1",  // 15
      "UseStrdThrm(ht)2",  // 16
      "UseStrdThrm(ht)3",  // 17
      "UseStrdThrm(ht)4",  // 18
  };
  if (mode_byte < sizeof(MODES) / sizeof(MODES[0])) {
    return MODES[mode_byte];
  }
  return "Unknown";
}

const char *DaikinAltherma::decode_0x10_error_type_(uint8_t error_byte) {
  // R0x10[4] error type decoding
  static const char *const ERROR_TYPES[] = {
      "Normal",   // 0
      "Error",    // 1
      "Warning",  // 2
      "Caution",  // 3
      "-",        // 4
  };
  if (error_byte < sizeof(ERROR_TYPES) / sizeof(ERROR_TYPES[0])) {
    return ERROR_TYPES[error_byte];
  }
  return "Unknown";
}

std::string DaikinAltherma::decode_0x10_error_code_(uint8_t code_byte) {
  // R0x10[5] error code decoding - nibble-based lookup
  static const char FIRST_CHAR[] = " ACEHFJLPU987654";
  static const char SECOND_CHAR[] = "0123456789AHCJEF";
  std::string result;
  result.reserve(2);
  result += FIRST_CHAR[(code_byte >> 4) & 0x0F];
  result += SECOND_CHAR[code_byte & 0x0F];
  result += "0";
  return result;
}

const char *DaikinAltherma::decode_0x60_operation_mode_(uint8_t mode_byte) {
  // R0x60[2] mode byte decoding - upper nibble contains operation mode
  uint8_t mode = (mode_byte & 0xF0) >> 4;
  switch (mode) {
    case 0:
      return "Stop";
    case 1:
      return "Heating";
    case 2:
      return "Cooling";
    case 3:
      return "??";
    case 4:
      return "DHW";
    case 5:
      return "Heating + DHW";
    case 6:
      return "Cooling + DHW";
    default:
      return "-";
  }
}

void DaikinAltherma::parse_register_(Register reg, const uint8_t *data, uint8_t data_len) {
  switch (reg) {
    case Register::R0X00_UNKNOWN:
      this->parse_0x00_unknown_(data, data_len);
      break;
    case Register::R0X10_OUTDOOR_CONTROL:
      this->parse_0x10_outdoor_control_(data, data_len);
      break;
    case Register::R0X11_EEPROM:
      this->parse_0x11_eeprom_(data, data_len);
      break;
    case Register::R0X20_OUTDOOR_TEMPERATURE:
      this->parse_0x20_outdoor_temperature_(data, data_len);
      break;
    case Register::R0X21_UNKNOWN:
      this->parse_0x21_unknown_(data, data_len);
      break;
    case Register::R0X30_OUTDOOR_UNIT:
      this->parse_0x30_outdoor_unit_(data, data_len);
      break;
    case Register::R0X60_INDOOR_CONTROL:
      this->parse_0x60_indoor_control_(data, data_len);
      break;
    case Register::R0X61_INDOOR_WATER_TEMPERATURE:
      this->parse_0x61_indoor_water_temperature_(data, data_len);
      break;
    case Register::R0X62_CONTROL_FLOW:
      this->parse_0x62_control_flow_(data, data_len);
      break;
    case Register::R0X63_UNKNOWN:
      this->parse_0x63_unknown_(data, data_len);
      break;
    case Register::R0X64_UNKNOWN:
      this->parse_0x64_unknown_(data, data_len);
      break;
    case Register::R0X65_INDOOR_OUTLET:
      this->parse_0x65_indoor_outlet_(data, data_len);
      break;
    case Register::R0XA0_OUTDOOR_REFRIGERANT:
      this->parse_0xa0_outdoor_refrigerant_(data, data_len);
      break;
    case Register::R0XA1_UNKNOWN:
      this->parse_0xa1_unknown_(data, data_len);
      break;
    default:
      break;
  }
}

void DaikinAltherma::parse_0x00_unknown_(const uint8_t *data, uint8_t data_len) {
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x00_text_sensor_ != nullptr)
    this->raw_0x00_text_sensor_->publish_state(format_hex_masked_(data, data_len, {}));
#endif
}

void DaikinAltherma::parse_0x10_outdoor_control_(const uint8_t *data, uint8_t data_len) {
  // [0]   = Operation Mode (index into mode strings)
  // [1]   = Status flags:
  //         bit7: Thermostat ON/OFF
  //         bit6: Restart standby
  //         bit5: Startup Control
  //         bit4: Defrost Operation
  //         bit3: Oil Return Operation
  //         bit2: Pressure equalizing operation
  //         bit1: Demand Signal
  //         bit0: Low noise control
  // [2]   = Unknown byte
  // [3]   = Unknown byte
  // [4]   = Error Type (0=Normal, 1=Error, 2=Warning, 3=Caution, 4=-)
  // [5]   = Error Code (nibble-based: upper->" ACEHFJLPU987654", lower->"0123456789AHCJEF")
  // [6:7] = Target Evaporator Temperature (signed Q8.8 LE × 10, °C)
  // [8:9] = Target Cond. Temperature (signed Q8.8 LE × 10, °C)
  // [10]  = Unknown byte
  // [11]  = bit7: HP drop control
  // [12]  = Unknown byte
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x10_text_sensor_ != nullptr)
    this->raw_0x10_text_sensor_->publish_state(format_hex_masked_(data, data_len, {0, 1, 4, 5, 6, 7, 8, 9, 11}));
  if (this->operation_mode_text_sensor_ != nullptr)
    this->operation_mode_text_sensor_->publish_state(decode_0x10_operation_mode_(data[0]));
#endif
#ifdef USE_BINARY_SENSOR
  uint8_t flags = data[1];
  if (this->thermostat_on_binary_sensor_ != nullptr)
    this->thermostat_on_binary_sensor_->publish_state(flags & 0x80);
  if (this->restart_standby_binary_sensor_ != nullptr)
    this->restart_standby_binary_sensor_->publish_state(flags & 0x40);
  if (this->startup_control_binary_sensor_ != nullptr)
    this->startup_control_binary_sensor_->publish_state(flags & 0x20);
  if (this->defrost_operation_binary_sensor_ != nullptr)
    this->defrost_operation_binary_sensor_->publish_state(flags & 0x10);
  if (this->oil_return_operation_binary_sensor_ != nullptr)
    this->oil_return_operation_binary_sensor_->publish_state(flags & 0x08);
  if (this->pressure_equalizing_binary_sensor_ != nullptr)
    this->pressure_equalizing_binary_sensor_->publish_state(flags & 0x04);
  if (this->demand_signal_binary_sensor_ != nullptr)
    this->demand_signal_binary_sensor_->publish_state(flags & 0x02);
  if (this->low_noise_control_binary_sensor_ != nullptr)
    this->low_noise_control_binary_sensor_->publish_state(flags & 0x01);
  if (this->hp_drop_control_binary_sensor_ != nullptr)
    this->hp_drop_control_binary_sensor_->publish_state((data[11] & 0x80) != 0);
#endif
#ifdef USE_TEXT_SENSOR
  if (this->error_type_text_sensor_ != nullptr)
    this->error_type_text_sensor_->publish_state(decode_0x10_error_type_(data[4]));
  if (this->error_code_text_sensor_ != nullptr)
    this->error_code_text_sensor_->publish_state(decode_0x10_error_code_(data[5]));
  // Known bits: 7=HP drop control
  if (this->r10_11_flags_text_sensor_ != nullptr)
    this->r10_11_flags_text_sensor_->publish_state(format_bits_masked_(data[11], {7}));
#endif
#ifdef USE_SENSOR
  if (this->target_evaporator_temperature_sensor_ != nullptr)
    this->target_evaporator_temperature_sensor_->publish_state(decode_fixed_point_signed_le_x10_(data + 6));
  if (this->target_cond_temperature_sensor_ != nullptr)
    this->target_cond_temperature_sensor_->publish_state(decode_fixed_point_signed_le_x10_(data + 8));
#endif
}

void DaikinAltherma::parse_0x11_eeprom_(const uint8_t *data, uint8_t data_len) {
  // [0] = O/U EEPROM (1. digit)
  // [1] = O/U EEPROM (3+4. digit)
  // [2] = O/U EEPROM (5+6. digit)
  // [3] = O/U EEPROM (7+8. digit)
  // [4] = O/U EEPROM (10. digit)
  // [5] = O/U EEPROM (11. digit)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x11_text_sensor_ != nullptr)
    this->raw_0x11_text_sensor_->publish_state(format_hex_masked_(data, data_len, {}));
#endif
}

void DaikinAltherma::parse_0x20_outdoor_temperature_(const uint8_t *data, uint8_t data_len) {
  // [0:1]   = Outdoor air temperature (int16 LE * 0.1 degC) // FIX {Außentemperatur}
  // [2:3]   = Outdoor heat exchanger temperature (int16 LE * 0.1 degC)
  // [4:5]   = Discharge pipe temperature (int16 LE * 0.1 degC)
  // [6:7]   = Suction pipe temperature (int16 LE * 0.1 degC)
  // [8:9]   = Outdoor heat exchanger mid temperature (int16 LE * 0.1 degC)
  // [10:11] = Liquid pipe temperature (R6T) (int16 LE * 0.1 degC)
  // [12:13] = High Pressure (int16 LE * 0.1 bar)
  // [14:15] = Low Pressure (int16 LE * 0.1 bar)
  // [16]    = Unknown byte
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x20_text_sensor_ != nullptr)
    this->raw_0x20_text_sensor_->publish_state(format_hex_masked_(data, data_len, {}));  // Full supported
#endif
#ifdef USE_SENSOR
  if (this->outdoor_air_temperature_sensor_ != nullptr)
    this->outdoor_air_temperature_sensor_->publish_state(decode_int16_div10_(data + 0));
  if (this->r20_outdoor_heat_exchanger_temperature_sensor_ != nullptr)
    this->r20_outdoor_heat_exchanger_temperature_sensor_->publish_state(decode_int16_div10_(data + 2));
  if (this->discharge_pipe_temperature_sensor_ != nullptr)
    this->discharge_pipe_temperature_sensor_->publish_state(decode_int16_div10_(data + 4));
  if (this->suction_pipe_temperature_sensor_ != nullptr)
    this->suction_pipe_temperature_sensor_->publish_state(decode_int16_div10_(data + 6));
  if (this->outdoor_heat_exchanger_mid_temperature_sensor_ != nullptr)
    this->outdoor_heat_exchanger_mid_temperature_sensor_->publish_state(decode_int16_div10_(data + 8));
  if (this->r20_liquid_pipe_temperature_sensor_ != nullptr)
    this->r20_liquid_pipe_temperature_sensor_->publish_state(decode_int16_div10_(data + 10));
  if (this->r20_high_pressure_sensor_ != nullptr)
    this->r20_high_pressure_sensor_->publish_state(decode_int16_div10_(data + 12));
  if (this->r20_low_pressure_sensor_ != nullptr)
    this->r20_low_pressure_sensor_->publish_state(decode_int16_div10_(data + 14));
  if (this->condensing_temperature_sensor_ != nullptr)
    this->condensing_temperature_sensor_->publish_state(decode_press2temp_r32_(data + 12));
  if (this->evaporating_temperature_sensor_ != nullptr)
    this->evaporating_temperature_sensor_->publish_state(decode_press2temp_r32_(data + 14));
  if (this->r20_16_unknown_sensor_ != nullptr)
    this->r20_16_unknown_sensor_->publish_state(data[16]);
#endif
}

void DaikinAltherma::parse_0x21_unknown_(const uint8_t *data, uint8_t data_len) {
  // [0:1]   = INV primary current (int16 LE * 0.1 A)
  // [2:3]   = INV secondary current (int16 LE * 0.1 A)
  // [4:5]   = INV fin temperature (int16 LE * 0.1 degC)
  // [7:8]   = Brine inlet temperature (int16 LE * 0.1 degC)
  // [9:10]  = Brine outlet temperature (int16 LE * 0.1 degC)
  // [12:13] = Unknown (int16 LE * 0.1 degC)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x21_text_sensor_ != nullptr)
    this->raw_0x21_text_sensor_->publish_state(format_hex_masked_(data, data_len, {0, 1, 2, 3, 4, 5, 12, 13}));
#endif
#ifdef USE_SENSOR
  if (this->inv_primary_current_sensor_ != nullptr)
    this->inv_primary_current_sensor_->publish_state(decode_int16_div10_(data + 0));
  if (this->inv_secondary_current_sensor_ != nullptr)
    this->inv_secondary_current_sensor_->publish_state(decode_int16_div10_(data + 2));
  if (this->inv_fin_temperature_sensor_ != nullptr)
    this->inv_fin_temperature_sensor_->publish_state(decode_int16_div10_(data + 4));
  if (this->r21_1213_unknown_sensor_ != nullptr)
    this->r21_1213_unknown_sensor_->publish_state(decode_int16_div10_(data + 12));
#endif
}

void DaikinAltherma::parse_0x30_outdoor_unit_(const uint8_t *data, uint8_t data_len) {
  // [0]   = Compressor frequency (rps/Hz)
  // [1]   = Fan speed (step)
  // [3:4] = Expansion valve pulses (uint16 LE, no scaling)
  // [5]   = Unknown byte
  // [11]  = Outdoor unit flags (bit7: four way valve state)
  // [13]  = Outdoor unit flags (bit5: Y3S, bit6: LP bypass valve Y2S, bit7: hot gas bypass valve Y3S)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x30_text_sensor_ != nullptr)
    this->raw_0x30_text_sensor_->publish_state(format_hex_masked_(data, data_len, {0, 1, 3, 4, 5, 11, 13}));
  // Known bits: 7=four way valve
  if (this->r30_11_flags_text_sensor_ != nullptr)
    this->r30_11_flags_text_sensor_->publish_state(format_bits_masked_(data[11], {7}));
  // Known bits: 5=Y3S, 6=LP bypass valve Y2S, 7=hot gas bypass valve Y3S
  if (this->r30_13_flags_text_sensor_ != nullptr)
    this->r30_13_flags_text_sensor_->publish_state(format_bits_masked_(data[13], {5, 6, 7}));
#endif
#ifdef USE_SENSOR
  if (this->compressor_frequency_sensor_ != nullptr)
    this->compressor_frequency_sensor_->publish_state(data[0]);
  if (this->fan_speed_sensor_ != nullptr)
    this->fan_speed_sensor_->publish_state(data[1]);
  if (this->expansion_valve_pulses_sensor_ != nullptr)
    this->expansion_valve_pulses_sensor_->publish_state(static_cast<uint16_t>(data[3] | (data[4] << 8)));
  if (this->r30_05_unknown_sensor_ != nullptr)
    this->r30_05_unknown_sensor_->publish_state(data[5]);
#endif
#ifdef USE_BINARY_SENSOR
  if (this->four_way_valve_binary_sensor_ != nullptr)
    this->four_way_valve_binary_sensor_->publish_state((data[11] & 0x80) != 0);
  if (this->hot_gas_bypass_valve_binary_sensor_ != nullptr)
    this->hot_gas_bypass_valve_binary_sensor_->publish_state((data[13] & 0x80) != 0);
  if (this->y2s_binary_sensor_ != nullptr)
    this->y2s_binary_sensor_->publish_state((data[13] & 0x40) != 0);
  if (this->y3s_binary_sensor_ != nullptr)
    this->y3s_binary_sensor_->publish_state((data[13] & 0x20) != 0);
#endif
}

void DaikinAltherma::parse_0x60_indoor_control_(const uint8_t *data, uint8_t data_len) {
  // [0]    = bit 7: Data Enabled/Disabled
  // [1]    = Indoor Unit Address
  // [2]    = Operation mode byte:
  //          bits 0-3: flags (bit0=freeze prot water piping, bit1=silent, bit2=freeze prot, bit3=thermostat)
  //          bits 4-7: operation mode (0=stop, 1=heating, 2=cooling, 4=DHW, 5=heating+DHW, 6=cooling+DHW)
  // [3]    = Error Code
  // [4]    = Error Detailed Code
  // [5]    = Error Type
  // [6]    = Indoor Unit Capacity
  // [7:8]  = DHW tank setpoint (int16 LE * 0.1 degC) // FIX
  // [9:10] = Leaving water setpoint (int16 LE * 0.1 degC) // FIX (check offset 1°C)
  // [11]   = External heat source flags (bit4=external heat source active, bit5=thermal protector BSH, bit6=thermal
  //          protector Q1L BUH)
  // [12]   = Output actuator bitmask (bit0=solar pump, bit1=water pump, bit2=bottom plate heater, bit3=backup heater
  //          step 2, bit4=backup heater step 1, bit5=booster heater, bit6=3/4-way valve, bit7=2/3-way valve)
  // [13]   = Indoor Option Code
  // [14]   = Indoor Software Version
  // [15]   = Indoor EEPROM Version
  // [16]   = I/U EEPROM Version?
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x60_text_sensor_ != nullptr)
    this->raw_0x60_text_sensor_->publish_state(format_hex_masked_(data, data_len, {}));  // Full supported
  if (this->indoor_operation_mode_text_sensor_ != nullptr)
    this->indoor_operation_mode_text_sensor_->publish_state(decode_0x60_operation_mode_(data[2]));
  // Known bits: 4=external heat source, 5=thermal protector BSH, 6=thermal protector Q1L BUH
  if (this->r60_11_flags_text_sensor_ != nullptr)
    this->r60_11_flags_text_sensor_->publish_state(format_bits_masked_(data[11], {4, 5, 6}));
#endif
#ifdef USE_BINARY_SENSOR
  if (this->r60_data_enabled_binary_sensor_ != nullptr)
    this->r60_data_enabled_binary_sensor_->publish_state((data[0] & 0x80) != 0);
  uint8_t mode_flags = data[2];
  if (this->indoor_thermostat_on_binary_sensor_ != nullptr)
    this->indoor_thermostat_on_binary_sensor_->publish_state((mode_flags & 0x08) != 0);
  if (this->freeze_protection_binary_sensor_ != nullptr)
    this->freeze_protection_binary_sensor_->publish_state((mode_flags & 0x04) != 0);
  if (this->silent_mode_binary_sensor_ != nullptr)
    this->silent_mode_binary_sensor_->publish_state((mode_flags & 0x02) != 0);
  if (this->freeze_protection_water_piping_binary_sensor_ != nullptr)
    this->freeze_protection_water_piping_binary_sensor_->publish_state((mode_flags & 0x01) != 0);
#endif
#ifdef USE_SENSOR
  if (this->r60_indoor_unit_address_sensor_ != nullptr)
    this->r60_indoor_unit_address_sensor_->publish_state(data[1]);
  if (this->indoor_error_code_sensor_ != nullptr)
    this->indoor_error_code_sensor_->publish_state(data[3]);
  if (this->indoor_error_detailed_code_sensor_ != nullptr)
    this->indoor_error_detailed_code_sensor_->publish_state(data[4]);
  if (this->indoor_error_type_sensor_ != nullptr)
    this->indoor_error_type_sensor_->publish_state(data[5]);
  if (this->indoor_unit_capacity_sensor_ != nullptr)
    this->indoor_unit_capacity_sensor_->publish_state(data[6]);
  if (this->dhw_tank_setpoint_sensor_ != nullptr)
    this->dhw_tank_setpoint_sensor_->publish_state(decode_int16_div10_(data + 7));
  if (this->r60_leaving_water_setpoint_sensor_ != nullptr)
    this->r60_leaving_water_setpoint_sensor_->publish_state(decode_int16_div10_(data + 9));
  if (this->indoor_option_code_sensor_ != nullptr)
    this->indoor_option_code_sensor_->publish_state(data[13]);
  if (this->indoor_software_version_sensor_ != nullptr)
    this->indoor_software_version_sensor_->publish_state(data[14]);
  if (this->indoor_eeprom_version_sensor_ != nullptr)
    this->indoor_eeprom_version_sensor_->publish_state(data[15]);
  if (this->iu_eeprom_version_sensor_ != nullptr)
    this->iu_eeprom_version_sensor_->publish_state(data[16]);
#ifdef USE_BINARY_SENSOR
  if (this->external_heat_source_binary_sensor_ != nullptr)
    this->external_heat_source_binary_sensor_->publish_state((data[11] & 0x10) != 0);
  if (this->thermal_protector_bsh_binary_sensor_ != nullptr)
    this->thermal_protector_bsh_binary_sensor_->publish_state((data[11] & 0x20) != 0);
  if (this->thermal_protector_q1l_buh_binary_sensor_ != nullptr)
    this->thermal_protector_q1l_buh_binary_sensor_->publish_state((data[11] & 0x40) != 0);
  uint8_t actuators = data[12];
  if (this->solar_pump_binary_sensor_ != nullptr)
    this->solar_pump_binary_sensor_->publish_state((actuators & 0x01) != 0);
  if (this->water_pump_binary_sensor_ != nullptr)
    this->water_pump_binary_sensor_->publish_state((actuators & 0x02) != 0);
  if (this->bottom_plate_heater_binary_sensor_ != nullptr)
    this->bottom_plate_heater_binary_sensor_->publish_state((actuators & 0x04) != 0);
  if (this->backup_heater_step2_binary_sensor_ != nullptr)
    this->backup_heater_step2_binary_sensor_->publish_state((actuators & 0x08) != 0);
  if (this->backup_heater_step1_binary_sensor_ != nullptr)
    this->backup_heater_step1_binary_sensor_->publish_state((actuators & 0x10) != 0);
  if (this->booster_heater_binary_sensor_ != nullptr)
    this->booster_heater_binary_sensor_->publish_state((actuators & 0x20) != 0);
  if (this->three_four_way_valve_binary_sensor_ != nullptr)
    this->three_four_way_valve_binary_sensor_->publish_state((actuators & 0x40) != 0);
  if (this->two_three_way_valve_binary_sensor_ != nullptr)
    this->two_three_way_valve_binary_sensor_->publish_state((actuators & 0x80) != 0);
#endif
#endif
}

void DaikinAltherma::parse_0x61_indoor_water_temperature_(const uint8_t *data, uint8_t data_len) {
  // [0]     = bit7: Data Enable/Disable
  // [1]     = Indoor Unit Address
  // [2:3]   = Inlet water temperature backup heater/Leaving water temperature before BUH (R1T) (int16 LE * 0.1 degC)
  // {Wassertemperatur Einlass Reserveheizung?}
  // [4:5]   = Leaving water temperature after BUH (R2T) (int16 LE * 0.1 degC)
  // FIX {Vorlauftemperatur|Wassertemperatur Einlass Plattenwärmetauscher?}
  // [6:7]   = Refrigerant temperature liquid side (R3T) (int16 LE * 0.1 degC)
  // FIX {Kältemitteltemperatur}
  // [8:9]   = Inlet water temperature (R4T) (int16 LE * 0.1 degC)
  // [10:11] = DHW tank temperature (R5T) (int16 LE * 0.1 degC)
  // FIX {Speichertemperatur}
  // [12:13] = Room temperature/Indoor ambient temperature (R1T) (int16 LE * 0.1 degC)
  // FIX {Raumtemperatur}
  // [14:15] = External indoor/outdoor ambient sensor (R6T) (int16 LE * 0.1 degC)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x61_text_sensor_ != nullptr)
    this->raw_0x61_text_sensor_->publish_state(format_hex_masked_(data, data_len, {}));  // Full supported
#endif
#ifdef USE_BINARY_SENSOR
  if (this->r61_data_enabled_binary_sensor_ != nullptr)
    this->r61_data_enabled_binary_sensor_->publish_state((data[0] & 0x80) != 0);
#endif
#ifdef USE_SENSOR
  if (this->r61_indoor_unit_address_sensor_ != nullptr)
    this->r61_indoor_unit_address_sensor_->publish_state(data[1]);
  if (this->leaving_water_temperature_before_buh_sensor_ != nullptr)
    this->leaving_water_temperature_before_buh_sensor_->publish_state(decode_int16_div10_(data + 2));
  if (this->leaving_water_temperature_after_buh_sensor_ != nullptr)
    this->leaving_water_temperature_after_buh_sensor_->publish_state(decode_int16_div10_(data + 4));
  if (this->refrigerant_liquid_temperature_sensor_ != nullptr)
    this->refrigerant_liquid_temperature_sensor_->publish_state(decode_int16_div10_(data + 6));
  if (this->inlet_water_temperature_sensor_ != nullptr)
    this->inlet_water_temperature_sensor_->publish_state(decode_int16_div10_(data + 8));
  if (this->dhw_tank_temperature_sensor_ != nullptr)
    this->dhw_tank_temperature_sensor_->publish_state(decode_int16_div10_(data + 10));
  if (this->room_temperature_sensor_ != nullptr)
    this->room_temperature_sensor_->publish_state(decode_int16_div10_(data + 12));
  if (this->external_ambient_temperature_sensor_ != nullptr)
    this->external_ambient_temperature_sensor_->publish_state(decode_int16_div10_(data + 14));
#endif
}

void DaikinAltherma::parse_0x62_control_flow_(const uint8_t *data, uint8_t data_len) {
  // [0]    = bit7: Data Enable/Disable
  // [1]    = Indoor Unit Address
  // [2]    = Control flags (bit3=space heating, bit4=powerful DHW)
  // [3:4]  = Leaving water setpoint (int16 LE * 0.1 degC)
  // [5:6]  = Room temperature setpoint (int16 LE * 0.1 degC)
  // [7]    = Unknown flags
  // [8]    = Output flags (bit0=space H operation output, bit5=unknown)
  // [9:10] = Flow sensor (B2L) (int16 LE * 0.1 L/min) // FIX {Durchflussmenge}
  // [11]   = Water pressure (B1PW) (int16 LE * 0.1 kg/cm²) // FIX {Wasserdruck}
  // [12]   = Pump speed signal (0=max, 100=stop)
  // [13]   = 3 way Valve Mixing 1
  // [14]   = 3 way Valve Mixing 2
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x62_text_sensor_ != nullptr)
    this->raw_0x62_text_sensor_->publish_state(
        format_hex_masked_(data, data_len, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}));
#endif
#ifdef USE_BINARY_SENSOR
  if (this->r62_data_enabled_binary_sensor_ != nullptr)
    this->r62_data_enabled_binary_sensor_->publish_state((data[0] & 0x80) != 0);
  uint8_t flags = data[2];
  if (this->powerful_dhw_binary_sensor_ != nullptr)
    this->powerful_dhw_binary_sensor_->publish_state((flags & 0x10) != 0);
  if (this->space_heating_binary_sensor_ != nullptr)
    this->space_heating_binary_sensor_->publish_state((flags & 0x08) != 0);
  if (this->r62_08_bit7_unknown_binary_sensor_ != nullptr)
    this->r62_08_bit7_unknown_binary_sensor_->publish_state((data[8] & 0x80) != 0);
  if (this->r62_08_bit6_unknown_binary_sensor_ != nullptr)
    this->r62_08_bit6_unknown_binary_sensor_->publish_state((data[8] & 0x40) != 0);
  if (this->r62_08_bit5_unknown_binary_sensor_ != nullptr)
    this->r62_08_bit5_unknown_binary_sensor_->publish_state((data[8] & 0x20) != 0);
  if (this->space_h_operation_output_binary_sensor_ != nullptr)
    this->space_h_operation_output_binary_sensor_->publish_state((data[8] & 0x01) != 0);
#endif
#ifdef USE_TEXT_SENSOR
  // Known bits: 3=space heating, 4=powerful DHW
  if (this->r62_02_flags_text_sensor_ != nullptr)
    this->r62_02_flags_text_sensor_->publish_state(format_bits_masked_(data[2], {3, 4}));
  if (this->r62_07_flags_text_sensor_ != nullptr)
    this->r62_07_flags_text_sensor_->publish_state(format_bits_masked_(data[7], {}));
  // Known bits: 0=space H operation output, 5=unknown, 6=unknown, 7=unknown
  if (this->r62_08_flags_text_sensor_ != nullptr)
    this->r62_08_flags_text_sensor_->publish_state(format_bits_masked_(data[8], {0, 5, 6, 7}));
#endif
#ifdef USE_SENSOR
  if (this->r62_indoor_unit_address_sensor_ != nullptr)
    this->r62_indoor_unit_address_sensor_->publish_state(data[1]);
  if (this->r62_leaving_water_setpoint_sensor_ != nullptr)
    this->r62_leaving_water_setpoint_sensor_->publish_state(decode_int16_div10_(data + 3));
  if (this->room_temperature_setpoint_sensor_ != nullptr)
    this->room_temperature_setpoint_sensor_->publish_state(decode_int16_div10_(data + 5));
  if (this->flow_rate_sensor_ != nullptr)
    this->flow_rate_sensor_->publish_state(decode_int16_div10_(data + 9));
  if (this->water_pressure_sensor_ != nullptr)
    this->water_pressure_sensor_->publish_state(data[11] / 10.0f);
  if (this->pump_speed_sensor_ != nullptr)
    this->pump_speed_sensor_->publish_state(data[12]);
#endif
}

void DaikinAltherma::parse_0x63_unknown_(const uint8_t *data, uint8_t data_len) {
  // [0]   = bit7: Data Enable/Disable
  // [1]   = Indoor Unit Address
  // [2:7] = Indoor Unit EEPROM number (BCD digit pairs + revision letter)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x63_text_sensor_ != nullptr)
    this->raw_0x63_text_sensor_->publish_state(format_hex_masked_(data, data_len, {0, 1, 2, 3, 4, 5, 6, 7}));
  if (this->indoor_eeprom_text_sensor_ != nullptr && data_len > 7)
    this->indoor_eeprom_text_sensor_->publish_state(format_indoor_eeprom_(data + 2));
#endif
#ifdef USE_BINARY_SENSOR
  if (this->r63_data_enabled_binary_sensor_ != nullptr)
    this->r63_data_enabled_binary_sensor_->publish_state((data[0] & 0x80) != 0);
#endif
#ifdef USE_SENSOR
  if (this->r63_indoor_unit_address_sensor_ != nullptr)
    this->r63_indoor_unit_address_sensor_->publish_state(data[1]);
#endif
}

void DaikinAltherma::parse_0x64_unknown_(const uint8_t *data, uint8_t data_len) {
  // [0]   = bit7: Data Enable/Disable
  // [1]   = Indoor Unit Address
  // [7:8] = Unknown (int16 LE * 0.1)
  // [9]   = Unknown pump
  // [12]  = Unknown byte
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x64_text_sensor_ != nullptr)
    this->raw_0x64_text_sensor_->publish_state(format_hex_masked_(data, data_len, {0, 1, 7, 8, 9, 12}));
#endif
#ifdef USE_BINARY_SENSOR
  if (this->r64_data_enabled_binary_sensor_ != nullptr)
    this->r64_data_enabled_binary_sensor_->publish_state((data[0] & 0x80) != 0);
#endif
#ifdef USE_SENSOR
  if (this->r64_indoor_unit_address_sensor_ != nullptr)
    this->r64_indoor_unit_address_sensor_->publish_state(data[1]);
  if (this->r64_0708_unknown_sensor_ != nullptr)
    this->r64_0708_unknown_sensor_->publish_state(decode_int16_div10_(data + 7));
  if (this->r64_09_unknown_pump_sensor_ != nullptr)
    this->r64_09_unknown_pump_sensor_->publish_state(data[9]);
  if (this->r64_12_unknown_sensor_ != nullptr)
    this->r64_12_unknown_sensor_->publish_state(data[12]);
#endif
}

void DaikinAltherma::parse_0x65_indoor_outlet_(const uint8_t *data, uint8_t data_len) {
  // [0]   = bit7: Data Enable/Disable
  // [1]   = Indoor Unit Address
  // [2:3] = Outlet water heat exchanger temperature (int16 LE * 0.1 degC)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0x65_text_sensor_ != nullptr)
    this->raw_0x65_text_sensor_->publish_state(format_hex_masked_(data, data_len, {0, 1, 2, 3}));
#endif
#ifdef USE_BINARY_SENSOR
  if (this->r65_data_enabled_binary_sensor_ != nullptr)
    this->r65_data_enabled_binary_sensor_->publish_state((data[0] & 0x80) != 0);
#endif
#ifdef USE_SENSOR
  if (this->r65_indoor_unit_address_sensor_ != nullptr)
    this->r65_indoor_unit_address_sensor_->publish_state(data[1]);
  if (this->outlet_water_hx_temperature_sensor_ != nullptr)
    this->outlet_water_hx_temperature_sensor_->publish_state(decode_int16_div10_(data + 2));
#endif
}

void DaikinAltherma::parse_0xa0_outdoor_refrigerant_(const uint8_t *data, uint8_t data_len) {
  // [0:1]   = Suction temperature (fixed-point: integer=data[1], frac=(data[0]&0x7F)/256; {0x00,0x80}=N/A)
  // [2:3]   = Outdoor heat exchanger temperature (fixed-point: integer=data[1], frac=data[0]/256; {0x00,0x80}=N/A)
  // [4:5]   = Liquid pipe temperature (fixed-point: integer=data[1], frac=data[0]/256; {0x00,0x80}=N/A)
  // [6:7]   = Pressure (fixed-point: integer=data[1], frac=(data[0]&0x7F)/256; {0x00,0x80}=N/A)
  // [8:9]   = Expansion valve 3 (int16 LE)
  // [14:15] = Compressor port temperature (int16 LE * 0.1 degC)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0xa0_text_sensor_ != nullptr)
    this->raw_0xa0_text_sensor_->publish_state(
        format_hex_masked_(data, data_len, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 14, 15}));
#endif
#ifdef USE_SENSOR
  if (this->suction_temperature_sensor_ != nullptr)
    this->suction_temperature_sensor_->publish_state(decode_fixed_point_le_(data + 0));
  if (this->ra0_outdoor_heat_exchanger_temperature_sensor_ != nullptr)
    this->ra0_outdoor_heat_exchanger_temperature_sensor_->publish_state(decode_fixed_point_le_(data + 2));
  if (this->ra0_liquid_pipe_temperature_sensor_ != nullptr)
    this->ra0_liquid_pipe_temperature_sensor_->publish_state(decode_fixed_point_le_(data + 4));
  if (this->ra0_pressure_sensor_ != nullptr)
    this->ra0_pressure_sensor_->publish_state(decode_fixed_point_le_(data + 6));
  if (this->expansion_valve_3_sensor_ != nullptr)
    this->expansion_valve_3_sensor_->publish_state(static_cast<int16_t>(data[8] | (data[9] << 8)));
  if (this->compressor_port_temperature_sensor_ != nullptr)
    this->compressor_port_temperature_sensor_->publish_state(decode_int16_div10_(data + 14));
#endif
}

void DaikinAltherma::parse_0xa1_unknown_(const uint8_t *data, uint8_t data_len) {
  // [4]   = Unknown byte
  // [5:6] = Unknown value (int16 LE * 0.1)
#ifdef USE_TEXT_SENSOR
  if (this->raw_0xa1_text_sensor_ != nullptr)
    this->raw_0xa1_text_sensor_->publish_state(format_hex_masked_(data, data_len, {4, 5, 6}));
#endif
#ifdef USE_SENSOR
  if (this->ra1_04_unknown_sensor_ != nullptr)
    this->ra1_04_unknown_sensor_->publish_state(data[4]);
  if (this->ra1_0506_unknown_sensor_ != nullptr)
    this->ra1_0506_unknown_sensor_->publish_state(decode_int16_div10_(data + 5));
#endif
}

}  // namespace esphome::daikin_altherma
