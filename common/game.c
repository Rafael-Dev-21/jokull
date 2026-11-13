#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "game.h"
#include "vec.h"
#include "skfantasy.h"

#define VERSION "0.1.0"

#define FPS 60

#define DISPLAY_WIDTH  800
#define DISPLAY_HEIGHT 600

typedef struct {
  uint8_t b, g, r, a;
} Color;

static Color display[DISPLAY_WIDTH*DISPLAY_HEIGHT];

#define AUDIO_SAMPLE_RATE 44100
static_assert((AUDIO_SAMPLE_RATE%FPS)==0, "Sample rate should divide evenly by frame rate.");
#define AUDIO_CHANNELS 2
#define AUDIO_FRAMES_PER_TICK (AUDIO_SAMPLE_RATE/FPS)
#define AUDIO_CAPACITY (AUDIO_FRAMES_PER_TICK*AUDIO_CHANNELS)

#define ARRAY_LEN(x) (sizeof((x))/sizeof((x)[0]))

static int16_t audio[AUDIO_CAPACITY];

uint8_t abuf[MB(32)] = {0};

static Arena arena;
static ModeData data;
static int mode;
static struct {
  uint8_t d[8];
} blocks[] = {
    {{0}},
    {{0, 0, 0x10, 0x10, 0x20, 0x02, 0x04, 0}},
    {{0, 0x86, 0x7E, 0x31, 0, 0x86, 0x7E, 0x31}},
    {{0xCC, 0x44, 0x66, 0x44, 0xCC, 0x44, 0x66, 0x44}},
    {{0, 0, 0, 0x13, 0x25, 0, 0, 0}},
    {{0x30, 0x30, 0xFF, 0xff, 0x03, 0x03, 0xFF, 0xFF}},
    {{0, 0x38, 0x44, 0x54, 0x44, 0x38, 0, 0}},
    {{0, 0, 0xaa, 0x55, 0xaa, 0xaa, 0xaa, 0}},
    {{0xff, 0xff, 0xC3, 0xDB, 0xDB, 0xDB, 0xDF, 0xDF}},
    {{0xff, 0xff, 0x3, 0x3, 0xff, 0xff, 0x3, 0x3}},
    {{0x0, 0x0, 0x5A, 0x7f, 0x7f, 0x49, 0x0, 0x0}},
};

static const Color BLACK = {.a = 255};
static const Color RED = {.r = 255, .a = 255};
static const Color GREEN    = {.g = 255, .a = 255};
static const Color YELLOW = {.r = 255, .g = 255, .a = 255};
static const Color BLUE = {.b = 255, .a = 255};
static const Color MAGENTA = {.r = 255, .b = 255, .a = 255};
static const Color CYAN = {.g = 255, .b = 255, .a = 255};
static const Color WHITE = {.r = 255, .g = 255, .b = 255, .a = 255};
static const Color GRAY = {.r = 127, .g = 127, .b = 127, .a = 255};

static struct {
  Color fg, bg;
} pals[] = {
    {BLACK, BLACK},
    {YELLOW, GREEN},
    {YELLOW, WHITE},
    {CYAN, BLUE},
    {WHITE, GRAY},
    {YELLOW, RED},
    {GREEN, RED},
    {RED, GREEN},
    {CYAN, MAGENTA},
    {RED, WHITE},
    {YELLOW, BLACK},
    {YELLOW, RED}
};

static void clear_display(Color color)
{
  for (size_t i = 0; i < ARRAY_LEN(display); ++i)
    display[i] = color;
}

static void put_pixel(int x, int y, Color color)
{
  if (x < 0 || x >= DISPLAY_WIDTH ||
      y < 0 || y >= DISPLAY_HEIGHT)
    return;
  display[y * DISPLAY_WIDTH + x] = color;
}

static void draw_line(
    int x0, int y0,
    int x1, int y1,
    Color color)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  int sx = ((x0 < x1)<<1)-1;
  int sy = ((y0 < y1)<<1)-1;

  int err = dx - dy;

  for (;;) {
    put_pixel(x0, y0, color);

    if (x0 == x1 && y0 == y1)
      break;

    int e2 = 2 * err;

    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }

    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static void draw_disk(
    int cx, int cy, int r,
    Color color)
{
  for (int j = -r; j <= r; ++j) {
    int y = cy + j;
    for (int i = -r; i <= r; ++i) {
      int x = cx + i;
      if ((i * i + j * j) > (r * r))
        continue;
      put_pixel(x, y, color);
    }
  }
}

