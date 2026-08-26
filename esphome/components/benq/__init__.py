from collections.abc import Callable, Mapping
from typing import Any

import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_COMMAND, CONF_ID, CONF_MODEL
from esphome.types import ConfigType

CODEOWNERS = ["@ximex"]
DEPENDENCIES = ["uart"]
MULTI_CONF = True

CONF_BENQ_ID = "benq_id"
CONF_COMMAND_TIMEOUT = "command_timeout"

benq_ns = cg.esphome_ns.namespace("benq")
BenQ = benq_ns.class_("BenQ", uart.UARTDevice, cg.PollingComponent)
BenqCommand = benq_ns.enum("BenqCommand", is_class=True)

# Every command the projector understands. The names are resolved in the
# generated C++ as ``benq::BenqCommand::<NAME>``, so the order here does not
# have to match the C++ enum. Each platform narrows this down to the commands
# that make sense for it.
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


def apply_command_defaults(
    defaults: Mapping[str, Mapping[str, Any]],
) -> Callable[[ConfigType], ConfigType]:
    """Build a validator that fills in the defaults for the chosen command.

    The schema itself cannot carry these values, because which ones apply
    depends on the command the user picked. The returned validator has to run
    *before* the entity schema, for two reasons: injected values then pass
    through the regular validators (``state_class`` and ``mode`` in particular
    become enums there, and a raw string would end up in the generated C++ and
    fail to compile), and keys that the schema gives a default of its own — such
    as ``mode`` — are always present afterwards, so a later pass could never
    fill them in.
    """

    def validate(config: ConfigType) -> ConfigType:
        if isinstance(command := config.get(CONF_COMMAND), str):
            for key, value in defaults.get(command.upper(), {}).items():
                config.setdefault(key, value)
        return config

    return validate


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BenQ),
            cv.Optional(CONF_MODEL, default="GENERIC"): cv.string_strict,
            # How long to wait for a reply. Projectors differ widely here: an
            # SP840 was measured at just over three seconds, while the same
            # unit answered in under one when it was idle.
            cv.Optional(
                CONF_COMMAND_TIMEOUT, default="5s"
            ): cv.positive_time_period_milliseconds,
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


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_model(config[CONF_MODEL]))
    cg.add(var.set_command_timeout(config[CONF_COMMAND_TIMEOUT]))
