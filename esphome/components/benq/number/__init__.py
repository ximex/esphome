import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ICON, CONF_MODE
from esphome.types import ConfigType

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, apply_command_defaults, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqNumber = benq_ns.class_("BenqNumber", number.Number, cg.Component)

# Only commands that take a numeric value work as a number, and each one has
# its own range: command -> (min, max, step)
NUMBER_COMMAND_RANGES = {
    "VOLUME": (0, 10, 1),
    "BRIGHTNESS": (0, 100, 1),
    "CONTRAST": (-50, 50, 1),
    "COLOR": (-50, 50, 1),
    "SHARPNESS": (0, 31, 1),
}

NUMBER_COMMAND_DEFAULTS = {
    "VOLUME": {CONF_ICON: "mdi:volume-high", CONF_MODE: "slider"},
    "BRIGHTNESS": {CONF_ICON: "mdi:brightness-6", CONF_MODE: "slider"},
    "CONTRAST": {CONF_ICON: "mdi:contrast-box", CONF_MODE: "slider"},
    "COLOR": {CONF_ICON: "mdi:palette", CONF_MODE: "slider"},
    "SHARPNESS": {
        CONF_ICON: "mdi:image-filter-center-focus",
        CONF_MODE: "slider",
    },
}

CONFIG_SCHEMA = cv.All(
    apply_command_defaults(NUMBER_COMMAND_DEFAULTS),
    number.number_schema(BenqNumber).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*NUMBER_COMMAND_RANGES, upper=True),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    cmd = config[CONF_COMMAND]
    min_val, max_val, step = NUMBER_COMMAND_RANGES[cmd]
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    var = await number.new_number(
        config,
        parent,
        BENQ_COMMANDS[cmd],
        min_value=min_val,
        max_value=max_val,
        step=step,
    )
    await cg.register_component(var, config)
