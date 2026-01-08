import esphome.codegen as cg
import esphome.config_validation as cv

from esphome import pins
from esphome.const import CONF_ID

MULTI_CONF = True

CONF_DATAPIN = "data_pin"
CONF_SCANPIN = "scan_pin"
CONF_LEDPIN  = "led_pin"
CONF_SENSORTYPE  = "sensor_type"

rf_scanner_ns = cg.esphome_ns.namespace('rf_scanner')
RfScanner = rf_scanner_ns.class_("RfScanner", cg.Component)

SENSOR_TYPE = rf_scanner_ns.enum("EnumSensorType")
SENSOR_OPTIONS = {
    "DRY-TEST":  SENSOR_TYPE.DRY_TEST,
    "GT-WT-02":  SENSOR_TYPE.GT_WT_02,
    "DG-TH8898": SENSOR_TYPE.DG_TH8898,
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RfScanner),
        cv.Required(CONF_DATAPIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_SCANPIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_LEDPIN): pins.gpio_input_pin_schema,
        cv.Optional(CONF_SENSORTYPE, default="DG-TH8898"): cv.enum(SENSOR_OPTIONS, upper=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    pin = await cg.gpio_pin_expression(config[CONF_DATAPIN])
    cg.add(var.set_datapin(pin))

    cg.add(var.set_sensor_type(config[CONF_SENSORTYPE]))

    if CONF_SCANPIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_SCANPIN])
        cg.add(var.set_scanpin(pin))

    if CONF_LEDPIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_LEDPIN])
        cg.add(var.set_ledpin(pin))
