#include "../include/game.h"
#include "../include/tmx_parser.h"
#include <iostream>

int main() {
  SDL_Surface *loadedSurface = IMG_Load("../resources/monkey.png");
  Game game("ragebait", "monkey.png", 800, 600, SDL_WINDOW_SHOWN,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  game.run();

  return 0;
}