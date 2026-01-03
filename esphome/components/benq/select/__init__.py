import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ENTITY_CATEGORY, CONF_ICON

from .. import BENQ_COMMANDS, CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqSelect = benq_ns.class_("BenqSelect", select.Select, cg.Component)

SELECT_COMMAND_DEFAULTS = {
    "SOURCE": {"icon": "mdi:video-input-hdmi"},
    "PICTURE_MODE": {"icon": "mdi:image-filter-hdr"},
    "LAMP_MODE": {
        "icon": "mdi:lightbulb",
        "entity_category": "config",
    },
    "ASPECT_RATIO": {"icon": "mdi:aspect-ratio"},
    "BAUD_RATE": {
        "icon": "mdi:serial-port",
        "entity_category": "config",
    },
}

# Default options per command
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
    "BAUD_RATE": ["9600", "14400", "19200", "38400", "57600", "115200"],
}


def _apply_defaults(config):
    cmd = config.get(CONF_COMMAND)
    if cmd in SELECT_COMMAND_DEFAULTS:
        defaults = SELECT_COMMAND_DEFAULTS[cmd]
        if CONF_ICON not in config:
            config[CONF_ICON] = defaults["icon"]
        if CONF_ENTITY_CATEGORY not in config and "entity_category" in defaults:
            config[CONF_ENTITY_CATEGORY] = defaults["entity_category"]
    return config


CONFIG_SCHEMA = cv.All(
    select.select_schema(BenqSelect).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
            cv.Required(CONF_COMMAND): cv.one_of(*BENQ_COMMANDS, upper=True),
        }
    ),
    _apply_defaults,
)


async def to_code(config):
    cmd = config[CONF_COMMAND]
    options = SELECT_COMMAND_OPTIONS.get(cmd, [])
    var = await select.new_select(config, options=options)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_command(BENQ_COMMANDS[cmd]))
    cg.add(parent.register_select(var))
