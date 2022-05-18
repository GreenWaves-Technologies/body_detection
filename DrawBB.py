import matplotlib.pyplot as plt
import matplotlib.patches as patches
from PIL import Image
import numpy as np

im = np.array(Image.open('test_samples/coco_55.pgm'), dtype=np.uint8)

# Create figure and axes
fig,ax = plt.subplots(1)

# Display the image
ax.imshow(im)

# Copy Paste code here:

rect = patches.Rectangle((22,19),45,83,linewidth=1,edgecolor='r',facecolor='none')
ax.add_patch(rect)
rect = patches.Rectangle((92,21),56,94,linewidth=1,edgecolor='r',facecolor='none')
ax.add_patch(rect)

################################

# Add the patch to the Axes
ax.add_patch(rect)

plt.show()
