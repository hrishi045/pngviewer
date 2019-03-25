#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <spng.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

spng_ctx *ctx;
unsigned char *out;
char *pngbuf = NULL;

enum {
  ERROR,
  NOT_PNG,
  LIBSPNG_ERROR,
  LIBSDL_ERROR,
};

void pv_exit(char *str, unsigned int code) {
  spng_ctx_free(ctx);
  free(out);
  free(pngbuf);
  perror(str);
  exit(code);
}

int main(int argc, char *argv[argc + 1]) {
  char *infile = argv[1];

  FILE *png = fopen(infile, "rb");
  if (!png) pv_exit("fopen()", ERROR);

  int r = 0;
  fseek(png, 0, SEEK_END);

  long siz_pngbuf = ftell(png);
  rewind(png);

  pngbuf = malloc(siz_pngbuf);
  if (pngbuf == NULL) pv_exit("malloc()", ERROR);

  if (fread(pngbuf, siz_pngbuf, 1, png) != 1) pv_exit("fread()", ERROR);

  ctx = spng_ctx_new(0);
  if (ctx == NULL) pv_exit("spng_ctx_new()", LIBSPNG_ERROR);

  r = spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
  if (r) {
    puts(spng_strerror(r));
    pv_exit("spng_set_src_action()", LIBSPNG_ERROR);
  }

  r = spng_set_png_buffer(ctx, pngbuf, siz_pngbuf);
  if (r) {
    puts(spng_strerror(r));
    pv_exit("spng_set_png_buffer()", LIBSPNG_ERROR);
  }

  struct spng_ihdr ihdr;
  r = spng_get_ihdr(ctx, &ihdr);
  if (r) {
    puts(spng_strerror(r));
    pv_exit("spng_get_ihdr()", LIBSPNG_ERROR);
  }

  size_t out_size;
  r = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size);
  if (r) {
    puts(spng_strerror(r));
    pv_exit("spng_decoded_image_size()", LIBSPNG_ERROR);
  }

  uint8_t *png_buf = malloc(out_size);

  spng_decode_image(ctx, png_buf, out_size, SPNG_FMT_RGBA8, SPNG_DECODE_USE_TRNS);

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  pv_exit("SDL_Init()", LIBSDL_ERROR);

  SDL_Window *win = SDL_CreateWindow(
    argv[1],
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    ihdr.width, ihdr.height, SDL_WINDOW_SHOWN);
  
  if (win == NULL) pv_exit("SDL_CreateWindow()", LIBSDL_ERROR);

  SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == NULL) pv_exit("SDL_CreateRenderer", LIBSDL_ERROR);

  bool quit = false;
  SDL_Event event;
  while (!quit) {
    SDL_RenderClear(ren);
    size_t l = 0;
    for (size_t i = 0; i < ihdr.height; i++) {
      for (size_t j = 0; j < ihdr.width; j++) {
        uint8_t r, g, b, a;
        r = png_buf[l++];
        g = png_buf[l++];
        b = png_buf[l++];
        a = png_buf[l++];
        SDL_SetRenderDrawColor(ren, r, g, b, a);
        SDL_RenderDrawPoint(ren, j, i);
      }
    }
    l = 0;
    // SDL_SetRenderDrawColor(ren, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderPresent(ren);
    
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit = true;
      }
    }
  }

  SDL_DestroyWindow(win);
  SDL_Quit();
  return EXIT_SUCCESS;

  spng_ctx_free(ctx);
  free(out);
  free(pngbuf);
  free(png_buf);

  return EXIT_SUCCESS;
}
