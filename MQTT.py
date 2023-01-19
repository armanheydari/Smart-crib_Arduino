import time
import paho.mqtt.client as mqtt


# MQTT
def on_connect(client, userdata, flags, rc):
    print("in onconnect")
    if rc == 0:
        print("connected OK Returned code=", rc)
        client.connected_flag = True  # Flag to indicate success
    else:
        print("Bad connection Returned code=", rc)
        client.bad_connection_flag = True


def on_publish(client, userdata, result):  # create function for callback
    print("data published \n")
    pass


port = 1883
broker = "45.149.77.235"
url = 'http://192.168.43.4'
portserver = '/:90/'


def run():
    client = mqtt.Client("python_arman")
    client.on_connect = on_connect
    client.on_publish = on_publish

    client.username_pw_set(username="97521036", password="0UmwhvIB")
    client.connect(broker, port)

    publish_data = 'data sent to mqtt'
    while True:
        client.publish("97521036/MamadArman", publish_data)
        time.sleep(1)


if __name__ == '__main__':
    # humidity = str(urllib.request.urlopen(url + portserver + "mode").read().decode('utf-8'))
    # print(humidity)
    run()
