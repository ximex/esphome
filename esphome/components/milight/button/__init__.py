import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_ICON, CONF_ID, ENTITY_CATEGORY_CONFIG

from .. import (
    CONF_GROUP_ID,
    CONF_MI_DEVICE_ID,
    CONF_MILIGHT_ID,
    CONF_REMOTE_TYPE,
    REMOTE_TYPES,
    MiLightHub,
    milight_ns,
    validate_group_id,
)

DEPENDENCIES = ["milight"]

CONF_COMMAND = "command"

MiLightButton = milight_ns.class_("MiLightButton", button.Button, cg.Component)

COMMANDS = {
    "pair": milight_ns.MILIGHT_CMD_PAIR,
    "unpair": milight_ns.MILIGHT_CMD_UNPAIR,
}

COMMAND_ICONS = {
    "pair": "mdi:link-variant",
    "unpair": "mdi:link-variant-off",
}


def _set_default_icon(config):
    if CONF_ICON not in config:
        config[CONF_ICON] = COMMAND_ICONS[config[CONF_COMMAND]]
    return config


CONFIG_SCHEMA = cv.All(
    button.button_schema(MiLightButton, entity_category=ENTITY_CATEGORY_CONFIG)
    .extend(
        {
            cv.GenerateID(CONF_MILIGHT_ID): cv.use_id(MiLightHub),
            cv.Required(CONF_MI_DEVICE_ID): cv.hex_uint16_t,
            cv.Required(CONF_GROUP_ID): cv.int_range(min=0, max=8),
            cv.Required(CONF_REMOTE_TYPE): cv.enum(REMOTE_TYPES, lower=True),
            cv.Required(CONF_COMMAND): cv.enum(COMMANDS, lower=True),
        }
    )
    .extend(cv.COMPONENT_SCHEMA),
    validate_group_id,
    _set_default_icon,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)

    hub = await cg.get_variable(config[CONF_MILIGHT_ID])
    cg.add(var.set_hub(hub))
    cg.add(var.set_device_id(config[CONF_MI_DEVICE_ID]))
    cg.add(var.set_group_id(config[CONF_GROUP_ID]))
    cg.add(var.set_remote_type(config[CONF_REMOTE_TYPE]))
    cg.add(var.set_command(config[CONF_COMMAND]))
