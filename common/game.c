#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "game.h"
#include "vec.h"

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

Game game_init(void)
{
  clear_display((Color) {
    .r = 12, .g = 16, .b = 24, .a = 255
  });

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
void game_update(void)
{
  draw_disk(100, 100, 25, (Color) { .r = 255, .a = 255 });
}
void game_key_down(int key)
{}
void game_key_up(int key)
{}
