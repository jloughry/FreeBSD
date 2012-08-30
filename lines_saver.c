/*-
 * Copyright (c) 2000 Joe Loughry
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

/*
 * Joe Loughry <loughry@uswest.net>, 01 MAY 2000
 * 
 * like the "qix" mode of xlock, but does not require X.  Based on the
 * fire_saver screen saver, but with anti-aliased line drawing routines
 * based on an article by Michael Abrash, Dr. Dobbs Journal, June 1992.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/syslog.h>

#include <machine/md_var.h>
#include <machine/random.h>

#include <saver.h>

static void	draw_line(int, int, int, int, int);
static void	vertical(int, int, int, int);
static void	horizontal(int, int, int, int);
static void	diagonal(int, int, int, int, int);
static void	arbitrary_slope(int, int, int, int, int);
static void	set_pixel(int, int, int);
static int	random_speed(void);
static int	lines_saver(video_adapter_t *, int);
static void	lines_update(void);
static int	lines_init(video_adapter_t *);
static int	lines_terminate(video_adapter_t *);

#define VIDEO_MODE	M_VGA_CG320
#define X_SIZE	320
#define Y_SIZE	200

#define NUM_LINES	40	/* how many lines to draw */

static int blanked = 0;
static u_char pal[768];
static u_char buf[X_SIZE * (Y_SIZE + 1)];
static u_char * vid;
static int head = 0;

const int num_shades = 8;	/* must be a power of two for Wu lines */
const int num_bits = 3;	/* log(2) of num_shades */
const int average_speed = 6;

/*
 * The VGA palette is based on 24 colors, evenly mixed from {R, G, B},
 * with each color ramped in brightness (with a gamma correction of 2.5
 * applied for PC monitors) in 8 levels ranging from full brightness
 * (at offset n + 0) to black (at offset n + 7), where n is a number
 * from 1 to 24.  The first 8 slots in the palette (color 0) are filled
 * with all 0 values to make erasing anti-aliased lines easier.
 *
 * The Wu method for drawing anti-aliased lines requires the number of
 * brightness levels (grey levels) to be a power of 2.  Twenty-four
 * colors and 8 brightness levels fit pretty well in the 256 VGA palette
 * slots available.
 */

