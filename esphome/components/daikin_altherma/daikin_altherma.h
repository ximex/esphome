#pragma once

#include <array>
#include <initializer_list>
#include <string>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome::daikin_altherma {

// Daikin Altherma X10A Protocol I
// Serial: 9600 baud, 8E1
// Request frame:  {0x03, 0x40, register_id, CRC}
// Response frame: {0x40, register_id, length, data[length-2], CRC}
// CRC = ~(sum of all preceding bytes) & 0xFF

enum class Register : uint8_t {
  R0X00_UNKNOWN = 0x00,                   // Unknown
  R0X10_OUTDOOR_CONTROL = 0x10,           // Operation mode, status flags, errors, target temperatures
  R0X11_EEPROM = 0x11,                    // EEPROM
  R0X20_OUTDOOR_TEMPERATURE = 0x20,       // Outdoor unit temperatures
  R0X21_UNKNOWN = 0x21,                   // Unknown
  R0X30_OUTDOOR_UNIT = 0x30,              // Compressor, fan, expansion valve
  R0X60_INDOOR_CONTROL = 0x60,            // Mode, setpoints, actuators
  R0X61_INDOOR_WATER_TEMPERATURE = 0x61,  // Leaving/return/DHW temperatures
  R0X62_CONTROL_FLOW = 0x62,              // Control flags, flow rate, pump
  R0X63_UNKNOWN = 0x63,                   // Unknown
  R0X64_UNKNOWN = 0x64,                   // Unknown
  R0X65_INDOOR_OUTLET = 0x65,             // Indoor unit address, outlet water HX temperature
  R0XA0_OUTDOOR_REFRIGERANT = 0xA0,       // Suction, HX, liquid pipe, pressure, expansion valve, compressor port
  R0XA1_UNKNOWN = 0xA1,                   // Unknown
};

class DaikinAltherma : public PollingComponent, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

