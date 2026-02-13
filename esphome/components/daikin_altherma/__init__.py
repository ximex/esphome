import esphome.codegen as cg
from esphome.components import binary_sensor, sensor, text_sensor, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_HEAT,
    DEVICE_CLASS_PRESSURE,
    DEVICE_CLASS_RUNNING,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLUME_FLOW_RATE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_FAN,
    ICON_GAUGE,
    ICON_THERMOMETER,
    ICON_WATER_HEATER,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_HERTZ,
)

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]
CODEOWNERS = ["@ximex"]

daikin_altherma_ns = cg.esphome_ns.namespace("daikin_altherma")
DaikinAltherma = daikin_altherma_ns.class_(
    "DaikinAltherma", cg.PollingComponent, uart.UARTDevice
)

# Sensor config keys
# Register 0x10
CONF_THERMOSTAT_ON = "thermostat_on"  # 0x10[1] bit7
CONF_RESTART_STANDBY = "restart_standby"  # 0x10[1] bit6
CONF_STARTUP_CONTROL = "startup_control"  # 0x10[1] bit5
CONF_DEFROST_OPERATION = "defrost_operation"  # 0x10[1] bit4
CONF_OIL_RETURN_OPERATION = "oil_return_operation"  # 0x10[1] bit3
CONF_PRESSURE_EQUALIZING = "pressure_equalizing"  # 0x10[1] bit2
CONF_DEMAND_SIGNAL = "demand_signal"  # 0x10[1] bit1
CONF_LOW_NOISE_CONTROL = "low_noise_control"  # 0x10[1] bit0
CONF_TARGET_EVAPORATOR_TEMPERATURE = "target_evaporator_temperature"  # 0x10[6:7]
CONF_TARGET_COND_TEMPERATURE = "target_cond_temperature"  # 0x10[8:9]
CONF_HP_DROP_CONTROL = "hp_drop_control"  # 0x10[11] bit7
# Register 0x20 - Outdoor unit temperatures
CONF_OUTDOOR_AIR_TEMPERATURE = "outdoor_air_temperature"  # 0x20[0:1]
CONF_0X20_OUTDOOR_HEAT_EXCHANGER_TEMPERATURE = (
    "0x20_outdoor_heat_exchanger_temperature"  # 0x20[2:3]
)
CONF_DISCHARGE_PIPE_TEMPERATURE = "discharge_pipe_temperature"  # 0x20[4:5]
CONF_SUCTION_PIPE_TEMPERATURE = "suction_pipe_temperature"  # 0x20[6:7]
CONF_OUTDOOR_HEAT_EXCHANGER_MID_TEMPERATURE = (
    "outdoor_heat_exchanger_mid_temperature"  # 0x20[8:9]
)
CONF_0X20_LIQUID_PIPE_TEMPERATURE = "0x20_liquid_pipe_temperature"  # 0x20[10:11]
CONF_0X20_1213_UNKNOWN = "0x20_1213_unknown"  # 0x20[12:13]
CONF_0X20_LOW_PRESSURE = "0x20_low_pressure"  # 0x20[14:15]
CONF_0X20_16_UNKNOWN = "0x20_16_unknown"  # 0x20[16]
# Register 0x21
CONF_INV_PRIMARY_CURRENT = "inv_primary_current"  # 0x21[0:1]
CONF_INV_SECONDARY_CURRENT = "inv_secondary_current"  # 0x21[2:3]
CONF_INV_FIN_TEMPERATURE = "inv_fin_temperature"  # 0x21[4:5]
CONF_0X21_1213_UNKNOWN = "0x21_1213_unknown"  # 0x21[12:13]
# Register 0x30 - Outdoor unit operational data
CONF_COMPRESSOR_FREQUENCY = "compressor_frequency"  # 0x30[0]
CONF_FAN_SPEED = "fan_speed"  # 0x30[1]
CONF_EXPANSION_VALVE_PULSES = "expansion_valve_pulses"  # 0x30[3:4]
CONF_0X30_05_UNKNOWN = "0x30_05_unknown"  # 0x30[5]
CONF_FOUR_WAY_VALVE = "four_way_valve"  # 0x30[11] bit7
CONF_Y3S = "y3s"  # 0x30[13] bit5
CONF_Y2S = "y2s"  # 0x30[13] bit6
CONF_HOT_GAS_BYPASS_VALVE = "hot_gas_bypass_valve"  # 0x30[13] bit7
# Register 0x60 - Indoor operation control
CONF_0X60_INDOOR_UNIT_ADDRESS = "0x60_indoor_unit_address"  # 0x60[1]
CONF_INDOOR_ERROR_CODE = "indoor_error_code"  # 0x60[3]
CONF_INDOOR_UNIT_CODE = "indoor_unit_code"  # 0x60[4]
CONF_INDOOR_ERROR_TYPE = "indoor_error_type"  # 0x60[5]
CONF_INDOOR_UNIT_CAPACITY = "indoor_unit_capacity"  # 0x60[6]
CONF_DHW_TANK_SETPOINT = "dhw_tank_setpoint"  # 0x60[7:8]
CONF_0X60_LEAVING_WATER_SETPOINT = "0x60_leaving_water_setpoint"  # 0x60[9:10]
CONF_INDOOR_OPTION_CODE = "indoor_option_code"  # 0x60[13]
CONF_INDOOR_SOFTWARE_VERSION = "indoor_software_version"  # 0x60[14]
CONF_INDOOR_EEPROM_VERSION = "indoor_eeprom_version"  # 0x60[15]
CONF_IU_EEPROM_VERSION = "iu_eeprom_version"  # 0x60[16]
# Register 0x61 - Indoor water temperatures
CONF_0X61_INDOOR_UNIT_ADDRESS = "0x61_indoor_unit_address"  # 0x61[1]
CONF_LEAVING_WATER_TEMPERATURE_BEFORE_BUH = (
    "leaving_water_temperature_before_buh"  # 0x61[2:3] R1T
)
CONF_LEAVING_WATER_TEMPERATURE_AFTER_BUH = (
    "leaving_water_temperature_after_buh"  # 0x61[4:5] R2T
)
CONF_REFRIGERANT_LIQUID_TEMPERATURE = "refrigerant_liquid_temperature"  # 0x61[6:7] R3T
CONF_INLET_WATER_TEMPERATURE = "inlet_water_temperature"  # 0x61[8:9] R4T
CONF_DHW_TANK_TEMPERATURE = "dhw_tank_temperature"  # 0x61[10:11] R5T
CONF_ROOM_TEMPERATURE = "room_temperature"  # 0x61[12:13]
CONF_EXTERNAL_AMBIENT_TEMPERATURE = "external_ambient_temperature"  # 0x61[14:15] R6T
# Register 0x62 - Control flags and flow sensors
CONF_0X62_INDOOR_UNIT_ADDRESS = "0x62_indoor_unit_address"  # 0x62[1]
CONF_0X62_LEAVING_WATER_SETPOINT = "0x62_leaving_water_setpoint"  # 0x62[3:4]
CONF_ROOM_TEMPERATURE_SETPOINT = "room_temperature_setpoint"  # 0x62[5:6]
CONF_FLOW_RATE = "flow_rate"  # 0x62[9:10]
CONF_WATER_PRESSURE = "water_pressure"  # 0x62[11]
CONF_PUMP_SPEED = "pump_speed"  # 0x62[12]
# Register 0x63
CONF_0X63_INDOOR_UNIT_ADDRESS = "0x63_indoor_unit_address"  # 0x63[1]
# Register 0x64
CONF_0X64_INDOOR_UNIT_ADDRESS = "0x64_indoor_unit_address"  # 0x64[1]
CONF_0X64_0708_UNKNOWN = "0x64_0708_unknown"  # 0x64[7:8]
CONF_0X64_09_UNKNOWN_PUMP = "0x64_09_unknown_pump"  # 0x64[9]
CONF_0X64_12_UNKNOWN = "0x64_12_unknown"  # 0x64[12]
# Register 0x65
CONF_0X65_INDOOR_UNIT_ADDRESS = "0x65_indoor_unit_address"  # 0x65[1]
CONF_OUTLET_WATER_HX_TEMPERATURE = "outlet_water_hx_temperature"  # 0x65[2:3]
# Register 0xA0
CONF_SUCTION_TEMPERATURE = "suction_temperature"  # 0xA0[0:1]
CONF_0XA0_OUTDOOR_HEAT_EXCHANGER_TEMPERATURE = (
    "0xa0_outdoor_heat_exchanger_temperature"  # 0xA0[2:3]
)
CONF_0XA0_LIQUID_PIPE_TEMPERATURE = "0xa0_liquid_pipe_temperature"  # 0xA0[4:5]
CONF_0XA0_PRESSURE = "0xa0_pressure"  # 0xA0[6:7]
CONF_EXPANSION_VALVE_3 = "expansion_valve_3"  # 0xA0[8:9]
CONF_COMPRESSOR_PORT_TEMPERATURE = "compressor_port_temperature"  # 0xA0[14:15]
# Register 0xA1
CONF_0XA1_04_UNKNOWN = "0xa1_04_unknown"  # 0xA1[4]
CONF_0XA1_0506_UNKNOWN = "0xa1_0506_unknown"  # 0xA1[5:6]

