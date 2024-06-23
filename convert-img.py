from PIL import Image

def convert_256colors():
    with Image.open("../pic-blue.png") as im:
        im = im.convert(mode='RGB')
        im = im.resize((320, 200))
        # im = im.quantize(colors=256, dither=Image.Dither.NONE)
        im.save('pic2.bmp')
        im = im.quantize(colors=256, dither=Image.Dither.NONE)

        # bts = im.tobytes()
        # ba = bytearray()
        # for i in range(0, im.width * im.height):
        #     rgb = (int((bts[i*3] / 32)) << 5) + (int((bts[i*3 + 1] / 32)) << 2) + int((bts[i*3 + 2] / 64))
        #     ba.append(rgb & 0xFF)
        # with open('pic2.raw', 'wb') as f2:
        #     f2.write(ba)

        with open('pic2.raw', 'wb') as f2:
            f2.write(im.tobytes())

        # im2 = Image.new('P', (320, 200), color=None)
        # for x in range(0, 320):
        #     for y in range(0, 200):
        #         px = im.getpixel((x, y))
        #         rgb = (int((px[0] / 32)) << 5) + (int((px[1] / 32)) << 2) + int((px[2] / 64))
        #         im2.putpixel((x, y), (rgb & 0xFF))
        # im2.save('pic3.bmp')
            
def convert_rgb():
    with Image.open("../loading.jpg") as im:
        im = im.resize((640, 480))
        im.save('loading-resize.jpg')
        im = im.convert(mode='RGB')
        im.save('loading-rgb.jpg')

        # background = Image.new("RGB", im.size, (255, 255, 255))
        # background.paste(rgba_image, mask = rgba_image.split()[3])
        # background.save("sample_2.jpg", "JPEG", quality=100)

        with open('pic-loading.raw', 'wb') as f2:
            f2.write(im.tobytes())

if __name__ == '__main__':
    convert_rgb()
