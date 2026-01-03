import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ENTITY_CATEGORY, CONF_ICON, CONF_MODE

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqNumber = benq_ns.class_("BenqNumber", number.Number, cg.Component)

# Default min/max/step and name/icon per command
NUMBER_COMMAND_RANGES = {
    "VOLUME": (0, 10, 1),
    "BRIGHTNESS": (0, 100, 1),
    "CONTRAST": (-50, 50, 1),
    "COLOR": (-50, 50, 1),
    "SHARPNESS": (0, 31, 1),
}

NUMBER_COMMAND_DEFAULTS = {
    "VOLUME": {"icon": "mdi:volume-high", "mode": "slider"},
    "BRIGHTNESS": {"icon": "mdi:brightness-6", "mode": "slider"},
    "CONTRAST": {"icon": "mdi:contrast-box", "mode": "slider"},
    "COLOR": {"icon": "mdi:palette", "mode": "slider"},
    "SHARPNESS": {
        "icon": "mdi:image-filter-center-focus",
        "mode": "slider",
    },
}


def _apply_defaults(config):
    cmd = config.get(CONF_COMMAND)
    if cmd in NUMBER_COMMAND_DEFAULTS:
        defaults = NUMBER_COMMAND_DEFAULTS[cmd]
        if CONF_ICON not in config:
            config[CONF_ICON] = defaults["icon"]
        if CONF_MODE not in config and "mode" in defaults:
            config[CONF_MODE] = defaults["mode"]
        if CONF_ENTITY_CATEGORY not in config and "entity_category" in defaults:
            config[CONF_ENTITY_CATEGORY] = defaults["entity_category"]
    return config


CONFIG_SCHEMA = cv.All(
    number.number_schema(BenqNumber).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*BENQ_COMMANDS, upper=True),
        }
    ),
    _apply_defaults,
)


async def to_code(config):
    cmd = config[CONF_COMMAND]
    min_val, max_val, step = NUMBER_COMMAND_RANGES.get(cmd, (0, 100, 1))
    var = await number.new_number(
        config, min_value=min_val, max_value=max_val, step=step
    )
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_command(BENQ_COMMANDS[cmd]))
    cg.add(parent.register_number(var))
