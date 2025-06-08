This IoT project utilizes the ESP32 microcontroller alongside the [ESP-IDF](https://github.com/espressif/esp-idf) framework.

The ESP32 receives temperature data from multiple Microcontrollers internal tempeature sensors and reports this information to the corresponding topic on the MQTT broker. Various services will monitor the temperature data and take action based on their specific functions, such as activating fans or sending notifications, among other tasks.

This project mimics the functionality of the [ROS framework](https://www.ros.org/) for learning purposes.