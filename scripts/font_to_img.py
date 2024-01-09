from PIL import Image, ImageDraw, ImageFont

# Path to your font file
font_path = '../res/RobotoMono-Medium.ttf'

# Load the font with a specific size
font_size = 200
font = ImageFont.truetype(font_path, font_size)

font_width, font_height = font.font.getsize("0")
font_width = max(*font_width)
font_height = max(*font_height)
print("font_width: ", font_width)
print("font_height: ", font_height)
# Create an image canvas. Adjust the size as needed
image_size = (font_width * 10, font_width)
image = Image.new('RGBA', image_size, color=(0, 0, 0, 0))

# Create a drawing context
draw = ImageDraw.Draw(image)

# Position for the first number
start_position = 0

# Draw numbers from 0 to 9
for number in range(10):
    draw.text((start_position, -font_height), str(number),
              fill=(255, 255, 255, 255), font=font)
    # draw.rectangle((start_position, 0, start_position +
    #                 font_width, font_width), outline=(255, 255, 255, 255))
    start_position += font_width

# Save the image
image.save('../res/numbers_image.png')
