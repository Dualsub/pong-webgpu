import argparse
import cv2 as cv
import struct


def main():
    parser = argparse.ArgumentParser(description='Convert image to dat file')
    parser.add_argument('--image', type=str, help='image path')
    parser.add_argument('--output', type=str, help='output dat path')

    args = parser.parse_args()

    image_path = args.image
    output_path = args.output

    print('Loading image...')
    image = cv.imread(image_path, cv.IMREAD_UNCHANGED)
    # Add alpha channel if image has only 3 channels
    if image.shape[2] == 3:
        image = cv.cvtColor(image, cv.COLOR_BGR2RGBA)

    print("image: ", image.shape, image.itemsize)

    print('Writing to file...')
    with open(output_path, 'wb') as f:
        # Write image width and height, number of channels and number of bytes per channel
        f.write(struct.pack(
            'IIII', image.shape[1], image.shape[0], image.shape[2], image.itemsize))

        # Write image data
        f.write(image.tobytes())

    print('Done')


if __name__ == '__main__':
    main()
