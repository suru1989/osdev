/* vim: tabstop=4 shiftwidth=4 noexpandtab
 *
 * Panel
 *
 * Provides a graphical panel with a clock, and
 * hopefully more things in the future.
 */
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <time.h>

#include "lib/utf8decode.h"

struct timeval {
	unsigned int tv_sec;
	unsigned int tv_usec;
};

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include "lib/window.h"
#include "lib/graphics.h"

sprite_t * sprites[128];
sprite_t alpha_tmp;

FT_Library   library;
FT_Face      face;
FT_Face      face_extra;
FT_GlyphSlot slot;
FT_UInt      glyph_index;


uint16_t win_width;
uint16_t win_height;
gfx_context_t * ctx;

int center_x(int x) {
	return (win_width - x) / 2;
}

int center_y(int y) {
	return (win_height - y) / 2;
}

void init_sprite(int i, char * filename, char * alpha) {
	sprites[i] = malloc(sizeof(sprite_t));
	load_sprite(sprites[i], filename);
	if (alpha) {
		sprites[i]->alpha = 1;
		load_sprite(&alpha_tmp, alpha);
		sprites[i]->masks = alpha_tmp.bitmap;
	} else {
		sprites[i]->alpha = ALPHA_OPAQUE;
	}
}

void draw_char(FT_Bitmap * bitmap, int x, int y, uint32_t fg) {
	int i, j, p, q;
	int x_max = x + bitmap->width;
	int y_max = y + bitmap->rows;
	for (j = y, q = 0; j < y_max; j++, q++) {
		for ( i = x, p = 0; i < x_max; i++, p++) {
			GFX(ctx, i,j) = alpha_blend(GFX(ctx, i,j),fg,rgb(bitmap->buffer[q * bitmap->width + p],0,0));
		}
	}
}

static void draw_string(int x, int y, uint32_t fg, char * string) {
	slot = face->glyph;
	int pen_x = x, pen_y = y, i = 0;
	int len = strlen(string);
	int error;

	for (i = 0; i < len; ++i) {
		FT_UInt glyph_index;

		glyph_index = FT_Get_Char_Index( face, string[i]);
		error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		if (error) {
			printf("Error loading glyph for '%c'\n", string[i]);
			continue;
		}
		slot = (face)->glyph;
		if (slot->format == FT_GLYPH_FORMAT_OUTLINE) {
			error = FT_Render_Glyph((face)->glyph, FT_RENDER_MODE_NORMAL);
			if (error) {
				printf("Error rendering glyph for '%c'\n", string[i]);
				continue;
			}
		}

		draw_char(&slot->bitmap, pen_x + slot->bitmap_left, pen_y - slot->bitmap_top, fg);
		pen_x += slot->advance.x >> 6;
		pen_y += slot->advance.y >> 6;
	}
}

int wstrlen(uint16_t * s) {
	int i = 0;
	while (s[i] != 0) {
		++i;
	}
	return i;
}

static void draw_string_wide(int x, int y, uint32_t fg, uint16_t * string) {
	slot = face->glyph;
	int pen_x = x, pen_y = y, i = 0;
	int len = wstrlen(string);
	int error;

	for (i = 0; i < len; ++i) {
		FT_UInt glyph_index;

		glyph_index = FT_Get_Char_Index( face, string[i]);

		if (glyph_index) {
			error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
			if (error) {
				printf("Error loading glyph for '%c'\n", string[i]);
				continue;
			}
			slot = (face)->glyph;
			if (slot->format == FT_GLYPH_FORMAT_OUTLINE) {
				error = FT_Render_Glyph((face)->glyph, FT_RENDER_MODE_NORMAL);
				if (error) {
					printf("Error rendering glyph for '%c'\n", string[i]);
					continue;
				}
			}
		} else {
			glyph_index = FT_Get_Char_Index( face_extra, string[i]);
			error = FT_Load_Glyph(face_extra, glyph_index, FT_LOAD_DEFAULT);
			if (error) {
				printf("Error loading glyph for '%c'\n", string[i]);
				continue;
			}
			slot = (face_extra)->glyph;
			if (slot->format == FT_GLYPH_FORMAT_OUTLINE) {
				error = FT_Render_Glyph((face_extra)->glyph, FT_RENDER_MODE_NORMAL);
				if (error) {
					printf("Error rendering glyph for '%c'\n", string[i]);
					continue;
				}
			}
		}

		draw_char(&slot->bitmap, pen_x + slot->bitmap_left, pen_y - slot->bitmap_top, fg);
		pen_x += slot->advance.x >> 6;
		pen_y += slot->advance.y >> 6;
	}
}

