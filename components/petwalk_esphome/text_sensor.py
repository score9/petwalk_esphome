import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor

from . import PetwalkEsphome, petwalk_ns

CONF_PETWALK_ESPHOME_ID = "petwalk_esphome_id"
CONF_ACTIVE_LOW = "active_low"
CONF_MINIMUM_STATE_TIME = "minimum_state_time"
CONF_INSERT_TIME_SEPARATOR = "insert_time_separator"
CONF_TIME_SEPARATOR = "time_separator"
CONF_TRIM_SPACES = "trim_spaces"

PetwalkDisplayTextSensor = petwalk_ns.class_(
    "PetwalkDisplayTextSensor", text_sensor.TextSensor
)


def validate_single_character(value):
    value = cv.string(value)
    if len(value) != 1:
        raise cv.Invalid("time_separator must contain exactly one character")
    return value


CONFIG_SCHEMA = text_sensor.text_sensor_schema(PetwalkDisplayTextSensor).extend(
    {
        cv.GenerateID(CONF_PETWALK_ESPHOME_ID): cv.use_id(PetwalkEsphome),
        cv.Optional(CONF_ACTIVE_LOW, default=True): cv.boolean,
        cv.Optional(CONF_MINIMUM_STATE_TIME, default="0ms"):
            cv.positive_time_period_milliseconds,
        cv.Optional(CONF_INSERT_TIME_SEPARATOR, default=True): cv.boolean,
        cv.Optional(CONF_TIME_SEPARATOR, default="."): validate_single_character,
        cv.Optional(CONF_TRIM_SPACES, default=True): cv.boolean,
    }
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)
    parent = await cg.get_variable(config[CONF_PETWALK_ESPHOME_ID])

    cg.add(var.set_active_low(config[CONF_ACTIVE_LOW]))
    cg.add(
        var.set_minimum_state_time(
            config[CONF_MINIMUM_STATE_TIME].total_milliseconds
        )
    )
    cg.add(var.set_insert_time_separator(config[CONF_INSERT_TIME_SEPARATOR]))
    cg.add(var.set_time_separator(config[CONF_TIME_SEPARATOR]))
    cg.add(var.set_trim_spaces(config[CONF_TRIM_SPACES]))
    cg.add(parent.register_text_sensor(var))
