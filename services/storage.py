import csv
from datetime import datetime

class DeviceTemperature:
    def __init__(self, device_id, temperature, timestamp=None):
        self.device_id = device_id
        self.temperature = temperature
        self.timestamp = timestamp or datetime.now().isoformat()+"Z"
    def to_dict(self):
        return {
            'device_id': self.device_id,
            'temperature': self.temperature,
            'timestamp': self.timestamp,
        }

def store(device_temperature: DeviceTemperature):
    file_path = 'device_temperatures.csv'
    with open(file_path, mode='a', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=device_temperature.to_dict().keys())
        if file.tell() == 0:
            writer.writeheader()
        writer.writerow(device_temperature.to_dict())