#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "game.h"

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

#define PI 3.14159265358979323846
#define TAU (2.0 * PI)

#define BMP 120

#define NOTE_FRAMES (AUDIO_SAMPLE_RATE*60/BMP/2)

#define FM_RATIO 2.0
#define FM_INDEX 2.5

#define FM_VOLUME 0.45

#define ATTACK_FRAMES 220
#define RELEASE_FRAMES 1323

static const double melody[] = {
  523.25, 659.25, 783.99, 659.25,
  440.00, 523.25, 659.25, 523.25,
  349.23, 440.00, 523.25, 440.00,
  392.00, 493.88, 587.33, 493.88,
};

#define MELODY_LENGTH (sizeof(melody) / sizeof(melody[0]))

static uint64_t audio_clock;

static double carrier_phase;
static double modulator_phase;

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

static double envelope(size_t position)
{
  double attack = 1.0;
  double release = 1.0;

  if (position < ATTACK_FRAMES)
    attack = (double) position / ATTACK_FRAMES;

  size_t remaining = NOTE_FRAMES - position;

  if (remaining < RELEASE_FRAMES)
    release = (double) remaining / RELEASE_FRAMES;

  return attack * release;
}

static void synthesize(int16_t *buffer, size_t frames)
{
  for (size_t i = 0; i < frames; ++i) {
    size_t note_index = (audio_clock / NOTE_FRAMES) % MELODY_LENGTH;
    size_t note_position = audio_clock % NOTE_FRAMES;

    double frequency = melody[note_index];
    double amplitude = envelope(note_position);
    double modulation = sin(modulator_phase) * FM_INDEX;
    double sample = sin(carrier_phase + modulation);

    sample *= amplitude * FM_VOLUME;

    int16_t output = (int16_t) (sample * 32767.0);

    for (size_t j = 0; j < AUDIO_CHANNELS; ++j) {
      buffer[i * AUDIO_CHANNELS + j] = output;
    }

    carrier_phase += TAU * frequency / AUDIO_SAMPLE_RATE;
    modulator_phase += TAU * frequency * FM_RATIO / AUDIO_SAMPLE_RATE;

    if (carrier_phase >= TAU)
      carrier_phase -= TAU;
    if (modulator_phase >= TAU)
      modulator_phase -= TAU;
    ++audio_clock;
  }
}

static void draw_oscilloscope(void)
{
  Color background = {
    .r = 12, .g = 16, .b = 24, .a = 255
  };

  Color baseline = {
    .r = 55, .g = 65, .b = 75, .a = 255
  };

  Color waveform = {
    .r = 80, .g = 230, .b = 150, .a = 255
  };

  clear_display(background);

  int center = DISPLAY_HEIGHT/2;

  draw_line(
      0, center,
      DISPLAY_WIDTH-1, center,
      baseline);

  int previous_y = center;

  for (int x = 0; x < DISPLAY_WIDTH; ++x) {
    size_t index = (size_t) x * AUDIO_FRAMES_PER_TICK / DISPLAY_WIDTH;
    int16_t sample = audio[index * AUDIO_CHANNELS];

    int y = center - (int) ((int32_t) sample * (DISPLAY_HEIGHT/3) / 32768);
    
    if (x > 0)
      draw_line(x-1, previous_y, x, y, waveform);

    previous_y = y;
  }
}

Game game_init(void)
{
  carrier_phase = 0.0;
  modulator_phase = 0.0;
  audio_clock = 0;

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
  synthesize(audio, AUDIO_FRAMES_PER_TICK);
  draw_oscilloscope();
}
void game_key_down(int key)
{}
void game_key_up(int key)
{}