#ifdef USE_SENSOR
  // Register 0x10
  void set_target_evaporator_temperature_sensor(sensor::Sensor *s) {
    this->target_evaporator_temperature_sensor_ = s;
  }  // 0x10[6:7]
  void set_target_cond_temperature_sensor(sensor::Sensor *s) {
    this->target_cond_temperature_sensor_ = s;
  }  // 0x10[8:9]
  // Register 0x20
  void set_outdoor_air_temperature_sensor(sensor::Sensor *s) {
    this->outdoor_air_temperature_sensor_ = s;
  }  // 0x20[0:1]
  void set_0x20_outdoor_heat_exchanger_temperature_sensor(sensor::Sensor *s) {
    this->r20_outdoor_heat_exchanger_temperature_sensor_ = s;
  }  // 0x20[2:3]
  void set_discharge_pipe_temperature_sensor(sensor::Sensor *s) {
    this->discharge_pipe_temperature_sensor_ = s;
  }  // 0x20[4:5]
  void set_suction_pipe_temperature_sensor(sensor::Sensor *s) {
    this->suction_pipe_temperature_sensor_ = s;
  }  // 0x20[6:7]
  void set_outdoor_heat_exchanger_mid_temperature_sensor(sensor::Sensor *s) {
    this->outdoor_heat_exchanger_mid_temperature_sensor_ = s;
  }  // 0x20[8:9]
  void set_0x20_liquid_pipe_temperature_sensor(sensor::Sensor *s) {
    this->r20_liquid_pipe_temperature_sensor_ = s;
  }                                                                                             // 0x20[10:11]
  void set_0x20_1213_unknown_sensor(sensor::Sensor *s) { this->r20_1213_unknown_sensor_ = s; }  // 0x20[12:13]
  void set_0x20_low_pressure_sensor(sensor::Sensor *s) { this->r20_low_pressure_sensor_ = s; }  // 0x20[14:15]
  void set_0x20_16_unknown_sensor(sensor::Sensor *s) { this->r20_16_unknown_sensor_ = s; }      // 0x20[16]
  // Register 0x21
  void set_inv_primary_current_sensor(sensor::Sensor *s) { this->inv_primary_current_sensor_ = s; }      // 0x21[0:1]
  void set_inv_secondary_current_sensor(sensor::Sensor *s) { this->inv_secondary_current_sensor_ = s; }  // 0x21[2:3]
  void set_inv_fin_temperature_sensor(sensor::Sensor *s) { this->inv_fin_temperature_sensor_ = s; }      // 0x21[4:5]
  void set_0x21_1213_unknown_sensor(sensor::Sensor *s) { this->r21_1213_unknown_sensor_ = s; }           // 0x21[12:13]
  // Register 0x30
  void set_compressor_frequency_sensor(sensor::Sensor *s) { this->compressor_frequency_sensor_ = s; }      // 0x30[0]
  void set_fan_speed_sensor(sensor::Sensor *s) { this->fan_speed_sensor_ = s; }                            // 0x30[1]
  void set_expansion_valve_pulses_sensor(sensor::Sensor *s) { this->expansion_valve_pulses_sensor_ = s; }  // 0x30[3:4]
  void set_0x30_05_unknown_sensor(sensor::Sensor *s) { this->r30_05_unknown_sensor_ = s; }                 // 0x30[5]
  // Register 0x60
  void set_0x60_indoor_unit_address_sensor(sensor::Sensor *s) { this->r60_indoor_unit_address_sensor_ = s; }  // 0x60[1]
  void set_indoor_error_code_sensor(sensor::Sensor *s) { this->indoor_error_code_sensor_ = s; }               // 0x60[3]
  void set_indoor_unit_code_sensor(sensor::Sensor *s) { this->indoor_unit_code_sensor_ = s; }                 // 0x60[4]
  void set_indoor_error_type_sensor(sensor::Sensor *s) { this->indoor_error_type_sensor_ = s; }               // 0x60[5]
  void set_indoor_unit_capacity_sensor(sensor::Sensor *s) { this->indoor_unit_capacity_sensor_ = s; }         // 0x60[6]
  void set_dhw_tank_setpoint_sensor(sensor::Sensor *s) { this->dhw_tank_setpoint_sensor_ = s; }  // 0x60[7:8]
  void set_0x60_leaving_water_setpoint_sensor(sensor::Sensor *s) {
    this->r60_leaving_water_setpoint_sensor_ = s;
  }                                                                                                // 0x60[9:10]
  void set_indoor_option_code_sensor(sensor::Sensor *s) { this->indoor_option_code_sensor_ = s; }  // 0x60[13]
  void set_indoor_software_version_sensor(sensor::Sensor *s) { this->indoor_software_version_sensor_ = s; }  // 0x60[14]
  void set_indoor_eeprom_version_sensor(sensor::Sensor *s) { this->indoor_eeprom_version_sensor_ = s; }      // 0x60[15]
  void set_iu_eeprom_version_sensor(sensor::Sensor *s) { this->iu_eeprom_version_sensor_ = s; }              // 0x60[16]
  // Register 0x61
  void set_0x61_indoor_unit_address_sensor(sensor::Sensor *s) { this->r61_indoor_unit_address_sensor_ = s; }  // 0x61[1]
  void set_leaving_water_temperature_before_buh_sensor(sensor::Sensor *s) {
    this->leaving_water_temperature_before_buh_sensor_ = s;
  }  // 0x61[2:3]
  void set_leaving_water_temperature_after_buh_sensor(sensor::Sensor *s) {
    this->leaving_water_temperature_after_buh_sensor_ = s;
  }  // 0x61[4:5]
  void set_refrigerant_liquid_temperature_sensor(sensor::Sensor *s) {
    this->refrigerant_liquid_temperature_sensor_ = s;
  }  // 0x61[6:7]
  void set_inlet_water_temperature_sensor(sensor::Sensor *s) {
    this->inlet_water_temperature_sensor_ = s;
  }                                                                                                    // 0x61[8:9]
  void set_dhw_tank_temperature_sensor(sensor::Sensor *s) { this->dhw_tank_temperature_sensor_ = s; }  // 0x61[10:11]
  void set_room_temperature_sensor(sensor::Sensor *s) { this->room_temperature_sensor_ = s; }          // 0x61[12:13]
  void set_external_ambient_temperature_sensor(sensor::Sensor *s) {
    this->external_ambient_temperature_sensor_ = s;
  }  // 0x61[14:15]
  // Register 0x62
  void set_0x62_indoor_unit_address_sensor(sensor::Sensor *s) { this->r62_indoor_unit_address_sensor_ = s; }  // 0x62[1]
  void set_0x62_leaving_water_setpoint_sensor(sensor::Sensor *s) {
    this->r62_leaving_water_setpoint_sensor_ = s;
  }  // 0x62[3:4]
  void set_room_temperature_setpoint_sensor(sensor::Sensor *s) {
    this->room_temperature_setpoint_sensor_ = s;
  }                                                                                        // 0x62[5:6]
  void set_flow_rate_sensor(sensor::Sensor *s) { this->flow_rate_sensor_ = s; }            // 0x62[9:10]
  void set_water_pressure_sensor(sensor::Sensor *s) { this->water_pressure_sensor_ = s; }  // 0x62[11]
  void set_pump_speed_sensor(sensor::Sensor *s) { this->pump_speed_sensor_ = s; }          // 0x62[12]
  // Register 0x63
  void set_0x63_indoor_unit_address_sensor(sensor::Sensor *s) { this->r63_indoor_unit_address_sensor_ = s; }  // 0x63[1]
  // Register 0x64
  void set_0x64_indoor_unit_address_sensor(sensor::Sensor *s) { this->r64_indoor_unit_address_sensor_ = s; }  // 0x64[1]
  void set_0x64_0708_unknown_sensor(sensor::Sensor *s) { this->r64_0708_unknown_sensor_ = s; }        // 0x64[7:8]
  void set_0x64_09_unknown_pump_sensor(sensor::Sensor *s) { this->r64_09_unknown_pump_sensor_ = s; }  // 0x64[9]
  void set_0x64_12_unknown_sensor(sensor::Sensor *s) { this->r64_12_unknown_sensor_ = s; }            // 0x64[12]
  // Register 0x65
  void set_0x65_indoor_unit_address_sensor(sensor::Sensor *s) { this->r65_indoor_unit_address_sensor_ = s; }  // 0x65[1]
  void set_outlet_water_hx_temperature_sensor(sensor::Sensor *s) {
    this->outlet_water_hx_temperature_sensor_ = s;
  }  // 0x65[2:3]
  // Register 0xA0
  void set_suction_temperature_sensor(sensor::Sensor *s) { this->suction_temperature_sensor_ = s; }  // 0xA0[0:1]
  void set_0xa0_outdoor_heat_exchanger_temperature_sensor(sensor::Sensor *s) {
    this->ra0_outdoor_heat_exchanger_temperature_sensor_ = s;
  }  // 0xA0[2:3]
  void set_0xa0_liquid_pipe_temperature_sensor(sensor::Sensor *s) {
    this->ra0_liquid_pipe_temperature_sensor_ = s;
  }                                                                                              // 0xA0[4:5]
  void set_0xa0_pressure_sensor(sensor::Sensor *s) { this->ra0_pressure_sensor_ = s; }           // 0xA0[6:7]
  void set_expansion_valve_3_sensor(sensor::Sensor *s) { this->expansion_valve_3_sensor_ = s; }  // 0xA0[8:9]
  void set_compressor_port_temperature_sensor(sensor::Sensor *s) {
    this->compressor_port_temperature_sensor_ = s;
  }  // 0xA0[14:15]
  // Register 0xA1
  void set_0xa1_04_unknown_sensor(sensor::Sensor *s) { this->ra1_04_unknown_sensor_ = s; }      // 0xA1[4]
  void set_0xa1_0506_unknown_sensor(sensor::Sensor *s) { this->ra1_0506_unknown_sensor_ = s; }  // 0xA1[5:6]
