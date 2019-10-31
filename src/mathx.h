// mathx.h -- Extended math utility functions, pseudo-random number generator, etc.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#pragma once
#include "duskfall.h"


namespace mathx
{

extern unsigned int	prand_seed;		// Pseudorandom number seed.

bool			check_flag(unsigned int flags, unsigned int flag_to_check);	// Checks to see if a flag is set.
float			grid_dist(long long x1, long long y1, long long x2, long long y2);	// Determines the difference between two points on a grid.
void			init();						// Sets up PCG pseudorandom number generator.
bool			is_odd(unsigned int num);	// Checks if a number is odd.
double			perlin(double x, double y, double zoom, double p, int octaves);	// Simple perlin noise generation.
double			perlin_findnoise2(double x, double y);	// Pseudo-random number generator for Perlin noise generation.
double			perlin_interpolate(double a, double b, double x);	// Cosine interpolation.
double			perlin_noise(double x, double y);	// Generate noise for a given coordinate.
unsigned char	perlin_rgb(double x, double y, double zoom, double p, int octaves);	// Wrapper to generate a 0-255 RGB value for the given coord.
unsigned int	prand(unsigned int lim);	// Simpler, easily-seedable pseudorandom number generator.
unsigned int	rnd(unsigned int max);		// Returns a random number between 1 and max.
float			round_to_two(float num);	// Rounds a float to two decimal places.

}	// namespace mathx
