import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.const import CONF_ICON

from .. import CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqMediaPlayer = benq_ns.class_(
    "BenqMediaPlayer", media_player.MediaPlayer, cg.Component
)

MEDIA_PLAYER_DEFAULTS = {
    "icon": "mdi:projector",
}


def _apply_defaults(config):
    if CONF_ICON not in config:
        config[CONF_ICON] = MEDIA_PLAYER_DEFAULTS["icon"]
    return config


CONFIG_SCHEMA = cv.All(
    media_player.media_player_schema(BenqMediaPlayer).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
        }
    ),
    _apply_defaults,
)


async def to_code(config):
    var = await media_player.new_media_player(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_media_player(var))