static inline void draw_rect(int x, int y, int w, int h, Color color)
{
  for (int row = 0; row < h; ++row) {
    for (int col = 0; col < w; ++col) {
      put_pixel(col+x, row+y, color);
    }
  }
}

static inline void draw_tile(int x, int y, int scalex, int scaley, uint8_t block, uint8_t pal)
{
  uint8_t *bd = blocks[block].d;
  for (int row = 0; row < 8; ++row) {
    uint8_t bc = bd[row];
    for (int col = 0; col < 8; ++col) {
      if (bc & 1) {
        draw_rect(col * scalex + x, row * scaley + y, scalex, scaley, pals[pal].fg);
      } else {
        draw_rect(col * scalex + x, row * scaley + y, scalex, scaley, pals[pal].bg);
      }
      bc >>= 1;
    }
  }
}


Game game_init(void)
{
  clear_display((Color){.r = 12, .g = 16, .b = 24, .a = 255});

  ArenaStatus err;
  if ((err = initArena(&arena, abuf, MB(32))) != ARENA_OK) {
    char buf[KB(1)];
    ArenaErrStr(buf, KB(1), err, &arena);
    fprintf(stderr, "game_init():%d: initArena() error: %s\n", __LINE__, buf);
    exit(1);
  }

  data = (ModeData) {&arena, NULL, NULL, VERSION};
	mode = 0;
  time_t seed = time(NULL);
  srand((unsigned int)seed);

  ArenaNewZ(&arena, data.world);

  init_world(data.world, 1234567);

  data.player = create_player(data.world);

  return (Game) {
    .fps               = FPS,
    .display           = (uint32_t *) display,
    .display_width     = DISPLAY_WIDTH,
    .display_height    = DISPLAY_HEIGHT,
    .audio             = audio,
    .audio_sample_rate = AUDIO_SAMPLE_RATE,
    .audio_channels    = AUDIO_CHANNELS,
  };
}
void game_update(void) {
  draw_world(data.world, data.player->position);
}
void game_key_down(int key)
{}
void game_key_up(int key)
{}

void draw_creature(Creature const * const creature, Point player)
{
	Point center = { (DISPLAY_WIDTH / 2), (DISPLAY_HEIGHT / 2) };
	Point cursor = {0,0};

	if (creature == NULL) {
		return;
	}
	
	Point pos = creature->position;
	cursor = point_from_dir(creature->facing);

	Point modified_pos = {center.x + (pos.x - player.x)*32, center.y + (pos.y - player.y)*32 };

        Point cell = {cursor.x * 32 + modified_pos.x,
                      cursor.y * 32 + modified_pos.y};
        
  draw_tile(modified_pos.x, modified_pos.y, 4, 4, creature->grphr.block, creature->grphr.pal);
}

void draw_world(World const * const world, Point center)
{
	if (world == NULL) {
		return;
	}

	int startx = center.x - DISPLAY_WIDTH/64;
	int starty = center.y - DISPLAY_HEIGHT/64;

	for (int row = 0; row < DISPLAY_HEIGHT/32; row += 1) {
		for (int col = 0; col < DISPLAY_WIDTH/32; col += 1) {
			int mapx = startx + col;
			int mapy = starty + row;
			if (mapx < 0 || mapy < 0
				|| mapx >= WORLD_WIDTH || mapy >= WORLD_HEIGHT) {
				draw_tile(col*32, row*32, 4, 4, 0, 0);
				continue;
			}

			int sprite = 0, color = 0;
			Tile tile = world->tiles[mapy][mapx];
			TileType tile_type = tile_types[tile.type];
			ObjectType obj_type = obj_types[tile.object];

			if (tile.object != 0) {
				sprite = obj_type.grphr.block;
        color = obj_type.grphr.pal;
      } else {
				sprite = tile_type.grphr.block;
        color = tile_type.grphr.pal;
      }

			draw_tile(col * 32, row * 32, 4, 4, sprite, color);
		}
	}

	for (size_t i = 0; i < world->creature_count; ++i) {
		draw_creature(&world->creatures[i], center);
	}
}