# Binary sensor config keys
# Register 0x60 - Indoor operation control
CONF_0X60_DATA_ENABLED = "0x60_data_enabled"  # 0x60[0] bit7
CONF_INDOOR_THERMOSTAT_ON = "indoor_thermostat_on"  # 0x60[2] bit3
CONF_FREEZE_PROTECTION = "freeze_protection"  # 0x60[2] bit2
CONF_SILENT_MODE = "silent_mode"  # 0x60[2] bit1
CONF_FREEZE_PROTECTION_WATER_PIPING = "freeze_protection_water_piping"  # 0x60[2] bit0
CONF_THERMAL_PROTECTOR_Q1L_BUH = "thermal_protector_q1l_buh"  # 0x60[11] bit6
CONF_THERMAL_PROTECTOR_BSH = "thermal_protector_bsh"  # 0x60[11] bit5
CONF_EXTERNAL_HEAT_SOURCE = "external_heat_source"  # 0x60[11] bit4
CONF_TWO_THREE_WAY_VALVE = "two_three_way_valve"  # 0x60[12] bit7
CONF_THREE_FOUR_WAY_VALVE = "three_four_way_valve"  # 0x60[12] bit6
CONF_BOOSTER_HEATER = "booster_heater"  # 0x60[12] bit5
CONF_BACKUP_HEATER_STEP1 = "backup_heater_step1"  # 0x60[12] bit4
CONF_BACKUP_HEATER_STEP2 = "backup_heater_step2"  # 0x60[12] bit3
CONF_BOTTOM_PLATE_HEATER = "bottom_plate_heater"  # 0x60[12] bit2
CONF_WATER_PUMP = "water_pump"  # 0x60[12] bit1
CONF_SOLAR_PUMP = "solar_pump"  # 0x60[12] bit0
# Register 0x61
CONF_0X61_DATA_ENABLED = "0x61_data_enabled"  # 0x61[0] bit7
# Register 0x62 - Control flags
CONF_0X62_DATA_ENABLED = "0x62_data_enabled"  # 0x62[0] bit7
CONF_POWERFUL_DHW = "powerful_dhw"  # 0x62[2] bit4
CONF_SPACE_HEATING = "space_heating"  # 0x62[2] bit3
CONF_0X62_08_BIT7_UNKNOWN = "0x62_08_bit7_unknown"  # 0x62[8] bit7
CONF_0X62_08_BIT6_UNKNOWN = "0x62_08_bit6_unknown"  # 0x62[8] bit6
CONF_0X62_08_BIT5_UNKNOWN = "0x62_08_bit5_unknown"  # 0x62[8] bit5
CONF_SPACE_H_OPERATION_OUTPUT = "space_h_operation_output"  # 0x62[8] bit0
# Register 0x63
CONF_0X63_DATA_ENABLED = "0x63_data_enabled"  # 0x63[0] bit7
# Register 0x64
CONF_0X64_DATA_ENABLED = "0x64_data_enabled"  # 0x64[0] bit7
# Register 0x65
CONF_0X65_DATA_ENABLED = "0x65_data_enabled"  # 0x65[0] bit7

