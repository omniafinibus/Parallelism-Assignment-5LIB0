
# Deadlines
1. four implementations of a real-time image processing pipeline
+ Wed 15 May - sequential
+ Mon 27 May - data parallel
+ Wed 5 June - function parallel
+ Wed 12 June - hybrid
2. submit report & code - Mon 17 June 23:55
3. submit peer review   - Fri 21 June 23:55


# Overal application
if the input image is a colour image then
    convert input image to greyscale & blur the image with an averaging convolution
else
    blur the image with a Gaussian convolution

then detect lines in the input image using the Sobel algorithm

then overlay the detected lines with the input image to give the output image

# Project requirements
+ at least one of the parallel versions must use frames as tokens (i.e. communication granularity is a frame)
+ at least one of the parallel versions must use pixels, lines, or blocks as tokens
    + same token for all inter-partition communication, including ARM → RISC-V
    + synchronise using FIFOs, shared buffers, mutexes, or anything you fancy
+ at least two of the parallel versions must use multiple partitions on the same RISC-V
+ at least one of the parallel versions must be a real-time implementation using the dataflow MOC
    + dataflow graph must use FIFOs and actors must have no state (use self-edges instead)
    + dataflow graph spans both ARM & RISC-Vs → you may assume that the ARM actors have a WCRT
    + OR
    + the dataflow graph is entirely on the RISC-Vs → compute the WCRT from the moment the ARM  that the input frame is ready until the RISC-V partition(s) signal that the output frame is ready

## sequential
+ split the ARM code in ARM & RISC-V parts
    + on the ARM: read the input bitmap, send the image (pixel array) to RISC-V, wait for the output image from the RISC-V, and write it to a file
    + on a single RISC-V core: wait for an incoming image, process it, and signal ARM when done
    + re-use the provided code & what you learned for the fractal!
+ ensure that the ARM partition can send multiple frames in succession to the RISC-V cores
    + without restarting the RISC-V partitions
    + use the runall.sh script in the ARM directory
    + like explained for the echo tutorial examples
    + we will evaluate all submissions using the runall.sh script

## data parallel
+ create ARM + RISC-V data-parallel image-processing application
    + on the ARM: (almost?) unchanged
    + split the image into parts that are processed by different partitions

## function parallel
+ create ARM + RISC-V function-parallel image-processing application
    + on the ARM: (almost?) unchanged
    + map the image-processing functions on different partitions

## hybrid parallel
+ create ARM + RISC-V hybrid-parallel image-processing application
    + on the ARM: (almost?) unchanged
    + a combination of data and function parallel