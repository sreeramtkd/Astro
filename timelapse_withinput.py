#Timelapse using Raspberry PI Camera & user input to controll ISO, ShutterSpeed & Brightness
import os,sys
import datetime
import time
from picamera import PiCamera
camera = PiCamera()
camera.start_preview(fullscreen=False, window = (50, 50, 640, 480))
datetime_object = datetime.datetime.fromtimestamp(time.time())
formatted_datetime = datetime_object.strftime("%Y-%m-%d_%H-%M")
dir_l="/home/pi/Pictures/timelapse/%s" % formatted_datetime
try:
    os.mkdir(dir_l)
except:
    pass

ISO_ = sys.argv[1]
SS = sys.argv[2]
brigh = sys.argv[3]

camera.resolution=(2048,1080)
camera.ISO=int(ISO_)
camera.shutter_speed=int(SS)
camera.brightness=int(brigh)

time.sleep(2)
for filename in camera.capture_continuous(dir_l+'/img{counter:03d}.jpg'):
    print('Captured %s' % filename)
    time.sleep(10) # wait 5 minutes
