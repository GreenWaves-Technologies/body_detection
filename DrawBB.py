import matplotlib.pyplot as plt
import matplotlib.patches as patches
from PIL import Image
import numpy as np

im = np.array(Image.open('test_samples/img_OUT0.pgm'), dtype=np.uint8)

# Create figure and axes
fig,ax = plt.subplots(1)

# Display the image
ax.imshow(im)

# Copy Paste code here:
rect = patches.Rectangle((74,30),25,69,linewidth=1,edgecolor='r',facecolor='none')
ax.add_patch(rect)
rect = patches.Rectangle((90,113),22,6,linewidth=1,edgecolor='r',facecolor='none')
ax.add_patch(rect)
rect = patches.Rectangle((134,87),47,31,linewidth=1,edgecolor='r',facecolor='none')
ax.add_patch(rect)
rect = patches.Rectangle((0,89),45,30,linewidth=1,edgecolor='r',facecolor='none')
ax.add_patch(rect)





################################

# Add the patch to the Axes
ax.add_patch(rect)

plt.show()