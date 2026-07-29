import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import PetwalkEsphome, petwalk_ns

CONF_PETWALK_ESPHOME_ID = "petwalk_esphome_id"
CONF_BIT = "bit"
CONF_ACTIVE_LOW = "active_low"
CONF_MINIMUM_STATE_TIME = "minimum_state_time"
CONF_MINIMUM_ON_TIME = "minimum_on_time"
CONF_MINIMUM_OFF_TIME = "minimum_off_time"

PetwalkBitBinarySensor = petwalk_ns.class_(
    "PetwalkBitBinarySensor", binary_sensor.BinarySensor
)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(PetwalkBitBinarySensor).extend(
    {
        cv.GenerateID(CONF_PETWALK_ESPHOME_ID): cv.use_id(PetwalkEsphome),
        # Bit numbering is deliberately 1-based: bit: 1 is the first bit clocked.
        cv.Required(CONF_BIT): cv.int_range(min=1, max=64),
        cv.Optional(CONF_ACTIVE_LOW, default=True): cv.boolean,
        # A common minimum duration can be supplied for both transitions. The
        # direction-specific options override it where present.
        cv.Optional(CONF_MINIMUM_STATE_TIME): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MINIMUM_ON_TIME): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MINIMUM_OFF_TIME): cv.positive_time_period_milliseconds,
    }
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    parent = await cg.get_variable(config[CONF_PETWALK_ESPHOME_ID])

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
    cg.add(parent.register_binary_sensor(var))