#endif

#ifdef USE_BINARY_SENSOR
  // Register 0x10 - Operation status flags
  void set_thermostat_on_binary_sensor(binary_sensor::BinarySensor *s) {
    this->thermostat_on_binary_sensor_ = s;
  }  // 0x10[1] bit7
  void set_restart_standby_binary_sensor(binary_sensor::BinarySensor *s) {
    this->restart_standby_binary_sensor_ = s;
  }  // 0x10[1] bit6
  void set_startup_control_binary_sensor(binary_sensor::BinarySensor *s) {
    this->startup_control_binary_sensor_ = s;
  }  // 0x10[1] bit5
  void set_defrost_operation_binary_sensor(binary_sensor::BinarySensor *s) {
    this->defrost_operation_binary_sensor_ = s;
  }  // 0x10[1] bit4
  void set_oil_return_operation_binary_sensor(binary_sensor::BinarySensor *s) {
    this->oil_return_operation_binary_sensor_ = s;
  }  // 0x10[1] bit3
  void set_pressure_equalizing_binary_sensor(binary_sensor::BinarySensor *s) {
    this->pressure_equalizing_binary_sensor_ = s;
  }  // 0x10[1] bit2
  void set_demand_signal_binary_sensor(binary_sensor::BinarySensor *s) {
    this->demand_signal_binary_sensor_ = s;
  }  // 0x10[1] bit1
  void set_low_noise_control_binary_sensor(binary_sensor::BinarySensor *s) {
    this->low_noise_control_binary_sensor_ = s;
  }  // 0x10[1] bit0
  void set_hp_drop_control_binary_sensor(binary_sensor::BinarySensor *s) {
    this->hp_drop_control_binary_sensor_ = s;
  }  // 0x10[11] bit7
  // Register 0x30
  void set_four_way_valve_binary_sensor(binary_sensor::BinarySensor *s) {
    this->four_way_valve_binary_sensor_ = s;
  }  // 0x30[11] bit7
  void set_hot_gas_bypass_valve_binary_sensor(binary_sensor::BinarySensor *s) {
    this->hot_gas_bypass_valve_binary_sensor_ = s;
  }                                                                                             // 0x30[13] bit7
  void set_y2s_binary_sensor(binary_sensor::BinarySensor *s) { this->y2s_binary_sensor_ = s; }  // 0x30[13] bit6
  void set_y3s_binary_sensor(binary_sensor::BinarySensor *s) { this->y3s_binary_sensor_ = s; }  // 0x30[13] bit5
  // Register 0x60
  void set_0x60_data_enabled_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r60_data_enabled_binary_sensor_ = s;
  }  // 0x60[0] bit7
  void set_indoor_thermostat_on_binary_sensor(binary_sensor::BinarySensor *s) {
    this->indoor_thermostat_on_binary_sensor_ = s;
  }  // 0x60[2] bit3
  void set_freeze_protection_binary_sensor(binary_sensor::BinarySensor *s) {
    this->freeze_protection_binary_sensor_ = s;
  }  // 0x60[2] bit2
  void set_silent_mode_binary_sensor(binary_sensor::BinarySensor *s) {
    this->silent_mode_binary_sensor_ = s;
  }  // 0x60[2] bit1
  void set_freeze_protection_water_piping_binary_sensor(binary_sensor::BinarySensor *s) {
    this->freeze_protection_water_piping_binary_sensor_ = s;
  }  // 0x60[2] bit0
  void set_thermal_protector_q1l_buh_binary_sensor(binary_sensor::BinarySensor *s) {
    this->thermal_protector_q1l_buh_binary_sensor_ = s;
  }  // 0x60[11] bit6
  void set_thermal_protector_bsh_binary_sensor(binary_sensor::BinarySensor *s) {
    this->thermal_protector_bsh_binary_sensor_ = s;
  }  // 0x60[11] bit5
  void set_external_heat_source_binary_sensor(binary_sensor::BinarySensor *s) {
    this->external_heat_source_binary_sensor_ = s;
  }  // 0x60[11] bit4
  void set_two_three_way_valve_binary_sensor(binary_sensor::BinarySensor *s) {
    this->two_three_way_valve_binary_sensor_ = s;
  }  // 0x60[12] bit7
  void set_three_four_way_valve_binary_sensor(binary_sensor::BinarySensor *s) {
    this->three_four_way_valve_binary_sensor_ = s;
  }  // 0x60[12] bit6
  void set_booster_heater_binary_sensor(binary_sensor::BinarySensor *s) {
    this->booster_heater_binary_sensor_ = s;
  }  // 0x60[12] bit5
  void set_backup_heater_step1_binary_sensor(binary_sensor::BinarySensor *s) {
    this->backup_heater_step1_binary_sensor_ = s;
  }  // 0x60[12] bit4
  void set_backup_heater_step2_binary_sensor(binary_sensor::BinarySensor *s) {
    this->backup_heater_step2_binary_sensor_ = s;
  }  // 0x60[12] bit3
  void set_bottom_plate_heater_binary_sensor(binary_sensor::BinarySensor *s) {
    this->bottom_plate_heater_binary_sensor_ = s;
  }  // 0x60[12] bit2
  void set_water_pump_binary_sensor(binary_sensor::BinarySensor *s) {
    this->water_pump_binary_sensor_ = s;
  }  // 0x60[12] bit1
  void set_solar_pump_binary_sensor(binary_sensor::BinarySensor *s) {
    this->solar_pump_binary_sensor_ = s;
  }  // 0x60[12] bit0
  // Register 0x61
  void set_0x61_data_enabled_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r61_data_enabled_binary_sensor_ = s;
  }  // 0x61[0] bit7
  // Register 0x62
  void set_0x62_data_enabled_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r62_data_enabled_binary_sensor_ = s;
  }  // 0x62[0] bit7
  void set_powerful_dhw_binary_sensor(binary_sensor::BinarySensor *s) {
    this->powerful_dhw_binary_sensor_ = s;
  }  // 0x62[2] bit4
  void set_space_heating_binary_sensor(binary_sensor::BinarySensor *s) {
    this->space_heating_binary_sensor_ = s;
  }  // 0x62[2] bit3
  void set_0x62_08_bit7_unknown_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r62_08_bit7_unknown_binary_sensor_ = s;
  }  // 0x62[8] bit7
  void set_0x62_08_bit6_unknown_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r62_08_bit6_unknown_binary_sensor_ = s;
  }  // 0x62[8] bit6
  void set_0x62_08_bit5_unknown_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r62_08_bit5_unknown_binary_sensor_ = s;
  }  // 0x62[8] bit5
  void set_space_h_operation_output_binary_sensor(binary_sensor::BinarySensor *s) {
    this->space_h_operation_output_binary_sensor_ = s;
  }  // 0x62[8] bit0
  // Register 0x63
  void set_0x63_data_enabled_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r63_data_enabled_binary_sensor_ = s;
  }  // 0x63[0] bit7
  // Register 0x64
  void set_0x64_data_enabled_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r64_data_enabled_binary_sensor_ = s;
  }  // 0x64[0] bit7
  // Register 0x65
  void set_0x65_data_enabled_binary_sensor(binary_sensor::BinarySensor *s) {
    this->r65_data_enabled_binary_sensor_ = s;
  }  // 0x65[0] bit7
