#Credits to http://www.tobias-westmeier.de/astronomy_tutorial_startrails.php
import os, numpy
from PIL import Image

files   = os.listdir(os.getcwd())
images  = [name for name in files if name[-4:] in [".jpg", ".JPG"]]
width, height = Image.open(images[0]).size

stack   = numpy.zeros((height, width, 3), numpy.float)
counter = 1

for image in images:
    print ("Processing image " + str(counter))
    image_new = numpy.array(Image.open(image), dtype = numpy.float)
    stack     = numpy.maximum(stack, image_new)
    counter  += 1

stack = numpy.array(numpy.round(stack), dtype = numpy.uint8)

output = Image.fromarray(stack, mode = "RGB")
output.save("MiniTrail_stacked_image.jpg", "JPEG")