static u_char pal[768] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0xff, 0x00, 0x00,
	0xef, 0x00, 0x00,
	0xde, 0x00, 0x00,
	0xcb, 0x00, 0x00,
	0xb5, 0x00, 0x00,
	0x9a, 0x00, 0x00,
	0x75, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0xff, 0x91, 0x00,
	0xef, 0x89, 0x00,
	0xde, 0x7f, 0x00,
	0xcb, 0x74, 0x00,
	0xb5, 0x67, 0x00,
	0x9a, 0x58, 0x00,
	0x75, 0x42, 0x00,
	0x00, 0x00, 0x00,
	0xff, 0xc0, 0x00,
	0xef, 0xb5, 0x00,
	0xde, 0xa8, 0x00,
	0xcb, 0x9a, 0x00,
	0xb5, 0x89, 0x00,
	0x9a, 0x74, 0x00,
	0x75, 0x58, 0x00,
	0x00, 0x00, 0x00,
	0xff, 0xe3, 0x00,
	0xef, 0xd5, 0x00,
	0xde, 0xc6, 0x00,
	0xcb, 0xb5, 0x00,
	0xb5, 0xa1, 0x00,
	0x9a, 0x89, 0x00,
	0x75, 0x68, 0x00,
	0x00, 0x00, 0x00,
	0xff, 0xff, 0x00,
	0xef, 0xef, 0x00,
	0xde, 0xde, 0x00,
	0xcb, 0xcb, 0x00,
	0xb5, 0xb5, 0x00,
	0x9a, 0x9a, 0x00,
	0x75, 0x75, 0x00,
	0x00, 0x00, 0x00,
	0xe3, 0xff, 0x00,
	0xd5, 0xef, 0x00,
	0xc6, 0xde, 0x00,
	0xb5, 0xcb, 0x00,
	0xa1, 0xb5, 0x00,
	0x89, 0x9a, 0x00,
	0x68, 0x75, 0x00,
	0x00, 0x00, 0x00,
	0xc0, 0xff, 0x00,
	0xb5, 0xef, 0x00,
	0xa8, 0xde, 0x00,
	0x9a, 0xcb, 0x00,
	0x89, 0xb5, 0x00,
	0x74, 0x9a, 0x00,
	0x58, 0x75, 0x00,
	0x00, 0x00, 0x00,
	0x91, 0xff, 0x00,
	0x89, 0xef, 0x00,
	0x7f, 0xde, 0x00,
	0x74, 0xcb, 0x00,
	0x67, 0xb5, 0x00,
	0x58, 0x9a, 0x00,
	0x42, 0x75, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0xff, 0x00,
	0x00, 0xef, 0x00,
	0x00, 0xde, 0x00,
	0x00, 0xcb, 0x00,
	0x00, 0xb5, 0x00,
	0x00, 0x9a, 0x00,
	0x00, 0x75, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0xff, 0x91,
	0x00, 0xef, 0x89,
	0x00, 0xde, 0x7f,
	0x00, 0xcb, 0x74,
	0x00, 0xb5, 0x67,
	0x00, 0x9a, 0x58,
	0x00, 0x75, 0x42,
	0x00, 0x00, 0x00,
	0x00, 0xff, 0xc0,
	0x00, 0xef, 0xb5,
	0x00, 0xde, 0xa8,
	0x00, 0xcb, 0x9a,
	0x00, 0xb5, 0x89,
	0x00, 0x9a, 0x74,
	0x00, 0x75, 0x58,
	0x00, 0x00, 0x00,
	0x00, 0xff, 0xe3,
	0x00, 0xef, 0xd5,
	0x00, 0xde, 0xc6,
	0x00, 0xcb, 0xb5,
	0x00, 0xb5, 0xa1,
	0x00, 0x9a, 0x89,
	0x00, 0x75, 0x68,
	0x00, 0x00, 0x00,
	0x00, 0xff, 0xff,
	0x00, 0xef, 0xef,
	0x00, 0xde, 0xde,
	0x00, 0xcb, 0xcb,
	0x00, 0xb5, 0xb5,
	0x00, 0x9a, 0x9a,
	0x00, 0x75, 0x75,
	0x00, 0x00, 0x00,
	0x00, 0xe3, 0xff,
	0x00, 0xd5, 0xef,
	0x00, 0xc6, 0xde,
	0x00, 0xb5, 0xcb,
	0x00, 0xa1, 0xb5,
	0x00, 0x89, 0x9a,
	0x00, 0x68, 0x75,
	0x00, 0x00, 0x00,
	0x00, 0xc0, 0xff,
	0x00, 0xb5, 0xef,
	0x00, 0xa8, 0xde,
	0x00, 0x9a, 0xcb,
	0x00, 0x89, 0xb5,
	0x00, 0x74, 0x9a,
	0x00, 0x58, 0x75,
	0x00, 0x00, 0x00,
	0x00, 0x91, 0xff,
	0x00, 0x89, 0xef,
	0x00, 0x7f, 0xde,
	0x00, 0x74, 0xcb,
	0x00, 0x67, 0xb5,
	0x00, 0x58, 0x9a,
	0x00, 0x42, 0x75,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xff,
	0x00, 0x00, 0xef,
	0x00, 0x00, 0xde,
	0x00, 0x00, 0xcb,
	0x00, 0x00, 0xb5,
	0x00, 0x00, 0x9a,
	0x00, 0x00, 0x75,
	0x00, 0x00, 0x00,
	0x91, 0x00, 0xff,
	0x89, 0x00, 0xef,
	0x7f, 0x00, 0xde,
	0x74, 0x00, 0xcb,
	0x67, 0x00, 0xb5,
	0x58, 0x00, 0x9a,
	0x42, 0x00, 0x75,
	0x00, 0x00, 0x00,
	0xc0, 0x00, 0xff,
	0xb5, 0x00, 0xef,
	0xa8, 0x00, 0xde,
	0x9a, 0x00, 0xcb,
	0x89, 0x00, 0xb5,
	0x74, 0x00, 0x9a,
	0x58, 0x00, 0x75,
	0x00, 0x00, 0x00,
	0xe3, 0x00, 0xff,
	0xd5, 0x00, 0xef,
	0xc6, 0x00, 0xde,
	0xb5, 0x00, 0xcb,
	0xa1, 0x00, 0xb5,
	0x89, 0x00, 0x9a,
	0x68, 0x00, 0x75,
	0x00, 0x00, 0x00,
	0xff, 0x00, 0xff,
	0xef, 0x00, 0xef,
	0xde, 0x00, 0xde,
	0xcb, 0x00, 0xcb,
	0xb5, 0x00, 0xb5,
	0x9a, 0x00, 0x9a,
	0x75, 0x00, 0x75,
	0x00, 0x00, 0x00,
	0xff, 0x00, 0xe3,
	0xef, 0x00, 0xd5,
	0xde, 0x00, 0xc6,
	0xcb, 0x00, 0xb5,
	0xb5, 0x00, 0xa1,
	0x9a, 0x00, 0x89,
	0x75, 0x00, 0x68,
	0x00, 0x00, 0x00,
	0xff, 0x00, 0xc0,
	0xef, 0x00, 0xb5,
	0xde, 0x00, 0xa8,
	0xcb, 0x00, 0x9a,
	0xb5, 0x00, 0x89,
	0x9a, 0x00, 0x74,
	0x75, 0x00, 0x58,
	0x00, 0x00, 0x00,
	0xff, 0x00, 0x91,
	0xef, 0x00, 0x89,
	0xde, 0x00, 0x7f,
	0xcb, 0x00, 0x74,
	0xb5, 0x00, 0x67,
	0x9a, 0x00, 0x58,
	0x75, 0x00, 0x42,
	0x00, 0x00, 0x00
	/* the rest is zero-filled by the compiler */
};

