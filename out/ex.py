from pynq.overlays.base import BaseOverlay
from pynq.lib.video import *
base = BaseOverlay("base.bit")
import cv2
import numpy as np
Mode = VideoMode(640,480,24)
hdmi_out = base.video.hdmi_out
hdmi_out.configure(Mode,PIXEL_BGR)
hdmi_out.start()
while True:
    img = cv2.imread('test.png')
    frame = cv2.resize(img,(640,480))
    outframe = hdmi_out.newframe()
    outframe[0:480,0:640,:] = frame[0:480,0:640,:]
    hdmi_out.writeframe(outframe)

    
