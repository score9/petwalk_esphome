import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID

from . import PetwalkEsphome, petwalk_ns

CONF_PETWALK_ESPHOME_ID = "petwalk_esphome_id"
CONF_BIT = "bit"
CONF_ACTIVE_LOW = "active_low"

PetwalkBitBinarySensor = petwalk_ns.class_(
    "PetwalkBitBinarySensor", binary_sensor.BinarySensor
)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(PetwalkBitBinarySensor).extend(
    {
        cv.GenerateID(CONF_PETWALK_ESPHOME_ID): cv.use_id(PetwalkEsphome),
        # Bit numbering is deliberately 1-based: bit: 1 is the first bit clocked.
        cv.Required(CONF_BIT): cv.int_range(min=1, max=64),
        cv.Optional(CONF_ACTIVE_LOW, default=True): cv.boolean,
    }
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    parent = await cg.get_variable(config[CONF_PETWALK_ESPHOME_ID])

    cg.add(var.set_bit(config[CONF_BIT]))
    cg.add(var.set_active_low(config[CONF_ACTIVE_LOW]))
    cg.add(parent.register_binary_sensor(var))