# Text sensor config keys
# Register 0x10 - Outdoor unit operation
CONF_OPERATION_MODE = "operation_mode"  # 0x10[0]
CONF_ERROR_TYPE = "error_type"  # 0x10[4]
CONF_ERROR_CODE = "error_code"  # 0x10[5]
CONF_0X10_11_FLAGS = "0x10_11_flags"  # 0x10[11] flags
# Register 0x30 - Outdoor unit operational data
CONF_0X30_11_FLAGS = "0x30_11_flags"  # 0x30[11] flags
CONF_0X30_13_FLAGS = "0x30_13_flags"  # 0x30[13] flags
# Register 0x60 - Indoor operation control
CONF_INDOOR_OPERATION_MODE = "indoor_operation_mode"  # 0x60[2]
CONF_0X60_11_FLAGS = "0x60_11_flags"  # 0x60[11] flags
# Register 0x62 - Control flags
CONF_0X62_02_FLAGS = "0x62_02_flags"  # 0x62[2] flags
CONF_0X62_07_FLAGS = "0x62_07_flags"  # 0x62[7] flags
CONF_0X62_08_FLAGS = "0x62_08_flags"  # 0x62[8] flags
# Raw register dumps (diagnostic)
CONF_RAW_0X00 = "raw_0x00"  # 0x00 raw
CONF_RAW_0X10 = "raw_0x10"  # 0x10 raw
CONF_RAW_0X11 = "raw_0x11"  # 0x11 raw
CONF_RAW_0X20 = "raw_0x20"  # 0x20 raw
CONF_RAW_0X21 = "raw_0x21"  # 0x21 raw
CONF_RAW_0X30 = "raw_0x30"  # 0x30 raw
CONF_RAW_0X60 = "raw_0x60"  # 0x60 raw
CONF_RAW_0X61 = "raw_0x61"  # 0x61 raw
CONF_RAW_0X62 = "raw_0x62"  # 0x62 raw
CONF_RAW_0X63 = "raw_0x63"  # 0x63 raw
CONF_RAW_0X64 = "raw_0x64"  # 0x64 raw
CONF_RAW_0X65 = "raw_0x65"  # 0x65 raw
CONF_RAW_0XA0 = "raw_0xa0"  # 0xA0 raw
CONF_RAW_0XA1 = "raw_0xa1"  # 0xA1 raw

# Units
UNIT_BAR = "bar"
UNIT_LITERS_PER_MINUTE = "l/min"

