import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_MODEL

CODEOWNERS = ["@ximex"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_BENQ_ID = "benq_id"

benq_ns = cg.esphome_ns.namespace("benq")
BenQ = benq_ns.class_("BenQ", uart.UARTDevice, cg.PollingComponent)
BenqCommand = benq_ns.enum("BenqCommand", is_class=True)

# Enum values matching C++ BenqCommand enum order
BENQ_COMMANDS = {
    "POWER": BenqCommand.POWER,
    "SOURCE": BenqCommand.SOURCE,
    "MUTE": BenqCommand.MUTE,
    "VOLUME": BenqCommand.VOLUME,
    "PICTURE_MODE": BenqCommand.PICTURE_MODE,
    "CONTRAST": BenqCommand.CONTRAST,
    "BRIGHTNESS": BenqCommand.BRIGHTNESS,
    "COLOR": BenqCommand.COLOR,
    "SHARPNESS": BenqCommand.SHARPNESS,
    "ASPECT_RATIO": BenqCommand.ASPECT_RATIO,
    "BLANK": BenqCommand.BLANK,
    "FREEZE": BenqCommand.FREEZE,
    "MENU": BenqCommand.MENU,
    "LAMP_MODE": BenqCommand.LAMP_MODE,
    "LAMP_TIME": BenqCommand.LAMP_TIME,
    "LAMP_HOUR_RESET": BenqCommand.LAMP_HOUR_RESET,
    "UP": BenqCommand.UP,
    "DOWN": BenqCommand.DOWN,
    "LEFT": BenqCommand.LEFT,
    "RIGHT": BenqCommand.RIGHT,
    "ENTER": BenqCommand.ENTER,
    "BAUD_RATE": BenqCommand.BAUD_RATE,
    "AUTO": BenqCommand.AUTO,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BenQ),
            cv.Optional(CONF_MODEL, default="GENERIC"): cv.string_strict,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "benq",
    require_tx=True,
    require_rx=True,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_model(config[CONF_MODEL]))
