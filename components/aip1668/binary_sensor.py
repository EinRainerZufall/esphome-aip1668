import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv

from . import CONF_AIP1668_ID, AIP1668Component

DEPENDENCIES = ["aip1668"]

CONF_KEY_BYTE = "key_byte"
CONF_BIT = "bit"

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema().extend(
    {
        cv.GenerateID(CONF_AIP1668_ID): cv.use_id(AIP1668Component),
        # Tastenbyte aus dem Lesebefehl 0x42 (0 bis 4)
        cv.Required(CONF_KEY_BYTE): cv.int_range(min=0, max=4),
        cv.Required(CONF_BIT): cv.int_range(min=0, max=7),
    }
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    parent = await cg.get_variable(config[CONF_AIP1668_ID])
    cg.add(parent.register_key(var, config[CONF_KEY_BYTE], config[CONF_BIT]))
