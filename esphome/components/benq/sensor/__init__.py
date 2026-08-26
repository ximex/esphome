import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ACCURACY_DECIMALS,
    CONF_COMMAND,
    CONF_DEVICE_CLASS,
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    CONF_STATE_CLASS,
    CONF_UNIT_OF_MEASUREMENT,
    DEVICE_CLASS_DURATION,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_HOUR,
)
from esphome.types import ConfigType

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, apply_command_defaults, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqSensor = benq_ns.class_("BenqSensor", sensor.Sensor, cg.Component)

# Only commands that answer with a number can drive a sensor; anything else
# would publish "unknown" forever.
SENSOR_COMMANDS = (
    "LAMP_TIME",
    "VOLUME",
    "CONTRAST",
    "BRIGHTNESS",
    "COLOR",
    "SHARPNESS",
)

SENSOR_COMMAND_DEFAULTS = {
    "LAMP_TIME": {
        CONF_UNIT_OF_MEASUREMENT: UNIT_HOUR,
        CONF_ICON: "mdi:lightbulb-on",
        CONF_ACCURACY_DECIMALS: 0,
        CONF_DEVICE_CLASS: DEVICE_CLASS_DURATION,
        CONF_STATE_CLASS: STATE_CLASS_TOTAL_INCREASING,
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_DIAGNOSTIC,
    },
}

CONFIG_SCHEMA = cv.All(
    apply_command_defaults(SENSOR_COMMAND_DEFAULTS),
    sensor.sensor_schema(BenqSensor).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*SENSOR_COMMANDS, upper=True),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    var = await sensor.new_sensor(config, parent, BENQ_COMMANDS[config[CONF_COMMAND]])
    await cg.register_component(var, config)
