/*
LodePNG Examples

Copyright (c) 2005-2010 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

// Modified by Raine "Gravecat" Simmons, 2016, 2017, 2018 & 2019.

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include "guru.h"
#include "iocore.h"
#include "lodepng/lodepng.h"
#include "lodepng/bmp2png.h"

#include <iostream>
#include <fstream>

//returns 0 if all went ok, non-0 if error
//output image is always given in RGBA (with alpha channel), even if it's a BMP without alpha channel
unsigned decodeBMP(std::vector<unsigned char>& image, unsigned& w, unsigned& h, const std::vector<unsigned char>& bmp)
{
	STACK_TRACE();
	static const unsigned MINHEADER = 54; //minimum BMP header size
  
	if(bmp.size() < MINHEADER) return -1;
	if(bmp[0] != 'B' || bmp[1] != 'M') return 1; //It's not a BMP file if it doesn't start with marker 'BM'
	const unsigned pixeloffset = bmp[10] + 256 * bmp[11]; //where the pixel data starts
	//read width and height from BMP header
	w = bmp[18] + bmp[19] * 256;
	h = bmp[22] + bmp[23] * 256;
	//read number of channels from BMP header
	if(bmp[28] != 24 && bmp[28] != 32) return 2; //only 24-bit and 32-bit BMPs are supported.
	const unsigned numChannels = bmp[28] / 8;

	//The amount of scanline bytes is width of image times channels, with extra bytes added if needed
	//to make it a multiple of 4 bytes.
	unsigned scanlineBytes = w * numChannels;
	if(scanlineBytes % 4 != 0) scanlineBytes = (scanlineBytes / 4) * 4 + 4;

	const unsigned dataSize = scanlineBytes * h;
	if(bmp.size() < dataSize + pixeloffset) return 3; //BMP file too small to contain all pixels

	image.resize(w * h * 4);

	/*
	There are 3 differences between BMP and the raw image buffer for LodePNG:
	-it's upside down
	-it's in BGR instead of RGB format (or BRGA instead of RGBA)
	-each scanline has padding bytes to make it a multiple of 4 if needed
	The 2D for loop below does all these 3 conversions at once.
	*/
	for(unsigned y = 0; y < h; y++)
	{
		for(unsigned x = 0; x < w; x++)
		{
			//pixel start byte position in the BMP
			const unsigned bmpos = pixeloffset + (h - y - 1) * scanlineBytes + numChannels * x;
			//pixel start byte position in the new raw image
			const unsigned newpos = 4 * y * w + 4 * x;
			if(numChannels == 3)
			{
				image[newpos + 0] = bmp[bmpos + 2]; //R
				image[newpos + 1] = bmp[bmpos + 1]; //G
				image[newpos + 2] = bmp[bmpos + 0]; //B
				image[newpos + 3] = 255;            //A
			}
			else
			{
				image[newpos + 0] = bmp[bmpos + 3]; //R
				image[newpos + 1] = bmp[bmpos + 2]; //G
				image[newpos + 2] = bmp[bmpos + 1]; //B
				image[newpos + 3] = bmp[bmpos + 0]; //A
			}
		}
	}
	return 0;
}

int convert_png(string filename)
{
	STACK_TRACE();
	const string source = filename + ".tmp";
	const string dest = filename + ".png";

	std::vector<unsigned char> bmp;
	lodepng::load_file(bmp, source.c_str());
	std::vector<unsigned char> image;
	unsigned w, h;
	unsigned error = decodeBMP(image, w, h, bmp);

	if (error)
	{
		guru->log("Could not decode BMP file!", GURU_ERROR);
		return 0;
	}

	std::vector<unsigned char> png;
	png.clear();

	lodepng::State state;
	state.encoder.filter_palette_zero = 0;	// We try several filter types, including zero, allow trying them all on palette images too.
	state.encoder.add_id = false;	// Don't add LodePNG version chunk to save more bytes
	state.encoder.text_compression = 1;	// Not needed because we don't add text chunks, but this demonstrates another optimization setting
	state.encoder.zlibsettings.nicematch = 258;	// Set this to the max possible, otherwise it can hurt compression
	state.encoder.zlibsettings.lazymatching = 1;	// Definitely use lazy matching for better compression
	state.encoder.zlibsettings.windowsize = 32768;	// Use maximum possible window size for best compression
	state.encoder.filter_strategy = LFS_ZERO;	// Don't fuck with these settings, they're the best options for what the game outputs.
	state.encoder.zlibsettings.minmatch = 3;
	state.encoder.zlibsettings.btype = 2;
	state.encoder.auto_convert = 0;
	std::vector<unsigned char> temp;
	error = lodepng::encode(temp, image, w, h, state);
	if (error)
	{
		guru->log("Could not encode PNG file!", GURU_ERROR);
		return 0;
	}
	temp.swap(png);
	lodepng::save_file(png, dest.c_str());
	error = remove(source.c_str());
	if (error) guru->log("Could not delete file: " + source, GURU_WARN);
	return 0;
}
