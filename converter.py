from PIL import Image
def process_image(image, selfwidth=800, selfheight=480):
    # Crop the image to the specified width and height
    image = rotate_to_portrait(image)

    # image.thumbnail((selfwidth, selfheight))

    image = resize_and_crop(image, selfwidth, selfheight)

    # Rotate the image to portrait mode
    
    # Create a pallette with the 7 colors supported by the panel
    pal_image = Image.new("P", (1, 1))

    pal_image.putpalette((0, 0, 0,
	255, 255, 255,
	67, 138, 28,
	100, 64, 255,
	191, 0, 0,
	255, 243, 56,
	232, 126, 0,
	194 ,164 , 244))

    # pal_image.putpalette( (16,14,27,  169,164,155,  19,30,19,   21,15,50,  122,41,37,  156,127,56, 128,67,54) + (0,0,0)*249)
    # Convert the source image to the 7 colors, dithering if needed
    image_7color = image.convert("RGB").quantize(palette=pal_image)

    return image_7color

def resize_and_crop(image, width, height):
    # Calculate the aspect ratio of the target size
    aspect_ratio = width / height

    # Calculate the aspect ratio of the source image
    img_width, img_height = image.size
    img_aspect_ratio = img_width / img_height

    if img_aspect_ratio > aspect_ratio:
        # Crop the width of the image to match the target aspect ratio
        new_width = int(img_height * aspect_ratio)
        left = (img_width - new_width) / 2
        top = 0
        right = (img_width + new_width) / 2
        bottom = img_height
    else:
        # Crop the height of the image to match the target aspect ratio
        new_height = int(img_width / aspect_ratio)
        left = 0
        top = (img_height - new_height) / 2
        right = img_width
        bottom = (img_height + new_height) / 2

    # Crop and resize the image
    cropped_resized_image = image.crop((left, top, right, bottom)).resize((width, height), Image.ANTIALIAS)

    return cropped_resized_image

def rotate_to_portrait(image):
    # Check if the image is in portrait mode (height > width)

    if image.size[1] < image.size[0]:   # if height > width
        return image  # No need to rotate

    # Rotate the image 90 degrees clockwise
    rotated_image = image.transpose(Image.Transpose.ROTATE_90)
    # rotated_image.show()
    return rotated_image

def buffImg(image):
    image_temp = image
    buf_7color = bytearray(image_temp.tobytes('raw'))
    # PIL does not support 4 bit color, so pack the 4 bits of color
    # into a single byte to transfer to the panel
    buf = [0x00] * int(image_temp.width * image_temp.height / 2)
    idx = 0
    for i in range(0, len(buf_7color), 2):
        buf[idx] = (buf_7color[i] << 4) + buf_7color[i+1]
        idx += 1
    
    # Convert each byte in buf to a hexadecimal string
    hex_buf = [hex(byte) for byte in buf]
    
    with open('data.txt', 'w') as file:
        for hex_str in hex_buf:
            file.write(hex_str + ',')

    return hex_buf


def main():
    image = Image.open('lp.jpg')
    dithered_image = process_image(image)
    dithered_image.show()
    buffImg(dithered_image)
    


if __name__ == "__main__":
    main()