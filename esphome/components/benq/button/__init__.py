import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ICON, CONF_VALUE
from esphome.types import ConfigType

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, apply_command_defaults, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqButton = benq_ns.class_("BenqButton", button.Button, cg.Component)

BUTTON_COMMAND_DEFAULTS = {
    "UP": {CONF_ICON: "mdi:arrow-up"},
    "DOWN": {CONF_ICON: "mdi:arrow-down"},
    "LEFT": {CONF_ICON: "mdi:arrow-left"},
    "RIGHT": {CONF_ICON: "mdi:arrow-right"},
    "ENTER": {CONF_ICON: "mdi:keyboard-return"},
    "AUTO": {CONF_ICON: "mdi:auto-fix"},
    "LAMP_HOUR_RESET": {CONF_ICON: "mdi:restart"},
}

# Unlike the other platforms, a button accepts every command. Sending one is
# always harmless, and together with ``value`` this is the escape hatch for
# commands that have no platform of their own.
CONFIG_SCHEMA = cv.All(
    apply_command_defaults(BUTTON_COMMAND_DEFAULTS),
    button.button_schema(BenqButton).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*BENQ_COMMANDS, upper=True),
            cv.Optional(CONF_VALUE): cv.string,
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    var = await button.new_button(config, parent, BENQ_COMMANDS[config[CONF_COMMAND]])
    await cg.register_component(var, config)
    if value := config.get(CONF_VALUE):
        cg.add(var.set_value(value))
