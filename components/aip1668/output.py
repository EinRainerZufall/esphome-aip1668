import esphome.codegen as cg
from esphome.components import output
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_ID

from . import CONF_AIP1668_ID, AIP1668Component, aip1668_ns

DEPENDENCIES = ["aip1668"]

CONF_BIT = "bit"

AIP1668Output = aip1668_ns.class_("AIP1668Output", output.BinaryOutput)

CONFIG_SCHEMA = output.BINARY_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(AIP1668Output),
        cv.GenerateID(CONF_AIP1668_ID): cv.use_id(AIP1668Component),
        # Adresse im Display-RAM (00H bis 0DH)
        cv.Required(CONF_ADDRESS): cv.int_range(min=0, max=13),
        cv.Required(CONF_BIT): cv.int_range(min=0, max=7),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await output.register_output(var, config)
    parent = await cg.get_variable(config[CONF_AIP1668_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_position(config[CONF_ADDRESS], config[CONF_BIT]))
