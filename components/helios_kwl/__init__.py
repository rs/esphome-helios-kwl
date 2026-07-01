import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]

helios_kwl_component_ns = cg.esphome_ns.namespace("helios_kwl_component")
HeliosKwlComponent = helios_kwl_component_ns.class_("HeliosKwlComponent", cg.PollingComponent, uart.UARTDevice)
SetFanSpeedAction = helios_kwl_component_ns.class_("SetFanSpeedAction", automation.Action)

CONF_HELIOS_KWL_ID = "helios_kwl_id"
CONF_LEVEL = "level"
CONF_REPEAT_FINAL_CHECKSUM = "repeat_final_checksum"
CONF_WRITE_ADDRESS = "write_address"
CONF_WRITE_BUS_IDLE = "write_bus_idle"
CONF_WRITE_CHECKSUM = "write_checksum"
CONF_WRITE_FRAME_DELAY = "write_frame_delay"

WRITE_CHECKSUM_MODES = {
    "mainboard": True,
    "recipient": False,
}


HELIOS_KWL_COMPONENT_SCHEMA = cv.Schema({cv.Required(CONF_HELIOS_KWL_ID): cv.use_id(HeliosKwlComponent)})

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(HeliosKwlComponent),
        cv.Optional(CONF_REPEAT_FINAL_CHECKSUM, default=True): cv.boolean,
        cv.Optional(CONF_WRITE_ADDRESS, default=0x2F): cv.hex_uint8_t,
        cv.Optional(CONF_WRITE_BUS_IDLE, default="30ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_WRITE_CHECKSUM, default="mainboard"): cv.enum(WRITE_CHECKSUM_MODES, lower=True),
        cv.Optional(CONF_WRITE_FRAME_DELAY, default="2ms"): cv.positive_time_period_milliseconds,
    })
    .extend(cv.polling_component_schema("2s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_repeat_final_checksum(config[CONF_REPEAT_FINAL_CHECKSUM]))
    cg.add(var.set_write_address(config[CONF_WRITE_ADDRESS]))
    cg.add(var.set_write_bus_idle_ms(config[CONF_WRITE_BUS_IDLE].total_milliseconds))
    cg.add(var.set_use_mainboard_write_checksum(config[CONF_WRITE_CHECKSUM]))
    cg.add(var.set_write_frame_delay_ms(config[CONF_WRITE_FRAME_DELAY].total_milliseconds))


@automation.register_action(
    "helios_kwl.set_fan_speed",
    SetFanSpeedAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(HeliosKwlComponent),
            cv.Required(CONF_LEVEL): cv.templatable(cv.int_range(min=0, max=8)),
        }
    ),
    synchronous=True,
)
async def set_fan_speed_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    level = await cg.templatable(config[CONF_LEVEL], args, cg.uint8)
    cg.add(var.set_level(level))
    return var
