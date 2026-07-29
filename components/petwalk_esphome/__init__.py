import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

CODEOWNERS = []
MULTI_CONF = True

CONF_DATA_PIN = "data_pin"
CONF_CLOCK_PIN = "clock_pin"
CONF_LATCH_PIN = "latch_pin"
CONF_FRAME_BITS = "frame_bits"
CONF_CLOCK_EDGE = "clock_edge"
CONF_LATCH_EDGE = "latch_edge"
CONF_DEBUG_FRAMES = "debug_frames"

petwalk_ns = cg.esphome_ns.namespace("petwalk_esphome")
PetwalkEsphome = petwalk_ns.class_("PetwalkEsphome", cg.Component)

ClockEdge = petwalk_ns.enum("ClockEdge")
CLOCK_EDGES = {
    "FALLING": ClockEdge.CLOCK_EDGE_FALLING,
    "RISING": ClockEdge.CLOCK_EDGE_RISING,
}

LatchEdge = petwalk_ns.enum("LatchEdge")
LATCH_EDGES = {
    "FALLING": LatchEdge.LATCH_EDGE_FALLING,
    "RISING": LatchEdge.LATCH_EDGE_RISING,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PetwalkEsphome),
        cv.Required(CONF_DATA_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_CLOCK_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_LATCH_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_FRAME_BITS, default=56): cv.int_range(min=1, max=64),
        cv.Optional(CONF_CLOCK_EDGE, default="FALLING"): cv.enum(CLOCK_EDGES, upper=True),
        cv.Optional(CONF_LATCH_EDGE, default="RISING"): cv.enum(LATCH_EDGES, upper=True),
        cv.Optional(CONF_DEBUG_FRAMES, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    data_pin = await cg.gpio_pin_expression(config[CONF_DATA_PIN])
    clock_pin = await cg.gpio_pin_expression(config[CONF_CLOCK_PIN])
    latch_pin = await cg.gpio_pin_expression(config[CONF_LATCH_PIN])

    cg.add(var.set_data_pin(data_pin))
    cg.add(var.set_clock_pin(clock_pin))
    cg.add(var.set_latch_pin(latch_pin))
    cg.add(var.set_frame_bits(config[CONF_FRAME_BITS]))
    cg.add(var.set_clock_edge(config[CONF_CLOCK_EDGE]))
    cg.add(var.set_latch_edge(config[CONF_LATCH_EDGE]))
    cg.add(var.set_debug_frames(config[CONF_DEBUG_FRAMES]))
