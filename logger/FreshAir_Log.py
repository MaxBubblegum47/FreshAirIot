# python 3.11
import sys
import datetime
import paho.mqtt.client as mqtt

# ("home/room/window/height", 3), ("home/room/window/airquality", 4), ("home/room/window/light", 5), ("home/room/temperature", 6), ("home/room/pressure", 7), ("home/room/height", 8), ("home/hall/temperature", 9), ("home/hall/pressure", 10), ("home/hall/height", 11), ("home/actuators", 12), ("weather/", 13), ("extern/", 14)

# Define the list of topics to subscribe to
topics = [("home/room/window/temperature", 0), ("home/room/window/humidity", 0), ("home/room/window/pressure", 0), ("home/room/window/height", 0), 
          ("home/room/window/airquality", 0), ("home/room/window/light", 0), ("home/room/temperature", 0), ("home/room/pressure", 0), 
          ("home/room/height", 0), ("home/hall/temperature", 0), ("home/hall/pressure", 0), ("home/hall/height", 0), 
          ("home/actuators", 0), ("weather/", 0), ("extern/", 0), ("home/room/window/airquality_voltage", 0)]

# Define the MQTT broker address and port
broker_address = 'broker.emqx.io'
broker_port = 1883

# Define the on_connect callback
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    # Subscribe to all topics in the list
    for topic, qos in topics:
        client.subscribe(topic, qos)

# Define the on_message callback
def on_message(client, userdata, msg):
    
    file_name = "dump_" + str(datetime.date.today())
    with open(file_name, "a+") as sys.stdout:
        print(f"{msg.topic} = {msg.payload.decode()} --- Date: " + str(datetime.datetime.now()))


# Create a new MQTT client instance
client = mqtt.Client()

# Assign the callbacks
client.on_connect = on_connect
client.on_message = on_message

# Connect to the broker
client.connect(broker_address, broker_port, 60)

# Start the MQTT client loop
client.loop_forever()