#endif

#ifdef USE_TEXT_SENSOR
  // Register 0x10 - Outdoor unit operation
  void set_operation_mode_text_sensor(text_sensor::TextSensor *s) { this->operation_mode_text_sensor_ = s; }  // 0x10[0]
  void set_error_type_text_sensor(text_sensor::TextSensor *s) { this->error_type_text_sensor_ = s; }          // 0x10[4]
  void set_error_code_text_sensor(text_sensor::TextSensor *s) { this->error_code_text_sensor_ = s; }          // 0x10[5]
  void set_0x10_11_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r10_11_flags_text_sensor_ = s;
  }  // 0x10[11] flags
  // Register 0x30
  void set_0x30_11_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r30_11_flags_text_sensor_ = s;
  }  // 0x30[11] flags
  void set_0x30_13_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r30_13_flags_text_sensor_ = s;
  }  // 0x30[13] flags
  // Register 0x60
  void set_indoor_operation_mode_text_sensor(text_sensor::TextSensor *s) {
    this->indoor_operation_mode_text_sensor_ = s;
  }  // 0x60[2]
  void set_0x60_11_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r60_11_flags_text_sensor_ = s;
  }  // 0x60[11] flags
  // Register 0x62
  void set_0x62_02_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r62_02_flags_text_sensor_ = s;
  }  // 0x62[2] flags
  void set_0x62_07_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r62_07_flags_text_sensor_ = s;
  }  // 0x62[7] flags
  void set_0x62_08_flags_text_sensor(text_sensor::TextSensor *s) {
    this->r62_08_flags_text_sensor_ = s;
  }  // 0x62[8] flags
  // Raw register dumps
  void set_raw_0x00_text_sensor(text_sensor::TextSensor *s) { this->raw_0x00_text_sensor_ = s; }  // 0x00 raw
  void set_raw_0x10_text_sensor(text_sensor::TextSensor *s) { this->raw_0x10_text_sensor_ = s; }  // 0x10 raw
  void set_raw_0x11_text_sensor(text_sensor::TextSensor *s) { this->raw_0x11_text_sensor_ = s; }  // 0x11 raw
  void set_raw_0x20_text_sensor(text_sensor::TextSensor *s) { this->raw_0x20_text_sensor_ = s; }  // 0x20 raw
  void set_raw_0x21_text_sensor(text_sensor::TextSensor *s) { this->raw_0x21_text_sensor_ = s; }  // 0x21 raw
  void set_raw_0x30_text_sensor(text_sensor::TextSensor *s) { this->raw_0x30_text_sensor_ = s; }  // 0x30 raw
  void set_raw_0x60_text_sensor(text_sensor::TextSensor *s) { this->raw_0x60_text_sensor_ = s; }  // 0x60 raw
  void set_raw_0x61_text_sensor(text_sensor::TextSensor *s) { this->raw_0x61_text_sensor_ = s; }  // 0x61 raw
  void set_raw_0x62_text_sensor(text_sensor::TextSensor *s) { this->raw_0x62_text_sensor_ = s; }  // 0x62 raw
  void set_raw_0x63_text_sensor(text_sensor::TextSensor *s) { this->raw_0x63_text_sensor_ = s; }  // 0x63 raw
  void set_raw_0x64_text_sensor(text_sensor::TextSensor *s) { this->raw_0x64_text_sensor_ = s; }  // 0x64 raw
  void set_raw_0x65_text_sensor(text_sensor::TextSensor *s) { this->raw_0x65_text_sensor_ = s; }  // 0x65 raw
  void set_raw_0xa0_text_sensor(text_sensor::TextSensor *s) { this->raw_0xa0_text_sensor_ = s; }  // 0xA0 raw
  void set_raw_0xa1_text_sensor(text_sensor::TextSensor *s) { this->raw_0xa1_text_sensor_ = s; }  // 0xA1 raw
