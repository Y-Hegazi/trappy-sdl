#include "../include/config.h"
#include "../include/game.h"
#include "../include/tmx_parser.h"
#include <iostream>

int main() {
  SDL_Surface *loadedSurface = IMG_Load(PLAYER_TEXTURE_PATH);
  Game game(WINDOW_TITLE, PLAYER_TEXTURE_PATH, WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  game.run();

  return 0;
}