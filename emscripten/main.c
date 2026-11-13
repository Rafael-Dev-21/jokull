#include <stdio.h>
#include <stdlib.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/webaudio.h>
#include <emscripten.h>
#include "game.h"

static Game game;

#include <emscripten.h>
#include <stdint.h>

#define AUDIO_FRAMES 735

EM_JS(void, audio_init, (), {
    const ctx = new AudioContext();

    let nextTime = 0;

    // O navegador exige interação do usuário
    // para liberar a reprodução de áudio.
    document.addEventListener("pointerdown", () => {
        ctx.resume();
    }, { once: true });

    Module.audioContext = ctx;
    Module.audioNextTime = nextTime;
});

EM_JS(void, audio_submit, (const int16_t *ptr, int frames), {
    const ctx = Module.audioContext;

    if (!ctx || ctx.state !== "running")
        return;

    const samples = HEAP16.subarray(
        ptr >> 1,
        (ptr >> 1) + frames * 2
    );

    const buffer = ctx.createBuffer(2, frames, 44100);

    const left = buffer.getChannelData(0);
    const right = buffer.getChannelData(1);

    for (let i = 0; i < frames; ++i) {
        left[i]  = samples[i * 2] / 32768;
        right[i] = samples[i * 2 + 1] / 32768;
    }

    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.connect(ctx.destination);

    const duration = frames / 44100;

    if (Module.audioNextTime < ctx.currentTime ||
        Module.audioNextTime > ctx.currentTime + 0.15) {
      Module.audioNextTime = ctx.currentTime + 0.03;
    }

    const start = Math.max(
        ctx.currentTime + 0.03,
        Module.audioNextTime
    );

    source.start(start);
    Module.audioNextTime = start + duration;
});

static void render()
{
  game_update();

  audio_submit(game.audio, 735);

  EM_ASM({
    const width = $0;
    const height = $1;
    const offset = $2 >> 2;

    const pixels = HEAPU32;
    const canvas = document.getElementById("canvas");
    const ctx = canvas.getContext("2d");
    const imgData = ctx.getImageData(0, 0, width, height);
    const data = imgData.data;

    for (let i = 0; i < width * height; ++i) {
      const pixel = pixels[offset + i];

      data[i * 4 + 0] = (pixel >> 16) & 0xff;
      data[i * 4 + 1] = (pixel >> 8) & 0xff;
      data[i * 4 + 2] = pixel & 0xff;
      data[i * 4 + 3] = 255;
    }

    ctx.putImageData(imgData, 0, 0);

  }, game.display_width, game.display_height, game.display);
}

int main()
{
  game = game_init();
  
  audio_init();

  EM_ASM({
    const canvas = document.getElementById("canvas");
    canvas.width = $0;
    canvas.height = $1;
  }, game.display_width, game.display_height);

  emscripten_set_main_loop(render, game.fps, EM_FALSE);

  return 0;
}
