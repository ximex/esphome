import esphome.codegen as cg
from esphome.components import media_player
import esphome.config_validation as cv
from esphome.const import CONF_ICON
from esphome.types import ConfigType

from .. import CONF_BENQ_ID, BenQ, benq_ns

DEPENDENCIES = ["benq"]
CODEOWNERS = ["@ximex"]

BenqMediaPlayer = benq_ns.class_(
    "BenqMediaPlayer", media_player.MediaPlayer, cg.Component
)


def _apply_defaults(config: ConfigType) -> ConfigType:
    # Runs before the schema so the icon passes the regular validator
    config.setdefault(CONF_ICON, "mdi:projector")
    return config


CONFIG_SCHEMA = cv.All(
    _apply_defaults,
    media_player.media_player_schema(BenqMediaPlayer).extend(
        {
            cv.GenerateID(CONF_BENQ_ID): cv.use_id(BenQ),
        }
    ),
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_BENQ_ID])
    var = await media_player.new_media_player(config, parent)
    await cg.register_component(var, config)
    cg.add(parent.set_media_player(var))
