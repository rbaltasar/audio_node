
import sys
from time import sleep
import paho.mqtt.client as paho

client= paho.Client("AudioNode")
print("connecting to broker ")
client.connect("localhost",1883)#connect
client.loop_start() #start loop to process received messages

if __name__== "__main__":


    if(len(sys.argv) > 1):
        sleep(1)
        topic = str(sys.argv[1])
        print("Publishing ", topic)
        print(client.publish(topic,"1"))

    


    