struct point {
	int	x_position;
	int	y_position;
};

struct vector {
	int	x_velocity;
	int	y_velocity;
};

struct Line {
	struct point	beginning;
	struct point	end;
	int	color;
};

static struct Line	line[NUM_LINES];
static struct vector	beginning, end;
static int	current_color = 1;

/*
 * Set the pixel at location (x, y) in the buffer to color color.
 */
static void
set_pixel(int x, int y, int color)
{
	buf[x + (y * X_SIZE)] = color;
	return;
}

/*
 * Return a random value near the constant average_speed, but
 * biased a little high, so it's not too slow.
 */
static int
random_speed(void)
{
	return (random() % (average_speed / 2)) + average_speed / 2;
}

/*
 * The value of current color goes from 1 to 24, then recycles.
 */
static int
next_color(void)
{
	int c;

	c = current_color;
	current_color++;
	if (current_color > 24)
		current_color = 1;
	return (c);
}

/*
 * This function gets called many times every second.  The first time
 * it gets called, it sets up the initial conditions.  Thereafter, it
 * calls lines_update(), which draws another frame in the animation.
 */
static int
lines_saver(video_adapter_t * adp, int blank)
{
	if (blank) {
		if (blanked == 0) {
			set_video_mode(adp, VIDEO_MODE);
			load_palette(adp, pal);

			/* initialize */
			line[head].beginning.x_position = random() % X_SIZE;
			line[head].beginning.y_position = random() % Y_SIZE;
			beginning.x_velocity = random_speed();
			if (random() % 2)
				beginning.x_velocity = -beginning.x_velocity;
			beginning.y_velocity = random_speed();
			if (random() % 2)
				beginning.y_velocity = -beginning.y_velocity;
			line[head].end.x_position = random() % X_SIZE;
			line[head].end.y_position = random() % Y_SIZE;
			end.x_velocity = random_speed();
			if (random() % 2)
				end.x_velocity = -end.x_velocity;
			end.y_velocity = random_speed();
			if (random() % 2)
				end.y_velocity = -end.y_velocity;
			line[head].color = next_color();

			blanked = 1;
			vid = (u_char *) adp->va_window;

			bzero(buf, sizeof buf);	/* clear the buffer to black */
		}
		/* draw one frame of animation */
		lines_update();
	} else
		blanked = 0;
	return (0);
}

