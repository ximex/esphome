import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import (
    CONF_COMMAND,
    CONF_ENTITY_CATEGORY,
    CONF_ICON,
    ENTITY_CATEGORY_CONFIG,
)
from esphome.types import ConfigType

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, apply_command_defaults, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqSelect = benq_ns.class_("BenqSelect", select.Select, cg.Component)

# The option list is what makes a select usable, so it also decides which
# commands the platform accepts. BAUD_RATE is deliberately absent: changing the
# projector's baud rate would cut the connection, because the UART on this side
# stays at its configured speed. Use a button with a value if you really need
# to send it.
SELECT_COMMAND_OPTIONS = {
    "SOURCE": [
        "RGB",
        "RGB2",
        "YPBR",
        "YPBR2",
        "DVI-A",
        "DVI-D",
        "HDMI",
        "HDMI2",
        "HDMI3",
        "HDBASET",
    ],
    "PICTURE_MODE": [
        "PRESENTATION",
        "BRIGHT",
        "CINEMA",
        "STD",
        "USER1",
        "USER2",
        "USER3",
    ],
    "LAMP_MODE": ["NORMAL", "ECO", "DUAL", "SINGLE"],
    "ASPECT_RATIO": ["4:3", "16:9", "16:10", "AUTO", "REAL"],
}

SELECT_COMMAND_DEFAULTS = {
    "SOURCE": {CONF_ICON: "mdi:video-input-hdmi"},
    "PICTURE_MODE": {CONF_ICON: "mdi:image-filter-hdr"},
    "LAMP_MODE": {
        CONF_ICON: "mdi:lightbulb",
        CONF_ENTITY_CATEGORY: ENTITY_CATEGORY_CONFIG,
    },
    "ASPECT_RATIO": {CONF_ICON: "mdi:aspect-ratio"},
}

CONFIG_SCHEMA = cv.All(
    apply_command_defaults(SELECT_COMMAND_DEFAULTS),
    select.select_schema(BenqSelect).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*SELECT_COMMAND_OPTIONS, upper=True),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    cmd = config[CONF_COMMAND]
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    var = await select.new_select(
        config, parent, BENQ_COMMANDS[cmd], options=SELECT_COMMAND_OPTIONS[cmd]
    )
    await cg.register_component(var, config)
