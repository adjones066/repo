---
title: Steganography
author: Dannie M. Stanley
points_possible: 100
grading_type: points
---


# Introduction

The word steganography means "concealed writing." Steganography has been used for centuries to transmit secret messages. Using steganography, the secret message is hidden in some transportation medium such as an image, book, or music. The secret message can only be retrieved by a receiver that knows how to decode the message. For this project you will create a bitmap image file (BMP) encoder. The encoder will store the bits of a secret message in the least significant bits of each RGB color byte of a BMP file.

The resulting encoded image file will appear to be mostly unaltered. This is because the least significant bit will not always be changed and when it is, the change will be imperceptible. For example a color value made up of RED 255, GREEN 255, and BLUE 255 (white) may become the color made up of RED 254, GREEN 254, and BLUE 254 (mostly white). The difference between *white* and *mostly white* is visually imperceptible.


# Building
* Programs must compile using the GCC 5.x C compiler (gcc)
* Programs must use only the "C standard library" a.k.a "libc" and Linux system libraries (libraries that are included with less-than and greater-than symbols, e.g. `include <foo.h>`)
	- Students are not permitted to use any other libraries (such as those that parse BMP files)
* Program must be written in standard ANSI C (C11)
	- Warning: gcc, by default, will accept non-standard C code
	- Tip: standard C online documentation can be found at [http://en.cppreference.com/w/c](http://en.cppreference.com/w/c)


# Program Input

Your program must accept three command line arguments:

+ Argument 1: path to an input file. The input file has the following characteristics:
	- Uncompressed BMP file
	- 32 bit pixel size
+ Argument 2: path to an output file
	- File may or may not exist, if it does exist it will be overwritten
+ Argument 3: text string containing secret message


## Sample Image

There are a few variations of the BMP file format. The following image, sheldon.bmp should be used as a reference. It is encoded in a way that is sure to be compatible with the steganography method described in this assignment.

*Note: other images, in addition to sheldon.bmp, will be used during grading so your encoder should be general enough to work with all similarly formatted BMP files.*

- [sheldon.bmp](https://canvas.cse.taylor.edu/files/6677/download?download_frd=1)



## Example 1: STDIN/STDOUT

```bash
$ ./encoder sheldon.bmp enc.bmp "top secret message"
Pixels start at: 54
Width of image: 1024
Height of image: -640
```

# Program Output

Your program should produce output to standard output (STDOUT) as shown in example 1. You will need to reference a BMP file format specification document to find the pixel start, width, and height. WikiPedia has a seemingly reasonable description of the file format: [http://en.wikipedia.org/wiki/BMP_file_format](http://en.wikipedia.org/wiki/BMP_file_format)


## STDOUT

- Write to STDOUT the starting position (32 bit integer representing file offset) of the pixel array
- Write to STDOUT the width of the image
+ Write to STDOUT the height
	- Tip: a negative height means that the pixels are order from top to bottom


## File Output

* In addition to STDOUT, your program must create a new file that is stored in the file path provided as input to the program (enc.bmp in example 1)
* The new file is a copy of the original with a secret message encoded in it using the method described in the steganography section next.


# Steganography Method

Each pixel in the bitmap file is represented by 32 bits. One byte for each of red, green, blue, and the alpha channel. Students must not modify the alpha channel byte (the fourth byte on disk).

The order of the colors may change between BMP files. Knowing the precise order of the colors is not paramount. You can assume that the color bytes will be represented by the first three bytes, from lower-address to higher-address in the file, of each 32 bit pixel stored in the file.

It is safe to assume, for this project, that the image will have enough pixel to accommodate the secret message.

Your program must use the following method to encode the secret message:

* To encode the secret message into the new BMP file your program should reuse the least significant bit (LSB) of each color byte (red, green, blue) in the following way:
	* The LSB of the first color byte of the first pixel will represent the LSB of the first byte of the secret message
	* The LSB of the second color byte of the first pixel will represent the next LSB of the first byte of the secret message
	* The LSB of the third color byte of the first pixel will represent the next LSB of the first byte of the secret message
	* The LSB of the FIRST color of the second pixel will represent the next LSB of the first byte of the secret message
	* ...
* To indicate the end of the secret message, you must encode a single null character at the end of the secret message in your encoded image file. The decoder will stop decoding when the first null character is encountered.


# Testing

The program that you create is an encoder. To retrieve the secret message from an encoded BMP you must have a decoder. A decoder binary can be used from the following web address:

- <https://cse.taylor.edu/~dstanley/cos232/decoder.php>

You should be able to encode a BMP with a secret message and decode it using the provided decoder.


# Grading

This assignment uses a progressive grading scheme. Students are required to push incremental work to a designated repository.

**Commit and push** your work for this assignment to the following Git repository and subdirectory:

- *Git Repository*: `https://repo.cse.taylor.edu/work/{username}`
- *Subdirectory*: `course/cos232/project/steganography`

To get full credit, students must commit and push code that satisfies the following milestone requirements:

- Milestone 1:
	- Parses command line arguments
	- Opens original bitmap file for reading
	- Opens output file for writing
	- Stores secret message
- Milestone 2:
	- Reads from bitmap and writes to STDOUT the starting position of the pixel array
	- Reads from bitmap and writes to STDOUT the width and height of image
- Milestone 3:
	- Loop that iterates over all the pixels in the image
	- Loop that iterates over all the bytes in the secret message
- Milestone 4:
	- Completed project


# Tips

- Use a debugger. No seriously. Use it.
- Use a graphics program to inspect RGB values (using the eyedropper tool, usually). This can help you be sure that you are reading the correct value in your program.
	- Depending on the image, the first pixel may be in different corners of the image
	- Don't use the operating system eyedropper, it may sample a filtered color (use a graphics program like Gimp or Paint)
- You don't need to read all of the pixel values into a variable. In fact, this could cause a problem (the stack may not be big enough for the image)
- Use a pencil and paper. Take a sample image. Take a sample message. ENCODE BY HAND on the piece of paper the first 3 characters from your secret message. As a result, you will know what the ones places should be (odd or even) for each RGB value of the first 8 pixels. Use this for debugging your program:
	- Run your program
	- Sample the resulting image using a graphics application (using the eyedropper)
	- Does the ones places match what you did by hand?