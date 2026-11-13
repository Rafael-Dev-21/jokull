#include "com_rafael04th_jokull_JokullLibJNIWrapper.h"
#include "game.h"

#include <math.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <string.h>

Game game;

ANativeWindow *window;

static inline uint32_t flip_bytes(uint32_t x)
{
  return ((x & 0x000000ff) << 16) |
         ((x & 0x0000ff00))  |
         ((x & 0x00ff0000) >> 16)  |
         ((x & 0xff000000));
}

JNIEXPORT void JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_on_1game_1init
  (JNIEnv *env, jclass clazz, jobject surface)
{
  game = game_init();

  window = ANativeWindow_fromSurface(env, surface);


  ANativeWindow_setBuffersGeometry(
      window,
      ANativeWindow_getWidth(window),
      ANativeWindow_getHeight(window),
      AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
}

static void framebuffer_blit_centered(
    uint32_t *dst, int dw, int dh, int ds,
    const uint32_t *src, int sw, int sh, int ss)
{
  int x = (dw - sw) / 2;
  int y = (dh - sh) / 2;
  int sx = fmaxf(0, -x);
  int sy = fmaxf(0, -y);
  int dx = fmaxf(0, x);
  int dy = fmaxf(0, y);

  int w = fminf(sw-sx, dw-dx);
  int h = fminf(sh-sy, dh-dy);

  for (int row = 0; row < h; ++row) {
    const uint32_t *srow = src + (sy + row) * ss + sx;
    uint32_t *drow = dst + (dy + row) * ds + dx;
    for (int col = 0; col < w; ++col)
      drow[col] = flip_bytes(srow[col]);
  }
}

JNIEXPORT void JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_on_1game_1step
  (JNIEnv *env, jclass clazz, jobject audio_buffer)
{
  game_update();

  ANativeWindow_Buffer buffer;

  if (ANativeWindow_lock(window, &buffer, nullptr) >= 0) {
    framebuffer_blit_centered(
        buffer.bits, buffer.width, buffer.height, buffer.stride,
        game.display, game.display_width, game.display_height, game.display_width);
    ANativeWindow_unlockAndPost(window);
  }

  int16_t *out_samples = (*env)->GetDirectBufferAddress(env, audio_buffer);
  size_t audio_size_in_bytes = game.audio_sample_rate / game.fps * game.audio_channels * sizeof(*game.audio);
  memcpy(out_samples, game.audio, audio_size_in_bytes);
}

JNIEXPORT void JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_on_1game_1unload
  (JNIEnv *env, jclass clazz)
{
  ANativeWindow_release(window);
}

JNIEXPORT jlong JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_game_1fps
  (JNIEnv *env, jclass clazz)
{
  return game.fps;
}

JNIEXPORT jlong JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_game_1display_1width
  (JNIEnv *env, jclass clazz)
{
  return game.display_width;
}

JNIEXPORT jlong JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_game_1display_1height
  (JNIEnv *env, jclass clazz)
{
  return game.display_height;
}

JNIEXPORT jlong JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_game_1audio_1sample_1rate
  (JNIEnv *env, jclass clazz)
{
  return game.audio_sample_rate;
}

JNIEXPORT jlong JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_game_1audio_1channels
  (JNIEnv *env, jclass clazz)
{
  return game.audio_channels;
}
