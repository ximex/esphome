import esphome.codegen as cg
from esphome.components import cover, remote_base
import esphome.config_validation as cv
from esphome.const import CONF_CLOSE_DURATION, CONF_CODE, CONF_OPEN_DURATION

from .. import lifta_ns

DEPENDENCIES = ["remote_transmitter"]
AUTO_LOAD = ["remote_base"]

CONF_FRAME_INTERVAL = "frame_interval"
CONF_TIMEOUT_MARGIN = "timeout_margin"

ICON_ELEVATOR_PASSENGER = "mdi:elevator-passenger"

LiftaCover = lifta_ns.class_(
    "LiftaCover",
    cover.Cover,
    cg.Component,
    remote_base.RemoteTransmittable,
    remote_base.RemoteReceiverListener,
)

CONFIG_SCHEMA = (
    cover.cover_schema(LiftaCover, icon=ICON_ELEVATOR_PASSENGER)
    .extend(
        {
            cv.Required(CONF_CODE): remote_base.validate_lifta_code,
            cv.Required(CONF_OPEN_DURATION): cv.positive_time_period_milliseconds,
            cv.Required(CONF_CLOSE_DURATION): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_FRAME_INTERVAL, default="150ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(
                    min=cv.TimePeriod(milliseconds=50),
                    max=cv.TimePeriod(milliseconds=500),
                ),
            ),
            cv.Optional(CONF_TIMEOUT_MARGIN, default="15s"): cv.Any(
                cv.percentage, cv.positive_time_period_milliseconds
            ),
            cv.Optional(remote_base.CONF_RECEIVER_ID): cv.use_id(
                remote_base.RemoteReceiverBase
            ),
        }
    )
    .extend(remote_base.REMOTE_TRANSMITTABLE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)
    await remote_base.register_transmittable(var, config)

    cg.add(var.set_code(config[CONF_CODE]))
    cg.add(var.set_frame_interval(config[CONF_FRAME_INTERVAL]))
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))

    margin = config[CONF_TIMEOUT_MARGIN]
    if isinstance(margin, float):
        open_margin = int(config[CONF_OPEN_DURATION].total_milliseconds * margin)
        close_margin = int(config[CONF_CLOSE_DURATION].total_milliseconds * margin)
    else:
        open_margin = margin
        close_margin = margin
    cg.add(var.set_open_margin(open_margin))
    cg.add(var.set_close_margin(close_margin))

    if remote_base.CONF_RECEIVER_ID in config:
        await remote_base.register_listener(var, config)