/*
 * Draws a line in the buffer from (x1, y1) to (x2, y2) with color c.
 */
static void
draw_line(int x1, int y1, int x2, int y2, int c)
{
	int dx, dy;

	dx = x2 - x1;
	dy = y2 - y1;

	if (dy < 0) {
		int	temp;

		temp = x2;
		x2 = x1;
		x1 = temp;

		temp = y2;
		y2 = y1;
		y1 = temp;
	}
	/*
	 * Vertical, horizontal, and 45-degree diagonal lines do not
	 * need anti-aliasing, so they are handled as special cases.
	 */
	if (dx == 0)
		vertical(x1, y1, y2, c);	/* not anti-aliased */
	else if (dy == 0)
		horizontal(x1, x2, y1, c);	/* not anti-aliased */
	else if (dx == dy)
		diagonal(x1, y1, x2, y2, c);	/* not anti-aliased */
	else
		arbitrary_slope(x1, y1, x2, y2, c);	/* anti-aliased */
	return;
}

/*
 * Draws a vertical line from (x, y1) to (x, y2) with color c.
 */
static void
vertical (int x, int y1, int y2, int c)
{
	int i, dy;

	dy = y2 - y1;
	if (dy < 0) {
		int	temp;

		temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (i = y1; i <= y2; i++)
		set_pixel(x, i, c);
	return;
}

/*
 * Draws a horizontal line from (x1, y) to (x2, y) with color c.
 */
static void
horizontal(int x1, int x2, int y, int c)
{
	int i, dx, temp;

	dx = x2 - x1;
	if (dx < 0) {
		temp = x2;
		x2 = x1;
		x1 = temp;
	}
	for (i = x1; i <= x2; i++)
		set_pixel(i, y, c);
	return;
}

/*
 * Draws a 45-degree diagonal line from (x1, y1) to (x2, y2) with color c.
 */
static void
diagonal(int x1, int y1, int x2, int y2, int c)
{
	int i, j, dx, dy, temp;

	dx = x2 - x1;
	dy = y2 - y1;

	if (dx < 0) {
		temp = x2;
		x2 = x1;
		x1 = temp;

		temp = y2;
		y2 = y1;
		y1 = temp;
	}
	for (i = x1, j = y1; i <= x2; i++, j++)
		set_pixel (i, j, c);
	return;
}

/*
 * Draws an anti-aliased line from (x1, y1) to (x2, y2) with color c.
 */
static void
arbitrary_slope(int x1, int y1, int x2, int y2, int c)
{
	int dx, dy, xinc;
	u_short error, errorinc, temp, brightness;

	dx = x2 - x1;
	dy = y2 - y1;

	xinc = 1;
	if (dx < 0) {
		xinc = -1;
		dx = -dx;
	}
	set_pixel(x1, y1, c);	/* line exactly intersects first pixel */

	error = 0;
	if (dx > dy) {	/* x-major line */
		errorinc = ((u_long)dy << sizeof(u_short) * 8) / (u_long)dx;
		while (--dx) {
			temp = error;
			error += errorinc;
			if (error <= temp)
				y1++;
			x1 += xinc;
			brightness = error >> 13;
			set_pixel(x1, y1, c + brightness);
			set_pixel(x1, y1 + 1, c + (brightness ^ 7));
		}
	} else {	/* y-major line */
		errorinc = ((u_long)dx << sizeof(u_short) * 8) / (u_long)dy;
		while (--dy) {
			temp = error;
			error += errorinc;
			if (error <= temp)
				x1 += xinc;
			y1++;
			brightness = error >> 13;
			set_pixel (x1, y1, c + brightness);
			set_pixel (x1 + xinc, y1, c + (brightness ^ 7));
		}
	}
	set_pixel(x2, y2, c);	/* line exactly intersects last pixel */
	return;
}

/*
 * When a moving point hits a wall between time steps, this formula
 * calculates the correct position after the collision at the next
 * time step in the animation.
 */
static int
overshoot(int larger, int smaller)
{
	int overshoot;

	overshoot = larger - smaller;
	return (smaller - 1 - overshoot);
}

/*
 * Computes the next frame in the animation.
 */
static void
lines_update(void)
{
	int i, next;

	next = head + 1;
	if (next >= NUM_LINES)
		next = 0;

	/* erase the oldest line */
	draw_line(line[next].beginning.x_position,
		  line[next].beginning.y_position,
		  line[next].end.x_position,
		  line[next].end.y_position, 0);

	/* calculate the position of the newest line */
	line[next].beginning.x_position = line[head].beginning.x_position
	    + beginning.x_velocity;
	line[next].beginning.y_position = line[head].beginning.y_position
	    + beginning.y_velocity;
	line[next].end.x_position = line[head].end.x_position + end.x_velocity;
	line[next].end.y_position = line[head].end.y_position + end.y_velocity;
	line[next].color = next_color();

	/*
	 * Check for collision with any of the walls; reverse direction
	 * and change speed when we collide with a wall.
	 */
	if (line[next].beginning.x_position < 0) {
		line[next].beginning.x_position
		    = -line[next].beginning.x_position;
		beginning.x_velocity = random_speed();
	}
	if (line[next].beginning.x_position >= X_SIZE) {
		line[next].beginning.x_position
		    = overshoot(line[next].beginning.x_position, X_SIZE);
		beginning.x_velocity = -1 * random_speed();
	}
	if (line[next].beginning.y_position < 0) {
		line[next].beginning.y_position
		    = -line[next].beginning.y_position;
		beginning.y_velocity = random_speed();
	}
	if (line[next].beginning.y_position >= Y_SIZE) {
		line[next].beginning.y_position
		    = overshoot(line[next].beginning.y_position, Y_SIZE);
		beginning.y_velocity = -1 * random_speed();
	}
	if (line[next].end.x_position < 0) {
		line[next].end.x_position = -line[next].end.x_position;
		end.x_velocity = random_speed();
	}
	if (line[next].end.x_position >= X_SIZE) {
		line[next].end.x_position
		    = overshoot(line[next].end.x_position, X_SIZE);
		end.x_velocity = -1 * random_speed();
	}
	if (line[next].end.y_position < 0) {
		line[next].end.y_position = -line[next].end.y_position;
		end.y_velocity = random_speed();
	}
	if (line[next].end.y_position >= Y_SIZE) {
		line[next].end.y_position
		    = overshoot(line[next].end.y_position, Y_SIZE);
		end.y_velocity = -1 * random_speed();
	}
	/* draw all lines, in order from oldest to newest */
	for (i = next + 1; i < NUM_LINES; i++)
		draw_line(line[i].beginning.x_position,
			  line[i].beginning.y_position,
			  line[i].end.x_position,
			  line[i].end.y_position,
			  line[i].color * num_shades);
	for (i = 0; i <= next; i++)
		draw_line(line[i].beginning.x_position,
			  line[i].beginning.y_position,
			  line[i].end.x_position,
			  line[i].end.y_position,
			  line[i].color * num_shades);
	head++;
	if (head >= NUM_LINES)
		head = 0;

	/* blit the buffer into video ram */
	memcpy(vid, buf, X_SIZE * Y_SIZE);

	return;
}

static int
lines_init(video_adapter_t * adp)
{
	video_info_t info;

	/* check that the console is capable of running in 320x200x256 */
	if (get_mode_info(adp, VIDEO_MODE, &info)) {
		log(LOG_NOTICE, "lines_saver: no suitable video mode\n");
		return (ENODEV);
	}
	blanked = 0;
	return (0);
}

static int
lines_terminate(video_adapter_t * adp)
{
	return (0);
}

static scrn_saver_t lines_module = {
	"lines_saver", lines_init, lines_terminate, lines_saver, NULL
};

SAVER_MODULE(lines_saver, lines_module);
