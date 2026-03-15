import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, ENTITY_CATEGORY_CONFIG, ICON_KEY_PLUS

from .. import (
    CONF_IO_HOMECONTROL_ID,
    IOHomecontrol,
    io_homecontrol_ns,
    validate_address,
)

DEPENDENCIES = ["io_homecontrol"]

IOHomecontrolPairButton = io_homecontrol_ns.class_(
    "IOHomecontrolPairButton", button.Button, cg.Component
)

CONFIG_SCHEMA = (
    button.button_schema(
        IOHomecontrolPairButton,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_KEY_PLUS,
    )
    .extend(
        {
            cv.GenerateID(CONF_IO_HOMECONTROL_ID): cv.use_id(IOHomecontrol),
            cv.Optional(CONF_ADDRESS): validate_address,
        },
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_IO_HOMECONTROL_ID])
    cg.add(var.set_parent(hub))
    if CONF_ADDRESS in config:
        cg.add(var.set_target_address(config[CONF_ADDRESS]))
