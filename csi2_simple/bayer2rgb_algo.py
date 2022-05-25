import cv2
import numpy as np
import matplotlib.pyplot as plt
import pdb

width = 640
height = 480

mul = 64

with open("./BUILD/GAP9_V2/GCC_RISCV_FREERTOS/Out.ppm", "rb") as rawimg:
    # Read the packed 16bits
    bayer_im0 = np.fromfile(rawimg, np.uint16, width * height)
    bayer_im = np.reshape(bayer_im0, (height, width))
    print (bayer_im.shape)

    out = np.zeros((height, width, 3), dtype="uint16")
    for i in range(height-1):
        for j in range(width-1):
            if (i % 2 == 0 and j % 2 ==0 ):
                out[i, j, 0] = bayer_im[i+1, j+1]
                out[i, j, 1] = (bayer_im[i+1, j] + bayer_im[i, j+1])/2
                out[i, j, 2] = bayer_im[i, j]
            elif (i % 2 == 0 and j % 2 == 1):
                out[i, j, 0] = bayer_im[i+1, j]
                out[i, j, 1] = (bayer_im[i, j] + bayer_im[i+1, j+1])/2
                out[i, j, 2] = bayer_im[i, j+1]
            elif (i % 2 == 1 and j % 2 == 0):
                out[i, j, 0] = bayer_im[i, j+1]
                out[i, j, 1] = (bayer_im[i, j] + bayer_im[i+1, j+1])/2
                out[i, j, 2] = bayer_im[i+1, j]
            else :
                out[i, j, 0] = bayer_im[i, j]
                out[i, j, 1] = (bayer_im[i, j+1] + bayer_im[i+1, j])/2
                out[i, j, 2] = bayer_im[i+1, j+1]


    # Show image for testing (multiply by 64 because imshow requires full uint16 range [0, 2^16-1]).
    cv2.imshow('bgr', out*mul)
    cv2.waitKey()
    cv2.destroyAllWindows()

    cv2.imwrite("./BG2RGB.png", out*mul)