#endif

 protected:
  void send_request_(Register reg);
  void advance_register_();
  void parse_register_(Register reg, const uint8_t *data, uint8_t data_len);
  void parse_0x00_unknown_(const uint8_t *data, uint8_t data_len);
  void parse_0x10_outdoor_control_(const uint8_t *data, uint8_t data_len);
  void parse_0x11_eeprom_(const uint8_t *data, uint8_t data_len);
  void parse_0x20_outdoor_temperature_(const uint8_t *data, uint8_t data_len);
  void parse_0x21_unknown_(const uint8_t *data, uint8_t data_len);
  void parse_0x30_outdoor_unit_(const uint8_t *data, uint8_t data_len);
  void parse_0x60_indoor_control_(const uint8_t *data, uint8_t data_len);
  void parse_0x61_indoor_water_temperature_(const uint8_t *data, uint8_t data_len);
  void parse_0x62_control_flow_(const uint8_t *data, uint8_t data_len);
  void parse_0x63_unknown_(const uint8_t *data, uint8_t data_len);
  void parse_0x64_unknown_(const uint8_t *data, uint8_t data_len);
  void parse_0x65_indoor_outlet_(const uint8_t *data, uint8_t data_len);
  void parse_0xa0_outdoor_refrigerant_(const uint8_t *data, uint8_t data_len);
  void parse_0xa1_unknown_(const uint8_t *data, uint8_t data_len);

  static uint8_t checksum_(const uint8_t *data, uint8_t len);
  static std::string format_hex_masked_(const uint8_t *data, uint8_t len, std::initializer_list<uint8_t> mask_indices);
  static std::string format_bits_masked_(uint8_t value, std::initializer_list<uint8_t> known_bits);
  static float decode_int16_div10_(const uint8_t *data);
  static float decode_uint16_div10_(const uint8_t *data);
  static float decode_fixed_point_le_(const uint8_t *data);
  static float decode_fixed_point_signed_le_x10_(const uint8_t *data);
  static const char *decode_0x10_operation_mode_(uint8_t mode_byte);
  static const char *decode_0x10_error_type_(uint8_t error_byte);
  static std::string decode_0x10_error_code_(uint8_t code_byte);
  static const char *decode_0x60_operation_mode_(uint8_t mode_byte);

  // Polling state machine
  enum class State : uint8_t {
    IDLE,
    WAITING_RESPONSE,
  };

  State state_{State::IDLE};
  uint8_t register_index_{0};
  uint32_t last_send_ms_{0};

  // Receive buffer (max response frame ~21 bytes)
  std::array<uint8_t, 32> rx_buf_{};
  uint8_t rx_pos_{0};

  static constexpr std::array<Register, 14> REGISTERS_ = {
      Register::R0X00_UNKNOWN,
      Register::R0X10_OUTDOOR_CONTROL,
      Register::R0X11_EEPROM,
      Register::R0X20_OUTDOOR_TEMPERATURE,
      Register::R0X21_UNKNOWN,
      Register::R0X30_OUTDOOR_UNIT,
      Register::R0X60_INDOOR_CONTROL,
      Register::R0X61_INDOOR_WATER_TEMPERATURE,
      Register::R0X62_CONTROL_FLOW,
      Register::R0X63_UNKNOWN,
      Register::R0X64_UNKNOWN,
      Register::R0X65_INDOOR_OUTLET,
      Register::R0XA0_OUTDOOR_REFRIGERANT,
      Register::R0XA1_UNKNOWN,
  };
  static constexpr uint16_t RESPONSE_TIMEOUT_MS = 500;

