import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import remote_base, switch
from esphome.const import (
    CONF_ADDRESS,
    CONF_COMMAND,
    CONF_ID,
    CONF_REPEAT,
    CONF_TIMES,
    CONF_WAIT_TIME,
)

from . import PetwalkEsphome, petwalk_ns

DEPENDENCIES = ["petwalk_esphome", "remote_transmitter"]

CONF_PETWALK_ESPHOME_ID = "petwalk_esphome_id"
CONF_TRANSMITTER_ID = "transmitter_id"
CONF_BIT = "bit"
CONF_ACTIVE_LOW = "active_low"
CONF_MINIMUM_STATE_TIME = "minimum_state_time"
CONF_MINIMUM_ON_TIME = "minimum_on_time"
CONF_MINIMUM_OFF_TIME = "minimum_off_time"

PetwalkBitSwitch = petwalk_ns.class_("PetwalkBitSwitch", switch.Switch)

REPEAT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TIMES): cv.positive_int,
        cv.Optional(CONF_WAIT_TIME, default="25ms"): cv.positive_time_period_microseconds,
    }
)

CONFIG_SCHEMA = switch.switch_schema(PetwalkBitSwitch).extend(
    {
        cv.GenerateID(CONF_PETWALK_ESPHOME_ID): cv.use_id(PetwalkEsphome),
        cv.GenerateID(CONF_TRANSMITTER_ID): cv.use_id(remote_base.RemoteTransmitterBase),
        # 1-based: bit 1 is the first bit sampled after LATCH.
        cv.Required(CONF_BIT): cv.int_range(min=1, max=64),
        cv.Optional(CONF_ACTIVE_LOW, default=True): cv.boolean,
        cv.Optional(CONF_MINIMUM_STATE_TIME): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MINIMUM_ON_TIME): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MINIMUM_OFF_TIME): cv.positive_time_period_milliseconds,
        cv.Required(CONF_ADDRESS): cv.hex_int_range(min=0x00, max=0x1F),
        cv.Required(CONF_COMMAND): cv.hex_int_range(min=0x00, max=0x7F),
        cv.Optional(CONF_REPEAT): REPEAT_SCHEMA,
    }
)


async def to_code(config):
    var = await switch.new_switch(config)
    parent = await cg.get_variable(config[CONF_PETWALK_ESPHOME_ID])
    transmitter = await cg.get_variable(config[CONF_TRANSMITTER_ID])

    cg.add(var.set_bit(config[CONF_BIT]))
    cg.add(var.set_active_low(config[CONF_ACTIVE_LOW]))

    common_ms = 0
    if CONF_MINIMUM_STATE_TIME in config:
        common_ms = config[CONF_MINIMUM_STATE_TIME].total_milliseconds

    minimum_on_ms = common_ms
    minimum_off_ms = common_ms

    if CONF_MINIMUM_ON_TIME in config:
        minimum_on_ms = config[CONF_MINIMUM_ON_TIME].total_milliseconds
    if CONF_MINIMUM_OFF_TIME in config:
        minimum_off_ms = config[CONF_MINIMUM_OFF_TIME].total_milliseconds

    cg.add(var.set_minimum_on_time(minimum_on_ms))
    cg.add(var.set_minimum_off_time(minimum_off_ms))

    cg.add(var.set_transmitter(transmitter))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_command(config[CONF_COMMAND]))

    if CONF_REPEAT in config:
        repeat = config[CONF_REPEAT]
        cg.add(var.set_repeat_times(repeat[CONF_TIMES]))
        cg.add(var.set_repeat_wait(repeat[CONF_WAIT_TIME].total_microseconds))

    cg.add(parent.register_switch(var))
