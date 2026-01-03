import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ENTITY_CATEGORY, CONF_ICON

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqSwitch = benq_ns.class_("BenqSwitch", switch.Switch, cg.Component)

SWITCH_COMMAND_DEFAULTS = {
    "POWER": {"icon": "mdi:projector"},
    "BLANK": {"icon": "mdi:monitor-off"},
    "FREEZE": {"icon": "mdi:pause"},
    "MUTE": {"icon": "mdi:volume-off"},
    "MENU": {"icon": "mdi:menu"},
}


def _apply_defaults(config):
    cmd = config.get(CONF_COMMAND)
    if cmd in SWITCH_COMMAND_DEFAULTS:
        defaults = SWITCH_COMMAND_DEFAULTS[cmd]
        if CONF_ICON not in config:
            config[CONF_ICON] = defaults["icon"]
        if CONF_ENTITY_CATEGORY not in config and "entity_category" in defaults:
            config[CONF_ENTITY_CATEGORY] = defaults["entity_category"]
    return config


CONFIG_SCHEMA = cv.All(
    switch.switch_schema(BenqSwitch).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*BENQ_COMMANDS, upper=True),
        }
    ),
    _apply_defaults,
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_command(BENQ_COMMANDS[config[CONF_COMMAND]]))
    cg.add(parent.register_switch(var))