#define FONT_SIZE 14

void _loadDejavu() {
	char * font;
	size_t s = 0;
	int error;
	font = (char *)syscall_shm_obtain(WINS_SERVER_IDENTIFIER ".fonts.sans-serif", &s);
	error = FT_New_Memory_Face(library, font, s, 0, &face);
	error = FT_Set_Pixel_Sizes(face, FONT_SIZE, FONT_SIZE);
}

void _loadVlgothic() {
	int error;
	error = FT_New_Face(library, "/usr/share/fonts/VLGothic.ttf", 0, &face_extra);
	error = FT_Set_Pixel_Sizes(face_extra, FONT_SIZE, FONT_SIZE);
}

volatile int _continue = 1;

void sig_int(int sig) {
	printf("Received shutdown signal in panel!\n");
	_continue = 0;
}

int main (int argc, char ** argv) {
	setup_windowing();

	int width  = wins_globals->server_width;
	int height = wins_globals->server_height;

	win_width = width;
	win_height = height;


	FT_Init_FreeType(&library);
	_loadDejavu();
	_loadVlgothic();

	/* Create the panel */
	window_t * panel = window_create(0, 0, width, 24);
	window_reorder (panel, 0xFFFF);
	ctx = init_graphics_window_double_buffer(panel);
	draw_fill(ctx, rgb(0,0,0));
	flip(ctx);

	init_sprite(0, "/usr/share/panel.bmp", NULL);

	for (uint32_t i = 0; i < width; i += sprites[0]->width) {
		draw_sprite(ctx, sprites[0], i, 0);
	}

	size_t buf_size = panel->width * panel->height * sizeof(uint32_t);
	char * buf = malloc(buf_size);
	memcpy(buf, ctx->backbuffer, buf_size);

	flip(ctx);

	struct timeval now;
	int last = 0;
	struct tm * timeinfo;
	char   buffer[80];

	char _uname[1024];
	syscall_kernel_string_XXX(_uname);

	char * os_version = strstr(_uname, " ");
	os_version++;
	char * tmp = strstr(os_version, " ");
	tmp[0] = 0;

	/* UTF-8 Strings FTW! */
	uint8_t * os_name_ = "とあるOS";

	uint8_t final[512];

	uint32_t l = snprintf(final, 512, "%s %s", os_name_, os_version);

	uint8_t *s = final;
	uint16_t os_name[256] = {0};
	uint16_t *o = os_name;

	uint32_t codepoint;
	uint32_t state = 0;

	/* TODO: This should be part of the graphics library (combined with generic text rendering) */
	while (*s) {
		if (!decode(&state, &codepoint, *s)) {
			*o = (uint16_t)codepoint;
			o++;
		} else if (state == UTF8_REJECT) {
			state = 0;
		}
		s++;
	}
	o = 0;

	syscall_signal(2, sig_int);

	while (_continue) {
		/* Redraw the background by memcpy (super speedy) */
		memcpy(ctx->backbuffer, buf, buf_size);
		syscall_gettimeofday(&now, NULL); //time(NULL);
		if (now.tv_sec != last) {
			last = now.tv_sec;
			timeinfo = localtime((time_t *)&now.tv_sec);
			strftime(buffer, 80, "%I:%M:%S %p", timeinfo);

			draw_string(width - 100, 17, rgb(255,255,255), buffer);
			draw_string_wide(10, 17, rgb(255,255,255), os_name);

			flip(ctx);
			syscall_yield();
		}
	}

	teardown_windowing();

	return 0;
}
