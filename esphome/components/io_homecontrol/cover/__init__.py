import esphome.codegen as cg
from esphome.components import cover
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS

from .. import (
    CONF_IO_HOMECONTROL_ID,
    IOHomecontrol,
    io_homecontrol_ns,
    validate_address,
)

DEPENDENCIES = ["io_homecontrol"]

IOHomecontrolCover = io_homecontrol_ns.class_(
    "IOHomecontrolCover", cover.Cover, cg.Component
)

CONFIG_SCHEMA = (
    cover.cover_schema(IOHomecontrolCover, device_class="blind")
    .extend(
        {
            cv.GenerateID(CONF_IO_HOMECONTROL_ID): cv.use_id(IOHomecontrol),
            cv.Required(CONF_ADDRESS): cv.ensure_list(validate_address),
        },
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)

    cg.add_define("USE_IO_HOMECONTROL_COVER")
    hub = await cg.get_variable(config[CONF_IO_HOMECONTROL_ID])
    cg.add(var.set_parent(hub))
    for addr in config[CONF_ADDRESS]:
        cg.add(var.add_address(addr))
