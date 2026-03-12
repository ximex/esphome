from esphome import pins
import esphome.codegen as cg
from esphome.components import spi
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@ximex"]
DEPENDENCIES = ["spi"]

CONF_MILIGHT_ID = "milight_id"
CONF_CE_PIN = "ce_pin"
CONF_PACKET_REPEATS = "packet_repeats"
CONF_LISTEN = "listen"
CONF_MI_DEVICE_ID = "mi_device_id"
CONF_GROUP_ID = "group_id"
CONF_REMOTE_TYPE = "remote_type"
CONF_RF24_POWER_LEVEL = "rf24_power_level"
CONF_PACKET_REPEATS_PER_LOOP = "packet_repeats_per_loop"
CONF_PACKET_REPEAT_THROTTLE_THRESHOLD = "packet_repeat_throttle_threshold"
CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY = "packet_repeat_throttle_sensitivity"
CONF_PACKET_REPEAT_MINIMUM = "packet_repeat_minimum"

milight_ns = cg.esphome_ns.namespace("milight")
MiLightHub = milight_ns.class_("MiLightHub", cg.Component, spi.SPIDevice)

Rf24PowerLevel = milight_ns.enum("Rf24PowerLevel", is_class=True)
RF24_POWER_LEVELS = {
    "MIN": Rf24PowerLevel.MIN,
    "LOW": Rf24PowerLevel.LOW,
    "HIGH": Rf24PowerLevel.HIGH,
    "MAX": Rf24PowerLevel.MAX,
}

RemoteType = milight_ns.enum("RemoteType", is_class=True)
REMOTE_TYPES = {
    "rgbw": RemoteType.RGBW,
    "cct": RemoteType.CCT,
    "rgb_cct": RemoteType.RGB_CCT,
    "rgb": RemoteType.RGB,
    "fut020": RemoteType.FUT020,
    "fut089": RemoteType.FUT089,
    "fut091": RemoteType.FUT091,
}


def validate_group_id(config):
    """Validate group_id is within range for the remote type."""
    remote_type = config[CONF_REMOTE_TYPE]
    group_id = config[CONF_GROUP_ID]
    if remote_type == "fut089":
        max_group = 8
    elif remote_type in ("rgb", "fut020"):
        max_group = 1
    else:
        max_group = 4
    if group_id < 0 or group_id > max_group:
        raise cv.Invalid(
            f"group_id must be 0-{max_group} for remote type '{remote_type}'"
        )
    return config


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MiLightHub),
            cv.Required(CONF_CE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_PACKET_REPEATS, default=50): cv.int_range(min=1, max=200),
            cv.Optional(CONF_LISTEN, default=True): cv.boolean,
            cv.Optional(CONF_RF24_POWER_LEVEL, default="MAX"): cv.enum(
                RF24_POWER_LEVELS, upper=True
            ),
            cv.Optional(CONF_PACKET_REPEATS_PER_LOOP, default=10): cv.int_range(
                min=1, max=50
            ),
            cv.Optional(
                CONF_PACKET_REPEAT_THROTTLE_THRESHOLD, default=200
            ): cv.int_range(min=0, max=5000),
            cv.Optional(
                CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY, default=0
            ): cv.int_range(min=0, max=255),
            cv.Optional(CONF_PACKET_REPEAT_MINIMUM, default=3): cv.int_range(
                min=1, max=200
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add_define("USE_MILIGHT")
    await spi.register_spi_device(var, config)

    ce_pin = await cg.gpio_pin_expression(config[CONF_CE_PIN])
    cg.add(var.set_ce_pin(ce_pin))
    cg.add(var.set_packet_repeats(config[CONF_PACKET_REPEATS]))
    cg.add(var.set_listen(config[CONF_LISTEN]))
    cg.add(var.set_rf24_power_level(config[CONF_RF24_POWER_LEVEL]))
    cg.add(var.set_packet_repeats_per_loop(config[CONF_PACKET_REPEATS_PER_LOOP]))
    cg.add(var.set_throttle_threshold(config[CONF_PACKET_REPEAT_THROTTLE_THRESHOLD]))
    cg.add(
        var.set_throttle_sensitivity(config[CONF_PACKET_REPEAT_THROTTLE_SENSITIVITY])
    )
    cg.add(var.set_throttle_minimum(config[CONF_PACKET_REPEAT_MINIMUM]))