# Icons
ICON_ALERT = "mdi:alert-circle-outline"
ICON_COMPRESSOR = "mdi:sine-wave"
ICON_HEAT_PUMP = "mdi:heat-pump"
ICON_HOME_THERMOMETER = "mdi:home-thermometer"
ICON_INFO = "mdi:information-outline"
ICON_PUMP = "mdi:pump"
ICON_REFRIGERANT = "mdi:snowflake-thermometer"
ICON_VALVE = "mdi:valve"
ICON_WATER_PUMP = "mdi:water-pump"
ICON_WATER_THERMOMETER = "mdi:water-thermometer"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(DaikinAltherma),
            # ===== SENSORS =====
            # Register 0x10
            cv.Optional(
                CONF_TARGET_EVAPORATOR_TEMPERATURE
            ): sensor.sensor_schema(  # 0x10[6:7]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(
                CONF_TARGET_COND_TEMPERATURE
            ): sensor.sensor_schema(  # 0x10[8:9]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Register 0x20 - Outdoor unit temperatures
            cv.Optional(
                CONF_OUTDOOR_AIR_TEMPERATURE
            ): sensor.sensor_schema(  # 0x20[0:1]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_THERMOMETER,
            ),
            cv.Optional(
                CONF_0X20_OUTDOOR_HEAT_EXCHANGER_TEMPERATURE
            ): sensor.sensor_schema(  # 0x20[2:3]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(
                CONF_DISCHARGE_PIPE_TEMPERATURE
            ): sensor.sensor_schema(  # 0x20[4:5]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(
                CONF_SUCTION_PIPE_TEMPERATURE
            ): sensor.sensor_schema(  # 0x20[6:7]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(
                CONF_OUTDOOR_HEAT_EXCHANGER_MID_TEMPERATURE
            ): sensor.sensor_schema(  # 0x20[8:9]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(
                CONF_0X20_LIQUID_PIPE_TEMPERATURE
            ): sensor.sensor_schema(  # 0x20[10:11]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_0X20_1213_UNKNOWN): sensor.sensor_schema(  # 0x20[12:13]
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_0X20_LOW_PRESSURE): sensor.sensor_schema(  # 0x20[14:15]
                unit_of_measurement=UNIT_BAR,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_PRESSURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_0X20_16_UNKNOWN): sensor.sensor_schema(  # 0x20[16]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x21
            cv.Optional(CONF_INV_PRIMARY_CURRENT): sensor.sensor_schema(  # 0x21[0:1]
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_INV_SECONDARY_CURRENT): sensor.sensor_schema(  # 0x21[2:3]
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_INV_FIN_TEMPERATURE): sensor.sensor_schema(  # 0x21[4:5]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_0X21_1213_UNKNOWN): sensor.sensor_schema(  # 0x21[12:13]
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x30 - Outdoor unit operational data
            cv.Optional(CONF_COMPRESSOR_FREQUENCY): sensor.sensor_schema(  # 0x30[0]
                unit_of_measurement=UNIT_HERTZ,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_FREQUENCY,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_COMPRESSOR,
            ),
            cv.Optional(CONF_FAN_SPEED): sensor.sensor_schema(  # 0x30[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_FAN,
            ),
            cv.Optional(CONF_EXPANSION_VALVE_PULSES): sensor.sensor_schema(  # 0x30[3:4]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_VALVE,
            ),
            cv.Optional(CONF_0X30_05_UNKNOWN): sensor.sensor_schema(  # 0x30[5]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x60 - Indoor operation control
            cv.Optional(CONF_0X60_INDOOR_UNIT_ADDRESS): sensor.sensor_schema(  # 0x60[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_INDOOR_ERROR_CODE): sensor.sensor_schema(  # 0x60[3]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_ALERT,
            ),
            cv.Optional(CONF_INDOOR_UNIT_CODE): sensor.sensor_schema(  # 0x60[4]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_INFO,
            ),
            cv.Optional(CONF_INDOOR_ERROR_TYPE): sensor.sensor_schema(  # 0x60[5]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_ALERT,
            ),
            cv.Optional(CONF_INDOOR_UNIT_CAPACITY): sensor.sensor_schema(  # 0x60[6]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_HOME_THERMOMETER,
            ),
            cv.Optional(CONF_DHW_TANK_SETPOINT): sensor.sensor_schema(  # 0x60[7:8]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_THERMOMETER,
            ),
            cv.Optional(
                CONF_0X60_LEAVING_WATER_SETPOINT
            ): sensor.sensor_schema(  # 0x60[9:10]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_THERMOMETER,
            ),
            cv.Optional(CONF_INDOOR_OPTION_CODE): sensor.sensor_schema(  # 0x60[13]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_INFO,
            ),
            cv.Optional(CONF_INDOOR_SOFTWARE_VERSION): sensor.sensor_schema(  # 0x60[14]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_INFO,
            ),
            cv.Optional(CONF_INDOOR_EEPROM_VERSION): sensor.sensor_schema(  # 0x60[15]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_INFO,
            ),
            cv.Optional(CONF_IU_EEPROM_VERSION): sensor.sensor_schema(  # 0x60[16]
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_INFO,
            ),
            # Register 0x61 - Indoor water temperatures
            cv.Optional(CONF_0X61_INDOOR_UNIT_ADDRESS): sensor.sensor_schema(  # 0x61[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_LEAVING_WATER_TEMPERATURE_BEFORE_BUH
            ): sensor.sensor_schema(  # 0x61[2:3]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_THERMOMETER,
            ),
            cv.Optional(
                CONF_LEAVING_WATER_TEMPERATURE_AFTER_BUH
            ): sensor.sensor_schema(  # 0x61[4:5]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_THERMOMETER,
            ),
            cv.Optional(
                CONF_REFRIGERANT_LIQUID_TEMPERATURE
            ): sensor.sensor_schema(  # 0x61[6:7]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_REFRIGERANT,
            ),
            cv.Optional(
                CONF_INLET_WATER_TEMPERATURE
            ): sensor.sensor_schema(  # 0x61[8:9]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_THERMOMETER,
            ),
            cv.Optional(CONF_DHW_TANK_TEMPERATURE): sensor.sensor_schema(  # 0x61[10:11]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_HEATER,
            ),
            cv.Optional(CONF_ROOM_TEMPERATURE): sensor.sensor_schema(  # 0x61[12:13]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_HOME_THERMOMETER,
            ),
            cv.Optional(
                CONF_EXTERNAL_AMBIENT_TEMPERATURE
            ): sensor.sensor_schema(  # 0x61[14:15]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Register 0x62 - Control flags and flow sensors
            cv.Optional(CONF_0X62_INDOOR_UNIT_ADDRESS): sensor.sensor_schema(  # 0x62[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_0X62_LEAVING_WATER_SETPOINT
            ): sensor.sensor_schema(  # 0x62[3:4]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(
                CONF_ROOM_TEMPERATURE_SETPOINT
            ): sensor.sensor_schema(  # 0x62[5:6]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_THERMOMETER,
            ),
            cv.Optional(CONF_FLOW_RATE): sensor.sensor_schema(  # 0x62[9:10]
                unit_of_measurement=UNIT_LITERS_PER_MINUTE,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_VOLUME_FLOW_RATE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_WATER_PUMP,
            ),
            cv.Optional(CONF_WATER_PRESSURE): sensor.sensor_schema(  # 0x62[11]
                unit_of_measurement=UNIT_BAR,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_PRESSURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_PUMP_SPEED): sensor.sensor_schema(  # 0x62[12]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon=ICON_PUMP,
            ),
            # Register 0x63
            cv.Optional(CONF_0X63_INDOOR_UNIT_ADDRESS): sensor.sensor_schema(  # 0x63[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x64
            cv.Optional(CONF_0X64_INDOOR_UNIT_ADDRESS): sensor.sensor_schema(  # 0x64[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_0X64_0708_UNKNOWN): sensor.sensor_schema(  # 0x64[7:8]
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_0X64_09_UNKNOWN_PUMP): sensor.sensor_schema(  # 0x64[9]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_0X64_12_UNKNOWN): sensor.sensor_schema(  # 0x64[12]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x65
            cv.Optional(CONF_0X65_INDOOR_UNIT_ADDRESS): sensor.sensor_schema(  # 0x65[1]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_OUTLET_WATER_HX_TEMPERATURE
            ): sensor.sensor_schema(  # 0x65[2:3]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Register 0xA0
            cv.Optional(CONF_SUCTION_TEMPERATURE): sensor.sensor_schema(  # 0xA0[0:1]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_REFRIGERANT,
            ),
            cv.Optional(
                CONF_0XA0_OUTDOOR_HEAT_EXCHANGER_TEMPERATURE
            ): sensor.sensor_schema(  # 0xA0[2:3]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_THERMOMETER,
            ),
            cv.Optional(
                CONF_0XA0_LIQUID_PIPE_TEMPERATURE
            ): sensor.sensor_schema(  # 0xA0[4:5]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_THERMOMETER,
            ),
            cv.Optional(CONF_0XA0_PRESSURE): sensor.sensor_schema(  # 0xA0[6:7]
                unit_of_measurement=UNIT_BAR,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_PRESSURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_GAUGE,
            ),
            cv.Optional(CONF_EXPANSION_VALVE_3): sensor.sensor_schema(  # 0xA0[8:9]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_VALVE,
            ),
            cv.Optional(
                CONF_COMPRESSOR_PORT_TEMPERATURE
            ): sensor.sensor_schema(  # 0xA0[14:15]
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon=ICON_COMPRESSOR,
            ),
            # Register 0xA1
            cv.Optional(CONF_0XA1_04_UNKNOWN): sensor.sensor_schema(  # 0xA1[4]
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_0XA1_0506_UNKNOWN): sensor.sensor_schema(  # 0xA1[5:6]
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # ===== BINARY SENSORS =====
            # Register 0x10 - Operation status flags
            cv.Optional(
                CONF_THERMOSTAT_ON
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit7
            cv.Optional(
                CONF_RESTART_STANDBY
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit6
            cv.Optional(
                CONF_STARTUP_CONTROL
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit5
            cv.Optional(
                CONF_DEFROST_OPERATION
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit4
            cv.Optional(
                CONF_OIL_RETURN_OPERATION
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit3
            cv.Optional(
                CONF_PRESSURE_EQUALIZING
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit2
            cv.Optional(
                CONF_DEMAND_SIGNAL
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit1
            cv.Optional(
                CONF_LOW_NOISE_CONTROL
            ): binary_sensor.binary_sensor_schema(),  # 0x10[1] bit0
            cv.Optional(
                CONF_HP_DROP_CONTROL
            ): binary_sensor.binary_sensor_schema(),  # 0x10[11] bit7
            # Register 0x30
            cv.Optional(
                CONF_FOUR_WAY_VALVE
            ): binary_sensor.binary_sensor_schema(  # 0x30[11] bit7
                icon=ICON_VALVE,
            ),
            cv.Optional(
                CONF_HOT_GAS_BYPASS_VALVE
            ): binary_sensor.binary_sensor_schema(  # 0x30[13] bit7
                icon=ICON_VALVE,
            ),
            cv.Optional(CONF_Y2S): binary_sensor.binary_sensor_schema(  # 0x30[13] bit6
                icon=ICON_VALVE,
            ),
            cv.Optional(
                CONF_Y3S
            ): binary_sensor.binary_sensor_schema(),  # 0x30[13] bit5
            # Register 0x60 - Indoor operation control
            cv.Optional(
                CONF_0X60_DATA_ENABLED
            ): binary_sensor.binary_sensor_schema(  # 0x60[0] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_INDOOR_THERMOSTAT_ON
            ): binary_sensor.binary_sensor_schema(),  # 0x60[2] bit3
            cv.Optional(
                CONF_FREEZE_PROTECTION
            ): binary_sensor.binary_sensor_schema(),  # 0x60[2] bit2
            cv.Optional(
                CONF_SILENT_MODE
            ): binary_sensor.binary_sensor_schema(  # 0x60[2] bit1
                icon="mdi:volume-off",
            ),
            cv.Optional(
                CONF_FREEZE_PROTECTION_WATER_PIPING
            ): binary_sensor.binary_sensor_schema(),  # 0x60[2] bit0
            cv.Optional(
                CONF_THERMAL_PROTECTOR_Q1L_BUH
            ): binary_sensor.binary_sensor_schema(),  # 0x60[11] bit6
            cv.Optional(
                CONF_THERMAL_PROTECTOR_BSH
            ): binary_sensor.binary_sensor_schema(),  # 0x60[11] bit5
            cv.Optional(
                CONF_EXTERNAL_HEAT_SOURCE
            ): binary_sensor.binary_sensor_schema(  # 0x60[11] bit4
                device_class=DEVICE_CLASS_HEAT,
            ),
            cv.Optional(
                CONF_TWO_THREE_WAY_VALVE
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit7
                icon=ICON_VALVE,
            ),
            cv.Optional(
                CONF_THREE_FOUR_WAY_VALVE
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit6
                icon=ICON_VALVE,
            ),
            cv.Optional(
                CONF_BOOSTER_HEATER
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit5
                device_class=DEVICE_CLASS_HEAT,
            ),
            cv.Optional(
                CONF_BACKUP_HEATER_STEP1
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit4
                device_class=DEVICE_CLASS_HEAT,
            ),
            cv.Optional(
                CONF_BACKUP_HEATER_STEP2
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit3
                device_class=DEVICE_CLASS_HEAT,
            ),
            cv.Optional(
                CONF_BOTTOM_PLATE_HEATER
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit2
                device_class=DEVICE_CLASS_HEAT,
            ),
            cv.Optional(
                CONF_WATER_PUMP
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit1
                device_class=DEVICE_CLASS_RUNNING,
            ),
            cv.Optional(
                CONF_SOLAR_PUMP
            ): binary_sensor.binary_sensor_schema(  # 0x60[12] bit0
                device_class=DEVICE_CLASS_RUNNING,
            ),
            # Register 0x61
            cv.Optional(
                CONF_0X61_DATA_ENABLED
            ): binary_sensor.binary_sensor_schema(  # 0x61[0] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x62 - Control flags
            cv.Optional(
                CONF_0X62_DATA_ENABLED
            ): binary_sensor.binary_sensor_schema(  # 0x62[0] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_POWERFUL_DHW
            ): binary_sensor.binary_sensor_schema(),  # 0x62[2] bit4
            cv.Optional(
                CONF_SPACE_HEATING
            ): binary_sensor.binary_sensor_schema(),  # 0x62[2] bit3
            cv.Optional(
                CONF_0X62_08_BIT7_UNKNOWN
            ): binary_sensor.binary_sensor_schema(  # 0x62[8] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_0X62_08_BIT6_UNKNOWN
            ): binary_sensor.binary_sensor_schema(  # 0x62[8] bit6
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_0X62_08_BIT5_UNKNOWN
            ): binary_sensor.binary_sensor_schema(  # 0x62[8] bit5
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_SPACE_H_OPERATION_OUTPUT
            ): binary_sensor.binary_sensor_schema(),  # 0x62[8] bit0
            # Register 0x63
            cv.Optional(
                CONF_0X63_DATA_ENABLED
            ): binary_sensor.binary_sensor_schema(  # 0x63[0] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x64
            cv.Optional(
                CONF_0X64_DATA_ENABLED
            ): binary_sensor.binary_sensor_schema(  # 0x64[0] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x65
            cv.Optional(
                CONF_0X65_DATA_ENABLED
            ): binary_sensor.binary_sensor_schema(  # 0x65[0] bit7
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # ===== TEXT SENSORS =====
            # Register 0x10 - Outdoor unit operation
            cv.Optional(CONF_OPERATION_MODE): text_sensor.text_sensor_schema(  # 0x10[0]
                icon=ICON_HEAT_PUMP,
            ),
            cv.Optional(CONF_ERROR_TYPE): text_sensor.text_sensor_schema(  # 0x10[4]
                icon=ICON_ALERT,
            ),
            cv.Optional(CONF_ERROR_CODE): text_sensor.text_sensor_schema(  # 0x10[5]
                icon=ICON_ALERT,
            ),
            cv.Optional(
                CONF_0X10_11_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x10[11] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x30 - Outdoor unit operational data
            cv.Optional(
                CONF_0X30_11_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x30[11] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_0X30_13_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x30[13] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x60 - Indoor operation control
            cv.Optional(
                CONF_INDOOR_OPERATION_MODE
            ): text_sensor.text_sensor_schema(  # 0x60[2]
                icon=ICON_HEAT_PUMP,
            ),
            cv.Optional(
                CONF_0X60_11_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x60[11] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Register 0x62 - Control flags
            cv.Optional(
                CONF_0X62_02_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x62[2] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_0X62_07_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x62[7] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_0X62_08_FLAGS
            ): text_sensor.text_sensor_schema(  # 0x62[8] flags
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Raw register dumps (diagnostic)
            cv.Optional(CONF_RAW_0X00): text_sensor.text_sensor_schema(  # 0x00 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X10): text_sensor.text_sensor_schema(  # 0x10 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X11): text_sensor.text_sensor_schema(  # 0x11 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X20): text_sensor.text_sensor_schema(  # 0x20 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X21): text_sensor.text_sensor_schema(  # 0x21 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X30): text_sensor.text_sensor_schema(  # 0x30 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X60): text_sensor.text_sensor_schema(  # 0x60 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X61): text_sensor.text_sensor_schema(  # 0x61 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X62): text_sensor.text_sensor_schema(  # 0x62 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X63): text_sensor.text_sensor_schema(  # 0x63 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X64): text_sensor.text_sensor_schema(  # 0x64 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0X65): text_sensor.text_sensor_schema(  # 0x65 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0XA0): text_sensor.text_sensor_schema(  # 0xA0 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_RAW_0XA1): text_sensor.text_sensor_schema(  # 0xA1 raw
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # Sensors
    for key, setter in [
        # Register 0x10
        (
            CONF_TARGET_EVAPORATOR_TEMPERATURE,
            "set_target_evaporator_temperature_sensor",
        ),  # 0x10[6:7]
        (
            CONF_TARGET_COND_TEMPERATURE,
            "set_target_cond_temperature_sensor",
        ),  # 0x10[8:9]
        # Register 0x20
        (
            CONF_OUTDOOR_AIR_TEMPERATURE,
            "set_outdoor_air_temperature_sensor",
        ),  # 0x20[0:1]
        (
            CONF_0X20_OUTDOOR_HEAT_EXCHANGER_TEMPERATURE,
            "set_0x20_outdoor_heat_exchanger_temperature_sensor",
        ),  # 0x20[2:3]
        (
            CONF_DISCHARGE_PIPE_TEMPERATURE,
            "set_discharge_pipe_temperature_sensor",
        ),  # 0x20[4:5]
        (
            CONF_SUCTION_PIPE_TEMPERATURE,
            "set_suction_pipe_temperature_sensor",
        ),  # 0x20[6:7]
        (
            CONF_OUTDOOR_HEAT_EXCHANGER_MID_TEMPERATURE,
            "set_outdoor_heat_exchanger_mid_temperature_sensor",
        ),  # 0x20[8:9]
        (
            CONF_0X20_LIQUID_PIPE_TEMPERATURE,
            "set_0x20_liquid_pipe_temperature_sensor",
        ),  # 0x20[10:11]
        (CONF_0X20_1213_UNKNOWN, "set_0x20_1213_unknown_sensor"),  # 0x20[12:13]
        (CONF_0X20_LOW_PRESSURE, "set_0x20_low_pressure_sensor"),  # 0x20[14:15]
        (CONF_0X20_16_UNKNOWN, "set_0x20_16_unknown_sensor"),  # 0x20[16]
        # Register 0x21
        (CONF_INV_PRIMARY_CURRENT, "set_inv_primary_current_sensor"),  # 0x21[0:1]
        (CONF_INV_SECONDARY_CURRENT, "set_inv_secondary_current_sensor"),  # 0x21[2:3]
        (CONF_INV_FIN_TEMPERATURE, "set_inv_fin_temperature_sensor"),  # 0x21[4:5]
        (CONF_0X21_1213_UNKNOWN, "set_0x21_1213_unknown_sensor"),  # 0x21[12:13]
        # Register 0x30
        (CONF_COMPRESSOR_FREQUENCY, "set_compressor_frequency_sensor"),  # 0x30[0]
        (CONF_FAN_SPEED, "set_fan_speed_sensor"),  # 0x30[1]
        (CONF_EXPANSION_VALVE_PULSES, "set_expansion_valve_pulses_sensor"),  # 0x30[3:4]
        (CONF_0X30_05_UNKNOWN, "set_0x30_05_unknown_sensor"),  # 0x30[5]
        # Register 0x60
        (
            CONF_0X60_INDOOR_UNIT_ADDRESS,
            "set_0x60_indoor_unit_address_sensor",
        ),  # 0x60[1]
        (CONF_INDOOR_ERROR_CODE, "set_indoor_error_code_sensor"),  # 0x60[3]
        (CONF_INDOOR_UNIT_CODE, "set_indoor_unit_code_sensor"),  # 0x60[4]
        (CONF_INDOOR_ERROR_TYPE, "set_indoor_error_type_sensor"),  # 0x60[5]
        (CONF_INDOOR_UNIT_CAPACITY, "set_indoor_unit_capacity_sensor"),  # 0x60[6]
        (CONF_DHW_TANK_SETPOINT, "set_dhw_tank_setpoint_sensor"),  # 0x60[7:8]
        (
            CONF_0X60_LEAVING_WATER_SETPOINT,
            "set_0x60_leaving_water_setpoint_sensor",
        ),  # 0x60[9:10]
        (CONF_INDOOR_OPTION_CODE, "set_indoor_option_code_sensor"),  # 0x60[13]
        (
            CONF_INDOOR_SOFTWARE_VERSION,
            "set_indoor_software_version_sensor",
        ),  # 0x60[14]
        (CONF_INDOOR_EEPROM_VERSION, "set_indoor_eeprom_version_sensor"),  # 0x60[15]
        (CONF_IU_EEPROM_VERSION, "set_iu_eeprom_version_sensor"),  # 0x60[16]
        # Register 0x61
        (
            CONF_0X61_INDOOR_UNIT_ADDRESS,
            "set_0x61_indoor_unit_address_sensor",
        ),  # 0x61[1]
        (
            CONF_LEAVING_WATER_TEMPERATURE_BEFORE_BUH,
            "set_leaving_water_temperature_before_buh_sensor",
        ),  # 0x61[2:3]
        (
            CONF_LEAVING_WATER_TEMPERATURE_AFTER_BUH,
            "set_leaving_water_temperature_after_buh_sensor",
        ),  # 0x61[4:5]
        (
            CONF_REFRIGERANT_LIQUID_TEMPERATURE,
            "set_refrigerant_liquid_temperature_sensor",
        ),  # 0x61[6:7]
        (
            CONF_INLET_WATER_TEMPERATURE,
            "set_inlet_water_temperature_sensor",
        ),  # 0x61[8:9]
        (CONF_DHW_TANK_TEMPERATURE, "set_dhw_tank_temperature_sensor"),  # 0x61[10:11]
        (CONF_ROOM_TEMPERATURE, "set_room_temperature_sensor"),  # 0x61[12:13]
        (
            CONF_EXTERNAL_AMBIENT_TEMPERATURE,
            "set_external_ambient_temperature_sensor",
        ),  # 0x61[14:15]
        # Register 0x62
        (
            CONF_0X62_INDOOR_UNIT_ADDRESS,
            "set_0x62_indoor_unit_address_sensor",
        ),  # 0x62[1]
        (
            CONF_0X62_LEAVING_WATER_SETPOINT,
            "set_0x62_leaving_water_setpoint_sensor",
        ),  # 0x62[3:4]
        (
            CONF_ROOM_TEMPERATURE_SETPOINT,
            "set_room_temperature_setpoint_sensor",
        ),  # 0x62[5:6]
        (CONF_FLOW_RATE, "set_flow_rate_sensor"),  # 0x62[9:10]
        (CONF_WATER_PRESSURE, "set_water_pressure_sensor"),  # 0x62[11]
        (CONF_PUMP_SPEED, "set_pump_speed_sensor"),  # 0x62[12]
        # Register 0x63
        (
            CONF_0X63_INDOOR_UNIT_ADDRESS,
            "set_0x63_indoor_unit_address_sensor",
        ),  # 0x63[1]
        # Register 0x64
        (
            CONF_0X64_INDOOR_UNIT_ADDRESS,
            "set_0x64_indoor_unit_address_sensor",
        ),  # 0x64[1]
        (CONF_0X64_0708_UNKNOWN, "set_0x64_0708_unknown_sensor"),  # 0x64[7:8]
        (CONF_0X64_09_UNKNOWN_PUMP, "set_0x64_09_unknown_pump_sensor"),  # 0x64[9]
        (CONF_0X64_12_UNKNOWN, "set_0x64_12_unknown_sensor"),  # 0x64[12]
        # Register 0x65
        (
            CONF_0X65_INDOOR_UNIT_ADDRESS,
            "set_0x65_indoor_unit_address_sensor",
        ),  # 0x65[1]
        (
            CONF_OUTLET_WATER_HX_TEMPERATURE,
            "set_outlet_water_hx_temperature_sensor",
        ),  # 0x65[2:3]
        # Register 0xA0
        (CONF_SUCTION_TEMPERATURE, "set_suction_temperature_sensor"),  # 0xA0[0:1]
        (
            CONF_0XA0_OUTDOOR_HEAT_EXCHANGER_TEMPERATURE,
            "set_0xa0_outdoor_heat_exchanger_temperature_sensor",
        ),  # 0xA0[2:3]
        (
            CONF_0XA0_LIQUID_PIPE_TEMPERATURE,
            "set_0xa0_liquid_pipe_temperature_sensor",
        ),  # 0xA0[4:5]
        (CONF_0XA0_PRESSURE, "set_0xa0_pressure_sensor"),  # 0xA0[6:7]
        (CONF_EXPANSION_VALVE_3, "set_expansion_valve_3_sensor"),  # 0xA0[8:9]
        (
            CONF_COMPRESSOR_PORT_TEMPERATURE,
            "set_compressor_port_temperature_sensor",
        ),  # 0xA0[14:15]
        # Register 0xA1
        (CONF_0XA1_04_UNKNOWN, "set_0xa1_04_unknown_sensor"),  # 0xA1[4]
        (CONF_0XA1_0506_UNKNOWN, "set_0xa1_0506_unknown_sensor"),  # 0xA1[5:6]
    ]:
        if conf := config.get(key):
            sens = await sensor.new_sensor(conf)
            cg.add(getattr(var, setter)(sens))

    # Binary sensors
    for key, setter in [
        # Register 0x10
        (CONF_THERMOSTAT_ON, "set_thermostat_on_binary_sensor"),  # 0x10[1] bit7
        (CONF_RESTART_STANDBY, "set_restart_standby_binary_sensor"),  # 0x10[1] bit6
        (CONF_STARTUP_CONTROL, "set_startup_control_binary_sensor"),  # 0x10[1] bit5
        (CONF_DEFROST_OPERATION, "set_defrost_operation_binary_sensor"),  # 0x10[1] bit4
        (
            CONF_OIL_RETURN_OPERATION,
            "set_oil_return_operation_binary_sensor",
        ),  # 0x10[1] bit3
        (
            CONF_PRESSURE_EQUALIZING,
            "set_pressure_equalizing_binary_sensor",
        ),  # 0x10[1] bit2
        (CONF_DEMAND_SIGNAL, "set_demand_signal_binary_sensor"),  # 0x10[1] bit1
        (CONF_LOW_NOISE_CONTROL, "set_low_noise_control_binary_sensor"),  # 0x10[1] bit0
        (CONF_HP_DROP_CONTROL, "set_hp_drop_control_binary_sensor"),  # 0x10[11] bit7
        # Register 0x30
        (CONF_FOUR_WAY_VALVE, "set_four_way_valve_binary_sensor"),  # 0x30[11] bit7
        (
            CONF_HOT_GAS_BYPASS_VALVE,
            "set_hot_gas_bypass_valve_binary_sensor",
        ),  # 0x30[13] bit7
        (CONF_Y2S, "set_y2s_binary_sensor"),  # 0x30[13] bit6
        (CONF_Y3S, "set_y3s_binary_sensor"),  # 0x30[13] bit5
        # Register 0x60
        (CONF_0X60_DATA_ENABLED, "set_0x60_data_enabled_binary_sensor"),  # 0x60[0] bit7
        (
            CONF_INDOOR_THERMOSTAT_ON,
            "set_indoor_thermostat_on_binary_sensor",
        ),  # 0x60[2] bit3
        (CONF_FREEZE_PROTECTION, "set_freeze_protection_binary_sensor"),  # 0x60[2] bit2
        (CONF_SILENT_MODE, "set_silent_mode_binary_sensor"),  # 0x60[2] bit1
        (
            CONF_FREEZE_PROTECTION_WATER_PIPING,
            "set_freeze_protection_water_piping_binary_sensor",
        ),  # 0x60[2] bit0
        (
            CONF_THERMAL_PROTECTOR_Q1L_BUH,
            "set_thermal_protector_q1l_buh_binary_sensor",
        ),  # 0x60[11] bit6
        (
            CONF_THERMAL_PROTECTOR_BSH,
            "set_thermal_protector_bsh_binary_sensor",
        ),  # 0x60[11] bit5
        (
            CONF_EXTERNAL_HEAT_SOURCE,
            "set_external_heat_source_binary_sensor",
        ),  # 0x60[11] bit4
        (
            CONF_TWO_THREE_WAY_VALVE,
            "set_two_three_way_valve_binary_sensor",
        ),  # 0x60[12] bit7
        (
            CONF_THREE_FOUR_WAY_VALVE,
            "set_three_four_way_valve_binary_sensor",
        ),  # 0x60[12] bit6
        (CONF_BOOSTER_HEATER, "set_booster_heater_binary_sensor"),  # 0x60[12] bit5
        (
            CONF_BACKUP_HEATER_STEP1,
            "set_backup_heater_step1_binary_sensor",
        ),  # 0x60[12] bit4
        (
            CONF_BACKUP_HEATER_STEP2,
            "set_backup_heater_step2_binary_sensor",
        ),  # 0x60[12] bit3
        (
            CONF_BOTTOM_PLATE_HEATER,
            "set_bottom_plate_heater_binary_sensor",
        ),  # 0x60[12] bit2
        (CONF_WATER_PUMP, "set_water_pump_binary_sensor"),  # 0x60[12] bit1
        (CONF_SOLAR_PUMP, "set_solar_pump_binary_sensor"),  # 0x60[12] bit0
        # Register 0x61
        (CONF_0X61_DATA_ENABLED, "set_0x61_data_enabled_binary_sensor"),  # 0x61[0] bit7
        # Register 0x62
        (CONF_0X62_DATA_ENABLED, "set_0x62_data_enabled_binary_sensor"),  # 0x62[0] bit7
        (CONF_POWERFUL_DHW, "set_powerful_dhw_binary_sensor"),  # 0x62[2] bit4
        (CONF_SPACE_HEATING, "set_space_heating_binary_sensor"),  # 0x62[2] bit3
        (
            CONF_0X62_08_BIT7_UNKNOWN,
            "set_0x62_08_bit7_unknown_binary_sensor",
        ),  # 0x62[8] bit7
        (
            CONF_0X62_08_BIT6_UNKNOWN,
            "set_0x62_08_bit6_unknown_binary_sensor",
        ),  # 0x62[8] bit6
        (
            CONF_0X62_08_BIT5_UNKNOWN,
            "set_0x62_08_bit5_unknown_binary_sensor",
        ),  # 0x62[8] bit5
        (
            CONF_SPACE_H_OPERATION_OUTPUT,
            "set_space_h_operation_output_binary_sensor",
        ),  # 0x62[8] bit0
        # Register 0x63
        (CONF_0X63_DATA_ENABLED, "set_0x63_data_enabled_binary_sensor"),  # 0x63[0] bit7
        # Register 0x64
        (CONF_0X64_DATA_ENABLED, "set_0x64_data_enabled_binary_sensor"),  # 0x64[0] bit7
        # Register 0x65
        (CONF_0X65_DATA_ENABLED, "set_0x65_data_enabled_binary_sensor"),  # 0x65[0] bit7
    ]:
        if conf := config.get(key):
            bs = await binary_sensor.new_binary_sensor(conf)
            cg.add(getattr(var, setter)(bs))

    # Text sensors
    for key, setter in [
        # Register 0x10
        (CONF_OPERATION_MODE, "set_operation_mode_text_sensor"),  # 0x10[0]
        (CONF_ERROR_TYPE, "set_error_type_text_sensor"),  # 0x10[4]
        (CONF_ERROR_CODE, "set_error_code_text_sensor"),  # 0x10[5]
        (CONF_0X10_11_FLAGS, "set_0x10_11_flags_text_sensor"),  # 0x10[11] flags
        # Register 0x30
        (CONF_0X30_11_FLAGS, "set_0x30_11_flags_text_sensor"),  # 0x30[11] flags
        (CONF_0X30_13_FLAGS, "set_0x30_13_flags_text_sensor"),  # 0x30[13] flags
        # Register 0x60
        (
            CONF_INDOOR_OPERATION_MODE,
            "set_indoor_operation_mode_text_sensor",
        ),  # 0x60[2]
        (CONF_0X60_11_FLAGS, "set_0x60_11_flags_text_sensor"),  # 0x60[11] flags
        # Register 0x62
        (CONF_0X62_02_FLAGS, "set_0x62_02_flags_text_sensor"),  # 0x62[2] flags
        (CONF_0X62_07_FLAGS, "set_0x62_07_flags_text_sensor"),  # 0x62[7] flags
        (CONF_0X62_08_FLAGS, "set_0x62_08_flags_text_sensor"),  # 0x62[8] flags
        # Raw register dumps
        (CONF_RAW_0X00, "set_raw_0x00_text_sensor"),  # 0x00 raw
        (CONF_RAW_0X10, "set_raw_0x10_text_sensor"),  # 0x10 raw
        (CONF_RAW_0X11, "set_raw_0x11_text_sensor"),  # 0x11 raw
        (CONF_RAW_0X20, "set_raw_0x20_text_sensor"),  # 0x20 raw
        (CONF_RAW_0X21, "set_raw_0x21_text_sensor"),  # 0x21 raw
        (CONF_RAW_0X30, "set_raw_0x30_text_sensor"),  # 0x30 raw
        (CONF_RAW_0X60, "set_raw_0x60_text_sensor"),  # 0x60 raw
        (CONF_RAW_0X61, "set_raw_0x61_text_sensor"),  # 0x61 raw
        (CONF_RAW_0X62, "set_raw_0x62_text_sensor"),  # 0x62 raw
        (CONF_RAW_0X63, "set_raw_0x63_text_sensor"),  # 0x63 raw
        (CONF_RAW_0X64, "set_raw_0x64_text_sensor"),  # 0x64 raw
        (CONF_RAW_0X65, "set_raw_0x65_text_sensor"),  # 0x65 raw
        (CONF_RAW_0XA0, "set_raw_0xa0_text_sensor"),  # 0xA0 raw
        (CONF_RAW_0XA1, "set_raw_0xa1_text_sensor"),  # 0xA1 raw
    ]:
        if conf := config.get(key):
            ts = await text_sensor.new_text_sensor(conf)
            cg.add(getattr(var, setter)(ts))
