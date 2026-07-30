import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button, remote_base, time
from esphome.const import CONF_ADDRESS, CONF_ID

from .. import PetwalkEsphome, petwalk_ns
from .text_sensor import PetwalkDisplayTextSensor

DEPENDENCIES = ["petwalk_esphome", "remote_transmitter", "time"]

CONF_PETWALK_ESPHOME_ID = "petwalk_esphome_id"
CONF_DISPLAY_ID = "display_id"
CONF_TIME_ID = "time_id"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_MENU_COMMAND = "menu_command"
CONF_UP_COMMAND = "up_command"
CONF_DOWN_COMMAND = "down_command"
CONF_OK_COMMAND = "ok_command"
CONF_TIME_PROGRAM_COMMAND = "time_program_command"
CONF_REPEAT_WAIT_TIME = "repeat_wait_time"
CONF_SECOND_PRESS_DELAY = "second_press_delay"
CONF_STEP_TIMEOUT = "step_timeout"
CONF_STATE_TIMEOUT = "state_timeout"
CONF_TARGET_LEAD_MINUTES = "target_lead_minutes"

PetwalkClockSyncButton = petwalk_ns.class_(
    "PetwalkClockSyncButton", button.Button, cg.Component
)

CONFIG_SCHEMA = button.button_schema(PetwalkClockSyncButton).extend(
    {
        cv.GenerateID(CONF_PETWALK_ESPHOME_ID): cv.use_id(PetwalkEsphome),
        cv.GenerateID(CONF_DISPLAY_ID): cv.use_id(PetwalkDisplayTextSensor),
        cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(remote_base.RemoteTransmitterBase),
        cv.Optional(CONF_ADDRESS, default=0x04): cv.hex_int_range(min=0x00, max=0x1F),
        cv.Optional(CONF_MENU_COMMAND, default=0x0C): cv.hex_int_range(min=0x00, max=0x7F),
        cv.Optional(CONF_UP_COMMAND, default=0x0E): cv.hex_int_range(min=0x00, max=0x7F),
        cv.Optional(CONF_DOWN_COMMAND, default=0x14): cv.hex_int_range(min=0x00, max=0x7F),
        cv.Optional(CONF_OK_COMMAND, default=0x11): cv.hex_int_range(min=0x00, max=0x7F),
        cv.Optional(CONF_TIME_PROGRAM_COMMAND, default=0x15): cv.hex_int_range(min=0x00, max=0x7F),
        cv.Optional(CONF_REPEAT_WAIT_TIME, default="82ms"): cv.positive_time_period_microseconds,
        cv.Optional(CONF_SECOND_PRESS_DELAY, default="2s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_STEP_TIMEOUT, default="2500ms"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_STATE_TIMEOUT, default="5s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TARGET_LEAD_MINUTES, default=2): cv.int_range(min=1, max=10),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_PETWALK_ESPHOME_ID])
    display = await cg.get_variable(config[CONF_DISPLAY_ID])
    clock = await cg.get_variable(config[CONF_TIME_ID])
    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])

    cg.add(var.set_parent(parent))
    cg.add(var.set_display(display))
    cg.add(var.set_clock(clock))
    cg.add(var.set_transmitter(transmitter))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_menu_command(config[CONF_MENU_COMMAND]))
    cg.add(var.set_up_command(config[CONF_UP_COMMAND]))
    cg.add(var.set_down_command(config[CONF_DOWN_COMMAND]))
    cg.add(var.set_ok_command(config[CONF_OK_COMMAND]))
    cg.add(var.set_time_program_command(config[CONF_TIME_PROGRAM_COMMAND]))
    cg.add(var.set_repeat_wait(config[CONF_REPEAT_WAIT_TIME].total_microseconds))
    cg.add(var.set_second_press_delay(config[CONF_SECOND_PRESS_DELAY].total_milliseconds))
    cg.add(var.set_step_timeout(config[CONF_STEP_TIMEOUT].total_milliseconds))
    cg.add(var.set_state_timeout(config[CONF_STATE_TIMEOUT].total_milliseconds))
    cg.add(var.set_target_lead_minutes(config[CONF_TARGET_LEAD_MINUTES]))
    cg.add(parent.register_clock_sync_button(var))
