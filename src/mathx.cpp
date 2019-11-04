// mathx.cpp -- Extended math utility functions, pseudo-random number generator, etc.
// Copyright (c) 2016-2019 Raine "Gravecat" Simmons. Licensed under the GNU General Public License v3.

#include "guru.h"
#include "mathx.h"

#include "pcg/pcg_random.hpp"

#include <chrono>
#include <cmath>
#include <random>


namespace mathx
{

std::chrono::time_point<std::chrono::system_clock> dev_timer;	// Timer used for testing.
pcg32			*pcg = nullptr;	// PCG random number generator.
unsigned int	prand_seed = 0;		// Pseudorandom number seed.

// Checks to see if a flag is set.
bool check_flag(unsigned int flags, unsigned int flag_to_check)
{
	return ((flags & flag_to_check) == flag_to_check);
}

// Starts a timer for debugging/testing purposes.
void dev_timer_start()
{
	dev_timer = std::chrono::system_clock::now();
}

// Stops the timer and reports the result.
float dev_timer_stop()
{
	std::chrono::duration<float> elapsed_seconds = std::chrono::system_clock::now() - dev_timer;
	return elapsed_seconds.count();
}

// Determines the difference between two points on a grid.
float grid_dist(long long x1, long long y1, long long x2, long long y2)
{
	STACK_TRACE();
	const long long dist_x = x2 - x1, dist_y = y2 - y1;
	const float dist = (sqrt((dist_x * dist_x) + (dist_y * dist_y)));
	return round_to_two(dist);
}

// Sets up PCG pseudorandom number generator.
void init()
{
	STACK_TRACE();
	pcg = new pcg32(pcg_extras::seed_seq_from<std::random_device>{});
	guru::log("Pseudorandom number generator initialized.");
}

// Checks if a number is odd.
bool is_odd(unsigned int num)
{
	return (num % 2);
}

// Simple perlin noise generation.
double perlin(double x, double y, double zoom, double p, int octaves)
{
	STACK_TRACE();
	double getnoise = 0.0;
	for (int a = 0; a < octaves - 1; a++)
	{
		const double frequency = pow(2, a);
		const double amplitude = pow(p, a);
		getnoise += perlin_noise(x * frequency / zoom, y / zoom * frequency) * amplitude;
	}
	return getnoise;
}

// Pseudo-random number generator for Perlin noise generation.
double perlin_findnoise2(double x, double y)
{
	STACK_TRACE();
	long long n = static_cast<long long>(x) + static_cast<long long>(y) * 57;
	n = (n << 13) ^ n;
	const long long nn = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
	return 1.0 - (static_cast<double>(nn) / 1073741824.0f);
}

// Cosine interpolation.
double perlin_interpolate(double a, double b, double x)
{
	STACK_TRACE();
	const double ft = x * M_PI;
	const double f = (1.0 - cos(ft)) * 0.5;
	return a * (1.0 - f) + b * f;
}

// Generate noise for a given coordinate.
double perlin_noise(double x, double y)
{
	STACK_TRACE();
	const double floorx = static_cast<double>(static_cast<long long>(x));
	const double floory = static_cast<double>(static_cast<long long>(y));
	double s, t, u, v;   // Integer declaration
	s = perlin_findnoise2(floorx, floory);
	t = perlin_findnoise2(floorx + 1, floory);
	u = perlin_findnoise2(floorx, floory + 1);
	v = perlin_findnoise2(floorx + 1, floory + 1);
	const double int1 = perlin_interpolate(s, t, x - floorx);
	const double int2 = perlin_interpolate(u, v, x - floorx);
	return perlin_interpolate(int1, int2, y - floory);
}

// Wrapper to generate a 0-255 RGB value for the given coord. I wrote some of this, butchered some. It's flakey and weird, don't fuck with it.
unsigned char perlin_rgb(double x, double y, double zoom, double p, int octaves)
{
	STACK_TRACE();
	const double getnoise = perlin(x, y, zoom, p, octaves);
	int color = static_cast<int>((getnoise * 128.0f) + 128.0f);  // Convert to 0-256 values.
	if (color > 255) color = 255;
	if (color < 0) color = 0;
	return static_cast<unsigned char>(color);
}

// Simpler, easily-seedable pseudorandom number generator.
unsigned int prand(unsigned int lim)
{
	STACK_TRACE();
	if (lim <= 1) return 1;
	prand_seed = (prand_seed * 0x43FD43FD + 0xC39EC3) & 0xFFFFFF;
	const float tmp = static_cast<float>(prand_seed) / 16777216.0f;
	return static_cast<unsigned int>((tmp * lim) + 1);
}

// Returns a random number between 1 and max.
unsigned int rnd(unsigned int max)
{
	STACK_TRACE();
	if (max <= 1) return max;
	std::uniform_int_distribution<unsigned int> uniform_dist(1, max);
	return uniform_dist(*pcg);
}

// Rounds a float to two decimal places.
float round_to_two(float num)
{
	STACK_TRACE();
	return floorf(num * 100 + 0.5) / 100;
}

}	// namespace mathx
