import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_BRIGHTNESS, CONF_CLK_PIN, CONF_DIO_PIN, CONF_ID

CODEOWNERS = []
MULTI_CONF = True

CONF_AIP1668_ID = "aip1668_id"
CONF_STB_PIN = "stb_pin"
CONF_DISPLAY_MODE = "display_mode"
CONF_HALF_PERIOD = "half_period"
CONF_REFRESH_INTERVAL = "refresh_interval"

aip1668_ns = cg.esphome_ns.namespace("aip1668")
AIP1668Component = aip1668_ns.class_("AIP1668Component", cg.PollingComponent)

# Grids x Segmente, Wert = Display-Modus-Befehl
DISPLAY_MODES = {
    "4x13": 0,
    "5x12": 1,
    "6x11": 2,
    "7x10": 3,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AIP1668Component),
        cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_DIO_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_STB_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_DISPLAY_MODE, default="6x11"): cv.enum(DISPLAY_MODES, lower=True),
        cv.Optional(CONF_BRIGHTNESS, default=3): cv.int_range(min=0, max=7),
        cv.Optional(CONF_HALF_PERIOD, default="20us"): cv.All(
            cv.positive_time_period_microseconds,
            cv.Range(
                min=cv.TimePeriod(microseconds=1), max=cv.TimePeriod(microseconds=100)
            ),
        ),
        cv.Optional(CONF_REFRESH_INTERVAL, default="1s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.polling_component_schema("20ms"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    clk = await cg.gpio_pin_expression(config[CONF_CLK_PIN])
    cg.add(var.set_clk_pin(clk))
    dio = await cg.gpio_pin_expression(config[CONF_DIO_PIN])
    cg.add(var.set_dio_pin(dio))
    stb = await cg.gpio_pin_expression(config[CONF_STB_PIN])
    cg.add(var.set_stb_pin(stb))

    cg.add(var.set_display_mode(config[CONF_DISPLAY_MODE]))
    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))
    cg.add(var.set_half_period(config[CONF_HALF_PERIOD].total_microseconds))
    cg.add(var.set_refresh_interval(config[CONF_REFRESH_INTERVAL].total_milliseconds))
