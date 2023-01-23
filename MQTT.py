import time
import numpy as np
import paho.mqtt.client as mqtt
from datetime import datetime
import cv2
import urllib.request


url = 'http://192.168.43.4'
portserver = ':90/'


def on_connect(client, userdata, flags, rc):
    print("in onconnect")
    if rc == 0:
        print("connected OK Returned code=", rc)
        client.connected_flag = True  # Flag to indicate success
    else:
        print("Bad connection Returned code=", rc)
        client.bad_connection_flag = True


def on_message(client, userdata, message):
    print("received message =", str(message.payload.decode("utf-8")))


def on_publish(client, userdata, result):  # create function for callback
    print("data published \n")
    pass


def run_mqtt(broker="45.149.77.235", port=1883):
    client = mqtt.Client("client_arman")
    client.on_connect = on_connect
    client.on_publish = on_publish
    client.on_message = on_message

    client.username_pw_set(username="97521036", password="0UmwhvIB")
    client.connect(broker, port)

    publish_data = 'data sent to mqtt'
    while True:
        client.publish("97521036/MamadArman", publish_data)
        time.sleep(1)


def save_image():
    img_resp = urllib.request.urlopen(url+'/capture')
    imgnp = np.asarray(bytearray(img_resp.read()), dtype=np.uint8)
    img = cv2.imdecode(imgnp, cv2.IMREAD_COLOR)
    img_name = datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + '.jpg'
    is_written = cv2.imwrite(img_name, img)
    if is_written:
        print('Image is successfully saved as', img_name)


def request_sensors():
    lines = str(urllib.request.urlopen(url + portserver +
                "mode").read().decode('utf-8')).split('\n')[:5]
    lines = [l.replace('\r', '') for l in lines]
    h = float(lines[0].replace('Humidity: ', '').replace('%', ''))
    t = float(lines[1].replace('Temprature: ', '').replace(' Celcius', ''))
    # hic = float(lines[2].replace('Heat index: ', '').replace(' Celcius', ''))
    is_sound = True
    if 'no' in lines[3]:
        is_sound = False
    is_motion = True
    if 'no' in lines[4]:
        is_motion = False
    return h, t, is_sound, is_motion, lines


def stream_camera():
    cv2.namedWindow("live transmission", cv2.WINDOW_AUTOSIZE)
    while True:
        img_resp = urllib.request.urlopen(url+'/capture')
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
        im = cv2.imdecode(imgnp, -1)
        cv2.imshow('live transmission', im)
        key = cv2.waitKey(5)
        if key == ord('q'):
            break
        time.sleep(0.1)

    cv2.destroyAllWindows()


if __name__ == '__main__':
    # run_mqtt()
    h, t, is_sound, is_motion, response = request_sensors()
    while True:
        t_before, h_before = t, h
        h, t, is_sound, is_motion, response = request_sensors()
        reasons = ''
        if t - t_before > 1:
            reasons += 'Temprature, '
        if h - h_before > 2:
            reasons += 'Humidity, '
        if is_sound:
            reasons += 'Sound, '
        if is_motion:
            reasons += 'motion, '
        if len(reasons) > 0:
            print('\n', 'you can check your baby due to high',
                  reasons, 'his status is:')
            print(response)
            n = int(input('enter 0 to capture a photo or 1 to watch him/her live:'))
            if n == 0:
                save_image()
            elif n == 1:
                stream_camera()
        time.sleep(10)