#ifdef USE_SENSOR
  // Register 0x10
  sensor::Sensor *target_evaporator_temperature_sensor_{nullptr};  // 0x10[6:7]
  sensor::Sensor *target_cond_temperature_sensor_{nullptr};        // 0x10[8:9]
  // Register 0x20
  sensor::Sensor *outdoor_air_temperature_sensor_{nullptr};                 // 0x20[0:1]
  sensor::Sensor *r20_outdoor_heat_exchanger_temperature_sensor_{nullptr};  // 0x20[2:3]
  sensor::Sensor *discharge_pipe_temperature_sensor_{nullptr};              // 0x20[4:5]
  sensor::Sensor *suction_pipe_temperature_sensor_{nullptr};                // 0x20[6:7]
  sensor::Sensor *outdoor_heat_exchanger_mid_temperature_sensor_{nullptr};  // 0x20[8:9]
  sensor::Sensor *r20_liquid_pipe_temperature_sensor_{nullptr};             // 0x20[10:11]
  sensor::Sensor *r20_1213_unknown_sensor_{nullptr};                        // 0x20[12:13]
  sensor::Sensor *r20_low_pressure_sensor_{nullptr};                        // 0x20[14:15]
  sensor::Sensor *r20_16_unknown_sensor_{nullptr};                          // 0x20[16]
  // Register 0x21
  sensor::Sensor *inv_primary_current_sensor_{nullptr};    // 0x21[0:1]
  sensor::Sensor *inv_secondary_current_sensor_{nullptr};  // 0x21[2:3]
  sensor::Sensor *inv_fin_temperature_sensor_{nullptr};    // 0x21[4:5]
  sensor::Sensor *r21_1213_unknown_sensor_{nullptr};       // 0x21[12:13]
  // Register 0x30
  sensor::Sensor *compressor_frequency_sensor_{nullptr};    // 0x30[0]
  sensor::Sensor *fan_speed_sensor_{nullptr};               // 0x30[1]
  sensor::Sensor *expansion_valve_pulses_sensor_{nullptr};  // 0x30[3:4]
  sensor::Sensor *r30_05_unknown_sensor_{nullptr};          // 0x30[5]
  // Register 0x60
  sensor::Sensor *r60_indoor_unit_address_sensor_{nullptr};     // 0x60[1]
  sensor::Sensor *indoor_error_code_sensor_{nullptr};           // 0x60[3]
  sensor::Sensor *indoor_unit_code_sensor_{nullptr};            // 0x60[4]
  sensor::Sensor *indoor_error_type_sensor_{nullptr};           // 0x60[5]
  sensor::Sensor *indoor_unit_capacity_sensor_{nullptr};        // 0x60[6]
  sensor::Sensor *dhw_tank_setpoint_sensor_{nullptr};           // 0x60[7:8]
  sensor::Sensor *r60_leaving_water_setpoint_sensor_{nullptr};  // 0x60[9:10]
  sensor::Sensor *indoor_option_code_sensor_{nullptr};          // 0x60[13]
  sensor::Sensor *indoor_software_version_sensor_{nullptr};     // 0x60[14]
  sensor::Sensor *indoor_eeprom_version_sensor_{nullptr};       // 0x60[15]
  sensor::Sensor *iu_eeprom_version_sensor_{nullptr};           // 0x60[16]
  // Register 0x61
  sensor::Sensor *r61_indoor_unit_address_sensor_{nullptr};               // 0x61[1]
  sensor::Sensor *leaving_water_temperature_before_buh_sensor_{nullptr};  // 0x61[2:3]
  sensor::Sensor *leaving_water_temperature_after_buh_sensor_{nullptr};   // 0x61[4:5]
  sensor::Sensor *refrigerant_liquid_temperature_sensor_{nullptr};        // 0x61[6:7]
  sensor::Sensor *inlet_water_temperature_sensor_{nullptr};               // 0x61[8:9]
  sensor::Sensor *dhw_tank_temperature_sensor_{nullptr};                  // 0x61[10:11]
  sensor::Sensor *room_temperature_sensor_{nullptr};                      // 0x61[12:13]
  sensor::Sensor *external_ambient_temperature_sensor_{nullptr};          // 0x61[14:15]
  // Register 0x62
  sensor::Sensor *r62_indoor_unit_address_sensor_{nullptr};     // 0x62[1]
  sensor::Sensor *r62_leaving_water_setpoint_sensor_{nullptr};  // 0x62[3:4]
  sensor::Sensor *room_temperature_setpoint_sensor_{nullptr};   // 0x62[5:6]
  sensor::Sensor *flow_rate_sensor_{nullptr};                   // 0x62[9:10]
  sensor::Sensor *water_pressure_sensor_{nullptr};              // 0x62[11]
  sensor::Sensor *pump_speed_sensor_{nullptr};                  // 0x62[12]
  // Register 0x63
  sensor::Sensor *r63_indoor_unit_address_sensor_{nullptr};  // 0x63[1]
  // Register 0x64
  sensor::Sensor *r64_indoor_unit_address_sensor_{nullptr};  // 0x64[1]
  sensor::Sensor *r64_0708_unknown_sensor_{nullptr};         // 0x64[7:8]
  sensor::Sensor *r64_09_unknown_pump_sensor_{nullptr};      // 0x64[9]
  sensor::Sensor *r64_12_unknown_sensor_{nullptr};           // 0x64[12]
  // Register 0x65
  sensor::Sensor *r65_indoor_unit_address_sensor_{nullptr};      // 0x65[1]
  sensor::Sensor *outlet_water_hx_temperature_sensor_{nullptr};  // 0x65[2:3]
  // Register 0xA0
  sensor::Sensor *suction_temperature_sensor_{nullptr};                     // 0xA0[0:1]
  sensor::Sensor *ra0_outdoor_heat_exchanger_temperature_sensor_{nullptr};  // 0xA0[2:3]
  sensor::Sensor *ra0_liquid_pipe_temperature_sensor_{nullptr};             // 0xA0[4:5]
  sensor::Sensor *ra0_pressure_sensor_{nullptr};                            // 0xA0[6:7]
  sensor::Sensor *expansion_valve_3_sensor_{nullptr};                       // 0xA0[8:9]
  sensor::Sensor *compressor_port_temperature_sensor_{nullptr};             // 0xA0[14:15]
  // Register 0xA1
  sensor::Sensor *ra1_04_unknown_sensor_{nullptr};    // 0xA1[4]
  sensor::Sensor *ra1_0506_unknown_sensor_{nullptr};  // 0xA1[5:6]
