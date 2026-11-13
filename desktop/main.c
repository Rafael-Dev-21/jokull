#include <stdio.h>
#include <time.h>
#include <X11/Xlib.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <unistd.h>

#include "game.h"

#define NAME    "Jokull"
#define VERSION "v0.1.0"
#define PROGRAM NAME " " VERSION

uint64_t current_nanos(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t) ts.tv_nsec + (uint64_t) ts.tv_sec * 1000000000;
}

int main(int argc, char *argv[])
{
  Game game = game_init();

  printf("game.fps               = %zu\n", game.fps);
  printf("game.display_width     = %zu\n", game.display_width);
  printf("game.display_height    = %zu\n", game.display_height);
  printf("game.audio_sample_rate = %zu\n", game.audio_sample_rate);
  printf("game.audio_channels    = %zu\n", game.audio_channels);

  pa_sample_spec ss = (pa_sample_spec) {
    .format = PA_SAMPLE_S16LE,
    .channels = game.audio_channels,
    .rate = game.audio_sample_rate,
  };

  int error;
  pa_simple *s = pa_simple_new(
      nullptr,
      PROGRAM,
      PA_STREAM_PLAYBACK,
      nullptr,
      "playback",
      &ss,
      nullptr,
      nullptr,
      &error);
  if (s == nullptr) {
    fprintf(stderr, "ERROR: Couldn't create simple playback: %s\n", pa_strerror(error));
    return 1;
  }

  Display *display = XOpenDisplay(nullptr);
  if (display == nullptr) {
    fprintf(stderr, "ERROR: Couldn't get default display\n");
    return 1;
  }
  printf("display = %p\n", display);

  Window window = XCreateSimpleWindow(
      display,
      XDefaultRootWindow(display),
      0, 0,
      game.display_width, game.display_height,
      0, 0, 0);
  printf("window = %ld\n", window);

  XWindowAttributes xa = {0};
  XGetWindowAttributes(display, window, &xa);

  XImage *image = XCreateImage(
      display,
      xa.visual,
      xa.depth,
      ZPixmap,
      0,
      (char *) game.display,
      game.display_width,
      game.display_height,
      32,
      game.display_width * sizeof(*game.display));
  printf("image = %p\n", image);

  GC gc = XCreateGC(display, window, 0, nullptr);
  printf("gc = %p\n", gc);

  Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(display, window, &wm_delete_window, 1);

  XSelectInput(display, window, KeyPressMask | PointerMotionMask);

  XStoreName(display, window, PROGRAM);
  XMapWindow(display, window);

  bool quit = false;
  while (!quit) {
    uint64_t begin = current_nanos();
    uint64_t delta_time = 1000000000/game.fps;

    while (XPending(display) > 0) {
      XEvent event = {0};
      XNextEvent(display, &event);
      switch (event.type) {
      case KeyPress:
        switch (XLookupKeysym(&event.xkey, 0)) {
          case 'q':
            quit = true;
            break;
        }
        break;
      case MotionNotify:
        // event.xmotion.x, event.xmotion.y
        break;
      case ClientMessage:
        if ((Atom) event.xclient.data.l[0] == wm_delete_window) {
          quit = true;
        }
        break;
      }
    }

    game_update();

    XPutImage(display, window, gc, image, 0, 0, 0, 0, game.display_width, game.display_height);
    size_t audio_size_in_bytes = game.audio_sample_rate/game.fps*game.audio_channels*sizeof(*game.audio);
    pa_simple_write(s, game.audio, audio_size_in_bytes, &error);
    
    uint64_t end = current_nanos();
    uint64_t delta = end - begin;

    if (delta < delta_time) {
      usleep((delta_time-delta)/1000);
    }
  }

  return 0;
}
