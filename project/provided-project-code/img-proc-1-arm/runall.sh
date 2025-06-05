#!/bin/bash
# run the images in order of increasing size
# you can omit the larger ones in your initial testing
time ./arm_img_proc \
	test-images/pixel-1x1x24.bmp \
	test-images/surfer-32x24x24.bmp \
	test-images/lena-130x68x08.bmp \
	test-images/triangle-100x100x08.bmp \
	test-images/lines-120x210x08.bmp \
	test-images/marbles-130x92x08.bmp \
	test-images/noise-128x96x08.bmp \
	test-images/dots-110x110x24.bmp \
	test-images/ray-140x105x08.bmp \
	test-images/flag-124x124x08.bmp \
	test-images/headset-128x128x08.bmp \
	test-images/bitmap-120x156x08.bmp \
	test-images/mary-100x213x08.bmp \
	test-images/mary-101x215x08.bmp \
	test-images/mary-102x217x08.bmp \
	test-images/mary-103x219x08.bmp \
	test-images/zebra-195x147x08.bmp \
	test-images/triangle-100x100x24.bmp \
	test-images/marbles-130x92x24.bmp \
	test-images/bitmap-100x130x24.bmp \
	test-images/dots-196x196x08.bmp \
	test-images/dots-197x197x08.bmp \
	test-images/flag-124x124x24.bmp \
	test-images/bitmap-120x156x24.bmp \
	test-images/ray-320x240x08.bmp \
	test-images/ray-188x141x24.bmp \
	test-images/cat-160x185x24.bmp \
	test-images/lena-240x126x24.bmp \
	test-images/lena-415x218x08.bmp \
	test-images/triangle-200x200x24.bmp \
	test-images/triangle-200x200x24.bmp \
	test-images/triangle-200x200x24.bmp \
	test-images/alps-300x225x8.bmp \
	test-images/cat-400x462x24.bmp