#endif

#ifdef USE_BINARY_SENSOR
  // Register 0x10 - Operation status flags
  binary_sensor::BinarySensor *thermostat_on_binary_sensor_{nullptr};         // 0x10[1] bit7
  binary_sensor::BinarySensor *restart_standby_binary_sensor_{nullptr};       // 0x10[1] bit6
  binary_sensor::BinarySensor *startup_control_binary_sensor_{nullptr};       // 0x10[1] bit5
  binary_sensor::BinarySensor *defrost_operation_binary_sensor_{nullptr};     // 0x10[1] bit4
  binary_sensor::BinarySensor *oil_return_operation_binary_sensor_{nullptr};  // 0x10[1] bit3
  binary_sensor::BinarySensor *pressure_equalizing_binary_sensor_{nullptr};   // 0x10[1] bit2
  binary_sensor::BinarySensor *demand_signal_binary_sensor_{nullptr};         // 0x10[1] bit1
  binary_sensor::BinarySensor *low_noise_control_binary_sensor_{nullptr};     // 0x10[1] bit0
  binary_sensor::BinarySensor *hp_drop_control_binary_sensor_{nullptr};       // 0x10[11] bit7
  // Register 0x30
  binary_sensor::BinarySensor *four_way_valve_binary_sensor_{nullptr};        // 0x30[11] bit7
  binary_sensor::BinarySensor *hot_gas_bypass_valve_binary_sensor_{nullptr};  // 0x30[13] bit7
  binary_sensor::BinarySensor *y2s_binary_sensor_{nullptr};                   // 0x30[13] bit6
  binary_sensor::BinarySensor *y3s_binary_sensor_{nullptr};                   // 0x30[13] bit5
  // Register 0x60
  binary_sensor::BinarySensor *r60_data_enabled_binary_sensor_{nullptr};                // 0x60[0] bit7
  binary_sensor::BinarySensor *indoor_thermostat_on_binary_sensor_{nullptr};            // 0x60[2] bit3
  binary_sensor::BinarySensor *freeze_protection_binary_sensor_{nullptr};               // 0x60[2] bit2
  binary_sensor::BinarySensor *silent_mode_binary_sensor_{nullptr};                     // 0x60[2] bit1
  binary_sensor::BinarySensor *freeze_protection_water_piping_binary_sensor_{nullptr};  // 0x60[2] bit0
  binary_sensor::BinarySensor *thermal_protector_q1l_buh_binary_sensor_{nullptr};       // 0x60[11] bit6
  binary_sensor::BinarySensor *thermal_protector_bsh_binary_sensor_{nullptr};           // 0x60[11] bit5
  binary_sensor::BinarySensor *external_heat_source_binary_sensor_{nullptr};            // 0x60[11] bit4
  binary_sensor::BinarySensor *two_three_way_valve_binary_sensor_{nullptr};             // 0x60[12] bit7
  binary_sensor::BinarySensor *three_four_way_valve_binary_sensor_{nullptr};            // 0x60[12] bit6
  binary_sensor::BinarySensor *booster_heater_binary_sensor_{nullptr};                  // 0x60[12] bit5
  binary_sensor::BinarySensor *backup_heater_step1_binary_sensor_{nullptr};             // 0x60[12] bit4
  binary_sensor::BinarySensor *backup_heater_step2_binary_sensor_{nullptr};             // 0x60[12] bit3
  binary_sensor::BinarySensor *bottom_plate_heater_binary_sensor_{nullptr};             // 0x60[12] bit2
  binary_sensor::BinarySensor *water_pump_binary_sensor_{nullptr};                      // 0x60[12] bit1
  binary_sensor::BinarySensor *solar_pump_binary_sensor_{nullptr};                      // 0x60[12] bit0
  // Register 0x61
  binary_sensor::BinarySensor *r61_data_enabled_binary_sensor_{nullptr};  // 0x61[0] bit7
  // Register 0x62
  binary_sensor::BinarySensor *r62_data_enabled_binary_sensor_{nullptr};          // 0x62[0] bit7
  binary_sensor::BinarySensor *powerful_dhw_binary_sensor_{nullptr};              // 0x62[2] bit4
  binary_sensor::BinarySensor *space_heating_binary_sensor_{nullptr};             // 0x62[2] bit3
  binary_sensor::BinarySensor *r62_08_bit7_unknown_binary_sensor_{nullptr};       // 0x62[8] bit7
  binary_sensor::BinarySensor *r62_08_bit6_unknown_binary_sensor_{nullptr};       // 0x62[8] bit6
  binary_sensor::BinarySensor *r62_08_bit5_unknown_binary_sensor_{nullptr};       // 0x62[8] bit5
  binary_sensor::BinarySensor *space_h_operation_output_binary_sensor_{nullptr};  // 0x62[8] bit0
  // Register 0x63
  binary_sensor::BinarySensor *r63_data_enabled_binary_sensor_{nullptr};  // 0x63[0] bit7
  // Register 0x64
  binary_sensor::BinarySensor *r64_data_enabled_binary_sensor_{nullptr};  // 0x64[0] bit7
  // Register 0x65
  binary_sensor::BinarySensor *r65_data_enabled_binary_sensor_{nullptr};  // 0x65[0] bit7
