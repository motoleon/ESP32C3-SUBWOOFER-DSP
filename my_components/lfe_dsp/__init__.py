import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID, CONF_GAIN
from esphome.components import esp32

DEPENDENCIES = ["esp32"]

lfe_dsp_ns = cg.esphome_ns.namespace("lfe_dsp")
LFEDsp = lfe_dsp_ns.class_("LFEDsp", cg.Component)

CONF_ADC_PIN = "adc_pin"
CONF_BCLK_PIN = "bclk_pin"
CONF_LRCK_PIN = "lrck_pin"
CONF_DATA_OUT_PIN = "data_out_pin"
CONF_SAMPLE_RATE = "sample_rate"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LFEDsp),
        cv.Required(CONF_ADC_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_BCLK_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_LRCK_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_DATA_OUT_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_SAMPLE_RATE, default=24000): cv.int_range(min=8000, max=48000),
        cv.Optional(CONF_GAIN, default=1.0): cv.float_range(min=0.0, max=8.0),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    adc_pin = await cg.gpio_pin_expression(config[CONF_ADC_PIN])
    cg.add(var.set_adc_pin(adc_pin))
    bclk_pin = await cg.gpio_pin_expression(config[CONF_BCLK_PIN])
    cg.add(var.set_bclk_pin(bclk_pin))
    lrck_pin = await cg.gpio_pin_expression(config[CONF_LRCK_PIN])
    cg.add(var.set_lrck_pin(lrck_pin))
    data_out_pin = await cg.gpio_pin_expression(config[CONF_DATA_OUT_PIN])
    cg.add(var.set_data_out_pin(data_out_pin))
    cg.add(var.set_sample_rate(config[CONF_SAMPLE_RATE]))
    cg.add(var.set_gain(config[CONF_GAIN]))
