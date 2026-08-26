import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ICON
from esphome.types import ConfigType

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, apply_command_defaults, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqSwitch = benq_ns.class_("BenqSwitch", switch.Switch, cg.Component)

# Only the commands the projector accepts as "on"/"off" work as a switch.
SWITCH_COMMAND_DEFAULTS = {
    "POWER": {CONF_ICON: "mdi:projector"},
    "BLANK": {CONF_ICON: "mdi:monitor-off"},
    "FREEZE": {CONF_ICON: "mdi:pause"},
    "MUTE": {CONF_ICON: "mdi:volume-off"},
    "MENU": {CONF_ICON: "mdi:menu"},
}

CONFIG_SCHEMA = cv.All(
    apply_command_defaults(SWITCH_COMMAND_DEFAULTS),
    switch.switch_schema(BenqSwitch).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*SWITCH_COMMAND_DEFAULTS, upper=True),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    var = await switch.new_switch(config, parent, BENQ_COMMANDS[config[CONF_COMMAND]])
    await cg.register_component(var, config)
