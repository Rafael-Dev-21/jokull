package com.rafael04th.jokull;

import android.content.Context;
import android.graphics.Canvas;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class GameViewWrapper extends SurfaceView implements Runnable, SurfaceHolder.Callback {
  
  private static final long FPS = 60;
  private static final long DISPLAY_WIDTH = 800;
  private static final long DISPLAY_HEIGHT = 600;
  private static final long AUDIO_SAMPLE_RATE = 44100;
  private static final long AUDIO_CHANNELS = 2;
  
  private volatile boolean running = false;
  private Thread worker;

  private AudioTrack audioTrack;
  private ByteBuffer directBuffer;
  
  public GameViewWrapper(Context context) {
    this(context, null);
  }
  
  public GameViewWrapper(Context context, AttributeSet attrs) {
    super(context, attrs);
    
    worker = new Thread(this);
    getHolder().addCallback(this);
  }
  
  @Override
  public void surfaceCreated(SurfaceHolder holder) {
    JokullLibJNIWrapper.on_game_init(holder.getSurface());

    if (worker.getState() == Thread.State.TERMINATED) {
      worker = new Thread(this);
    } else {
      onLoad();
    }

    running = true;
    worker.start();
  }
  
  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}
  
  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {
    boolean retry = true;
    running = false;
    while (retry) {
      try {
        worker.join();
        retry = false;
      } catch (InterruptedException e) {
        Thread.currentThread().interrupt();
      }
    }
    JokullLibJNIWrapper.on_game_unload();
  }
  
  @Override
  public void run() {
    Canvas canvas = null;
    audioTrack.play();
    while (running) {
      long deltaTime = 1000000000L / JokullLibJNIWrapper.game_fps();
      long begin = System.nanoTime();
      if (!getHolder().getSurface().isValid()) {
        continue;
      }
     
      /*
      try {
        canvas = getHolder().lockCanvas();
        
        synchronized (getHolder()) {
          onStep(canvas);
        }
      } catch (Exception e) {
        
      } finally {
        if (canvas != null) {
          getHolder().unlockCanvasAndPost(canvas);
          canvas = null;
        }
      }*/

      directBuffer.clear();
      
      synchronized(getHolder()) {
        JokullLibJNIWrapper.on_game_step(directBuffer);
      }

      int audioSizeInBytes = (int)JokullLibJNIWrapper.game_audio_sample_rate() / (int)JokullLibJNIWrapper.game_fps() * (int)JokullLibJNIWrapper.game_audio_channels() * Short.BYTES;
      audioTrack.write(directBuffer, audioSizeInBytes, AudioTrack.WRITE_BLOCKING);

      long end = System.nanoTime();
      long delta = end - begin;
      long remaining = deltaTime - delta;
      if (remaining > 0) {
        long millis = remaining / 1000000L;
        int nanos = (int) (remaining % 1000000L);
        try {
          Thread.sleep(millis, nanos);
        } catch (InterruptedException e) {
          Thread.currentThread().interrupt();
        }
      }
    }
    audioTrack.stop();
    onUnload();
  }
  
  public void onLoad() {
    int sampleRate = (int)JokullLibJNIWrapper.game_audio_sample_rate();
    int channels = (int)JokullLibJNIWrapper.game_audio_channels();
    int channelConfig = channels == 1 ? AudioFormat.CHANNEL_OUT_MONO : AudioFormat.CHANNEL_OUT_STEREO;
    int audioFormat = AudioFormat.ENCODING_PCM_16BIT;

    int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat);

    directBuffer = ByteBuffer.allocateDirect(minBufferSize).order(ByteOrder.LITTLE_ENDIAN);

    audioTrack = new AudioTrack.Builder()
      .setAudioAttributes(
          new AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
            .build()
      )
      .setAudioFormat(
          new AudioFormat.Builder()
            .setEncoding(audioFormat)
            .setSampleRate(sampleRate)
            .setChannelMask(channelConfig)
            .build()
      )
      .setBufferSizeInBytes(minBufferSize)
      .setTransferMode(AudioTrack.MODE_STREAM)
      .build();
  }
  public void onStep(Canvas canvas) {
    canvas.drawARGB(255, 255, 0, 0);
  }
  public void onUnload() {}

  public void release() {
    audioTrack.release();
  }
}
