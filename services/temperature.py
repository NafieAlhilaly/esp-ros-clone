import paho.mqtt.client as mqtt
from os import environ
from storage import store, DeviceTemperature
from logging import getLogger, basicConfig, DEBUG

logger = getLogger(__name__)
basicConfig(level=DEBUG,format="{asctime} - {name} - {levelname} - {message}", style="{")

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

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message

logger.info("Connecting to MQTT broker...")
mqttc.connect("localhost", 1883, 60)
logger.info("Connected to MQTT broker.")

mqttc.loop_forever()