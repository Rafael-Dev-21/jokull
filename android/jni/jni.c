#include "com_rafael04th_jokull_JokullLibJNIWrapper.h"
#include "game.h"

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
      game.display_width,
      game.display_height,
      AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);
}

JNIEXPORT void JNICALL Java_com_rafael04th_jokull_JokullLibJNIWrapper_on_1game_1step
  (JNIEnv *env, jclass clazz, jobject audio_buffer)
{
  game_update();

  ANativeWindow_Buffer buffer;

  if (ANativeWindow_lock(window, &buffer, nullptr) >= 0) {
    for (size_t y = 0; y < game.display_height; y++) {
      for (size_t x = 0; x < game.display_width; x++) {
        *((uint32_t *) buffer.bits + y * buffer.stride + x) = flip_bytes(game.display[y * game.display_width+x]);
      }
    }
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