#endif

#ifdef USE_TEXT_SENSOR
  // Register 0x10 - Outdoor unit operation
  text_sensor::TextSensor *operation_mode_text_sensor_{nullptr};  // 0x10[0]
  text_sensor::TextSensor *error_type_text_sensor_{nullptr};      // 0x10[4]
  text_sensor::TextSensor *error_code_text_sensor_{nullptr};      // 0x10[5]
  text_sensor::TextSensor *r10_11_flags_text_sensor_{nullptr};    // 0x10[11] flags
  // Register 0x30
  text_sensor::TextSensor *r30_11_flags_text_sensor_{nullptr};  // 0x30[11] flags
  text_sensor::TextSensor *r30_13_flags_text_sensor_{nullptr};  // 0x30[13] flags
  // Register 0x60
  text_sensor::TextSensor *indoor_operation_mode_text_sensor_{nullptr};  // 0x60[2]
  text_sensor::TextSensor *r60_11_flags_text_sensor_{nullptr};           // 0x60[11] flags
  // Register 0x62
  text_sensor::TextSensor *r62_02_flags_text_sensor_{nullptr};  // 0x62[2] flags
  text_sensor::TextSensor *r62_07_flags_text_sensor_{nullptr};  // 0x62[7] flags
  text_sensor::TextSensor *r62_08_flags_text_sensor_{nullptr};  // 0x62[8] flags
  // Raw register dumps
  text_sensor::TextSensor *raw_0x00_text_sensor_{nullptr};  // 0x00 raw
  text_sensor::TextSensor *raw_0x10_text_sensor_{nullptr};  // 0x10 raw
  text_sensor::TextSensor *raw_0x11_text_sensor_{nullptr};  // 0x11 raw
  text_sensor::TextSensor *raw_0x20_text_sensor_{nullptr};  // 0x20 raw
  text_sensor::TextSensor *raw_0x21_text_sensor_{nullptr};  // 0x21 raw
  text_sensor::TextSensor *raw_0x30_text_sensor_{nullptr};  // 0x30 raw
  text_sensor::TextSensor *raw_0x60_text_sensor_{nullptr};  // 0x60 raw
  text_sensor::TextSensor *raw_0x61_text_sensor_{nullptr};  // 0x61 raw
  text_sensor::TextSensor *raw_0x62_text_sensor_{nullptr};  // 0x62 raw
  text_sensor::TextSensor *raw_0x63_text_sensor_{nullptr};  // 0x63 raw
  text_sensor::TextSensor *raw_0x64_text_sensor_{nullptr};  // 0x64 raw
  text_sensor::TextSensor *raw_0x65_text_sensor_{nullptr};  // 0x65 raw
  text_sensor::TextSensor *raw_0xa0_text_sensor_{nullptr};  // 0xA0 raw
  text_sensor::TextSensor *raw_0xa1_text_sensor_{nullptr};  // 0xA1 raw
#endif
};

}  // namespace esphome::daikin_altherma
