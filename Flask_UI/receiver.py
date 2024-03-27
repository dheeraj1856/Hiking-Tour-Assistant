import time
from bluepy import btle
import sys

fname = "data.csv"

def init_file():
    f = open(fname, 'w')
    f.write("steps,calories,time,distance\n")
    f.close()

def write_session(steps, cal, htime, dist):
    f = open(fname, 'a+')
    f.write(str(steps) + "," + str(cal) + "," + str(htime) + "," + str(dist) + "\n")
    f.close()

class MyDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        data_list = data.decode().split(';')
        print(data_list)
        write_session(int(data_list[0]), float(data_list[1]), float(data_list[2]), float(data_list[3]))


p = btle.Peripheral("44:17:93:88:d3:52")
p.setDelegate(MyDelegate())
svc = p.getServiceByUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
ch_Tx = svc.getCharacteristics("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")[0]
ch_Rx = svc.getCharacteristics("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")[0]

setup_data = b"\x01\00"
ch = p.writeCharacteristic(ch_Rx.valHandle+1, setup_data)

init_file()
while True:
    if p.waitForNotifications(3.0):
        continue
    print("Waiting...")