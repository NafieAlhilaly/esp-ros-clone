import paho.mqtt.client as mqtt
from os import environ
from storage import store, DeviceTemperature
from log import logger
from json import loads, dumps

logger.name = __file__.split('/')[-1]

TEMP_THRESHOLD = int(environ.get("TEMP_THRESHOLD", 45))  # Temperature threshold for fan activation

def on_connect(client, userdata, flags, reason_code, properties):
    logger.info(f"Connected with result code {reason_code}")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("/devices/+/temperature", qos=1, options=None)

def on_message(client, userdata, msg):
    device_id = msg.topic.split('/')[2]
    temperature = float(msg.payload.decode())
    store(DeviceTemperature(device_id=device_id, temperature=temperature))
    if temperature > TEMP_THRESHOLD:
        client.publish(f"/hot_devices/{device_id}/temperature", temperature, qos=1)
    with open("fan_power_status.json", "r") as file:
        fans_status = loads(file.read())
    if device_id + "_fan" in fans_status:
        if fans_status[device_id + "_fan"] == "on":
            fans_status[device_id + "_fan"] = "off"
            with open("fan_power_status.json", "w") as file:
                file.write(dumps(fans_status))

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message

logger.info("Connecting to MQTT broker...")
mqttc.connect("localhost", 1883, 60)
logger.info("Connected to MQTT broker.")

mqttc.loop_forever()