import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import (
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_DEFAULT_TRANSITION_LENGTH,
    CONF_GAMMA_CORRECT,
    CONF_OUTPUT_ID,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
)

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

MiLightLight = milight_ns.class_("MiLightLight", cg.Component, light.LightOutput)


CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(MiLightLight),
            cv.GenerateID(CONF_MILIGHT_ID): cv.use_id(MiLightHub),
            cv.Required(CONF_MI_DEVICE_ID): cv.hex_uint16_t,
            cv.Required(CONF_GROUP_ID): cv.int_range(min=0, max=8),
            cv.Required(CONF_REMOTE_TYPE): cv.enum(REMOTE_TYPES, lower=True),
            cv.Optional(
                CONF_COLD_WHITE_COLOR_TEMPERATURE, default="153 mireds"
            ): cv.color_temperature,
            cv.Optional(
                CONF_WARM_WHITE_COLOR_TEMPERATURE, default="370 mireds"
            ): cv.color_temperature,
            cv.Optional(
                CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_GAMMA_CORRECT, default=1.0): cv.positive_float,
        }
    ),
    validate_group_id,
    light.validate_color_temperature_channels,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    hub = await cg.get_variable(config[CONF_MILIGHT_ID])
    cg.add(var.set_hub(hub))
    cg.add(var.set_device_id(config[CONF_MI_DEVICE_ID]))
    cg.add(var.set_group_id(config[CONF_GROUP_ID]))
    cg.add(var.set_remote_type(config[CONF_REMOTE_TYPE]))
    cg.add(var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))
    cg.add(var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
