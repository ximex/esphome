import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_KEY

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@ximex"]
DEPENDENCIES = ["cc1101"]
MULTI_CONF = False

CONF_IO_HOMECONTROL_ID = "io_homecontrol_id"
CONF_CC1101_ID = "cc1101_id"
CONF_GDO0_PIN = "gdo0_pin"
CONF_SOURCE_ADDRESS = "source_address"
CONF_TX_REPEATS = "tx_repeats"
CONF_PAIRING_MODE = "pairing_mode"
CONF_INITIAL_SEQUENCE = "initial_sequence"
CONF_MIN_RSSI = "min_rssi"

io_homecontrol_ns = cg.esphome_ns.namespace("io_homecontrol")
IOHomecontrol = io_homecontrol_ns.class_("IOHomecontrol", cg.Component)

# CC1101 radio component class reference (resolved at code generation time)
cc1101_ns = cg.esphome_ns.namespace("cc1101")
CC1101Component = cc1101_ns.class_("CC1101Component", cg.Component)


def validate_address(value):
    """Validate a 3-byte io-homecontrol address (0x000000-0xFFFFFF)."""
    value = cv.hex_uint32_t(value)
    if value > 0xFFFFFF:
        raise cv.Invalid("Address must be 3 bytes (0x000000-0xFFFFFF)")
    return value


def validate_key(value):
    """Validate a 16-byte AES-128 key as hex string."""
    value = cv.string_strict(value)
    value = value.replace(" ", "").replace("-", "").replace(":", "")
    if len(value) != 32:
        raise cv.Invalid("Key must be 32 hex characters (16 bytes)")
    try:
        key_bytes = bytes.fromhex(value)
    except ValueError as err:
        raise cv.Invalid("Key must contain only hex characters") from err
    return list(key_bytes)


def validate_config(config):
    """Validate conditional requirements."""
    if not config.get(CONF_PAIRING_MODE, False):
        if CONF_SOURCE_ADDRESS not in config:
            raise cv.Invalid(
                f"'{CONF_SOURCE_ADDRESS}' is required when '{CONF_PAIRING_MODE}' is not enabled"
            )
        if CONF_KEY not in config:
            raise cv.Invalid(
                f"'{CONF_KEY}' is required when '{CONF_PAIRING_MODE}' is not enabled"
            )
    return config


CONFIG_SCHEMA = cv.All(
    cv.only_on_esp32,
    cv.only_with_esp_idf,
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(IOHomecontrol),
            cv.Required(CONF_CC1101_ID): cv.use_id(CC1101Component),
            cv.Required(CONF_GDO0_PIN): cv.int_range(min=0, max=48),
            cv.Optional(CONF_SOURCE_ADDRESS): validate_address,
            cv.Optional(CONF_KEY): validate_key,
            cv.Optional(CONF_TX_REPEATS, default=4): cv.int_range(min=1, max=10),
            cv.Optional(CONF_PAIRING_MODE, default=False): cv.boolean,
            cv.Optional(CONF_INITIAL_SEQUENCE): cv.uint16_t,
            cv.Optional(CONF_MIN_RSSI, default=-90.0): cv.float_range(
                min=-120.0, max=0.0
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_config,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    radio = await cg.get_variable(config[CONF_CC1101_ID])
    cg.add(var.set_cc1101_radio(radio))
    cg.add(var.set_gdo0_pin(config[CONF_GDO0_PIN]))

    cg.add(var.set_pairing_mode(config[CONF_PAIRING_MODE]))
    if CONF_SOURCE_ADDRESS in config:
        cg.add(var.set_source_address(config[CONF_SOURCE_ADDRESS]))
    if CONF_KEY in config:
        cg.add(var.set_key(config[CONF_KEY]))
    cg.add(var.set_tx_repeats(config[CONF_TX_REPEATS]))
    if CONF_INITIAL_SEQUENCE in config:
        cg.add(var.set_initial_sequence(config[CONF_INITIAL_SEQUENCE]))
    cg.add(var.set_min_rssi(config[CONF_MIN_RSSI]))
