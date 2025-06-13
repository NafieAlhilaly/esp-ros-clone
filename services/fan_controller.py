import paho.mqtt.client as mqtt
from json import loads, dumps
from log import logger

logger.name = __file__.split('/')[-1]

def init_fans():
    """Initialize the fans' power status"""
    with open('connected_devices.json', 'r') as file:
        devices = loads(file.read())
    
    device_fan_map = {}
    for device in devices:
        device_fan_map[device+"_fan"] = "off"  # Default state for fans

    with open('fan_power_status.json', 'w') as file:
        file.write(dumps(device_fan_map))

def activate_fan(device_id):
    """Activate the fan for a specific device."""
    with open('fan_power_status.json', 'r') as file:
        fans_status = loads(file.read())
    
    if device_id + "_fan" in fans_status:
        fans_status[device_id + "_fan"] = "on"
        with open('fan_power_status.json', 'w') as file:
            file.write(dumps(fans_status))
        logger.info(f"Fan for {device_id} activated.")
    else:
        logger.error(f"No fan found for device {device_id}.")


def get_connected_devices():
    """Retrieve a list of connected devices from the connected_devices.json file."""
    try:
        with open('connected_devices.json', 'r') as file:
            devices = loads(file.read())
            return devices
    except FileNotFoundError:
        logger.info("connected_devices.json not found. Returning an empty list.")
        return []


def on_connect(client, userdata, flags, reason_code, properties):
    logger.info(f"Connected with result code {reason_code}")
    client.subscribe("/hot_devices/+/temperature", qos=1, options=None)

def on_message(client, userdata, msg):
    device_id = msg.topic.split('/')[2]
    with open("fan_power_status.json", "r") as file:
        fan_status = loads(file.read())
    if fan_status.get(device_id + "_fan", "off") == "on":
        logger.info(f"Fan for {device_id} is already on. No action taken.")
        return
    client.publish(f"/fans/{device_id}/status", "on", qos=1)
    activate_fan(device_id)
    
    logger.info(f"Fan for {device_id} activated due to high temperature.")


mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_connect = on_connect
mqttc.on_message = on_message

logger.info("Connecting to MQTT broker...")
mqttc.connect("localhost", 1883, 60)
logger.info("Connected to MQTT broker.")

init_fans()
mqttc.loop_forever()