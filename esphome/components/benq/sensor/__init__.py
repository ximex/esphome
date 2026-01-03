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

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqSensor = benq_ns.class_("BenqSensor", sensor.Sensor, cg.Component)

# Per-command defaults. The schema itself cannot carry these, because the
# applicable values depend on the command that the user selects.
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


def _apply_defaults(config: ConfigType) -> ConfigType:
    # Runs before the schema, so the injected values pass through the regular
    # validators. state_class in particular is turned into an enum there, and a
    # raw string would end up in the generated C++ and fail to compile.
    if isinstance(command := config.get(CONF_COMMAND), str):
        for key, value in SENSOR_COMMAND_DEFAULTS.get(command.upper(), {}).items():
            config.setdefault(key, value)
    return config


CONFIG_SCHEMA = cv.All(
    _apply_defaults,
    sensor.sensor_schema(BenqSensor).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*BENQ_COMMANDS, upper=True),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_command(BENQ_COMMANDS[config[CONF_COMMAND]]))
    cg.add(parent.register_sensor(var))
