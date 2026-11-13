package com.rafael04th.jokull;

class JokullLibJNIWrapper {
  static {
    System.loadLibrary("jokull");
  }

  public static native void on_game_init(Object surface);
  public static native void on_game_step(Object audio_buffer);
  public static native void on_game_unload();

  public static native long game_fps();
  public static native long game_display_width();
  public static native long game_display_height();
  public static native long game_audio_sample_rate();
  public static native long game_audio_channels();
}
