
# The Gameboy Advance console uses colors in a BGR format, but all my favorite
# graphics tools use RGBA.

from PIL import Image
import os


if not os.path.exists('tmp'):
    os.makedirs('tmp')


image_dir = "images/"


def get_concat_h(im1, im2):
    dst = Image.new('RGBA', (im1.width + im2.width, im1.height))
    dst.paste(im1, (0, 0))
    dst.paste(im2, (im1.width, 0))
    return dst



# At least one tile in the image needs to contain some transparency, for the
# image to be displayed opaquely. So append this 8x8 image onto flattened
# images, so that they will appear entirely opaque.
transparency_key = Image.open(image_dir + 'transparency_reference.png')



def flatten_image(file_name):
    """
    Slice an image into 8-pixel tall strips, and join them end-to-end. Much
    easier than trying to meta-tile a 240x160 image!
    """
    im = Image.open(image_dir + file_name)

    strips = []

    w, h = im.size

    for i in range(0, int(h / 8)):
        strips.append(im.crop((0, int(i * 8), w, int(i * 8) + 8)))

    result = strips.pop(0)

    while strips:
        result = get_concat_h(result, strips.pop(0))

    result = get_concat_h(transparency_key, result)

    result.save(image_dir + file_name.split('.')[0] + '_flattened.png')



for name in ['old_poster.png']:
    flatten_image(name)



def rgb_to_bgr(file_name):
    """
    Convert rgb image file to bgr
    """
    im = Image.open(image_dir + file_name)

    r, g, b, a = im.split()
    im = Image.merge('RGBA', (b, g, r, a))

    im.save('tmp/' + file_name)


for name in ['spritesheet.png',
             'spritesheet_boss0.png',
             'spritesheet_boss1.png',
             'spritesheet2.png',
             'tilesheet.png',
             'tilesheet2.png',
             'overlay.png',
             'overlay_journal.png',
             'old_poster_flattened.png']:
    rgb_to_bgr(name)
