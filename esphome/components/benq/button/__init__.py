import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ICON, CONF_VALUE

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqButton = benq_ns.class_("BenqButton", button.Button, cg.Component)

BUTTON_COMMAND_DEFAULTS = {
    "UP": {"icon": "mdi:arrow-up"},
    "DOWN": {"icon": "mdi:arrow-down"},
    "LEFT": {"icon": "mdi:arrow-left"},
    "RIGHT": {"icon": "mdi:arrow-right"},
    "ENTER": {"icon": "mdi:keyboard-return"},
    "AUTO": {"icon": "mdi:auto-fix"},
    "LAMP_HOUR_RESET": {"icon": "mdi:restart"},
}


def _apply_defaults(config):
    cmd = config.get(CONF_COMMAND)
    if cmd in BUTTON_COMMAND_DEFAULTS:
        defaults = BUTTON_COMMAND_DEFAULTS[cmd]
        if CONF_ICON not in config:
            config[CONF_ICON] = defaults["icon"]
    return config


CONFIG_SCHEMA = cv.All(
    button.button_schema(BenqButton).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*BENQ_COMMANDS, upper=True),
            cv.Optional(CONF_VALUE): cv.string,
        }
    ),
    _apply_defaults,
)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_command(BENQ_COMMANDS[config[CONF_COMMAND]]))
    if value := config.get(CONF_VALUE):
        cg.add(var.set_value(value))
