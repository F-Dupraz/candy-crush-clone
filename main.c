#include <stdio.h>
#include <SDL3/SDL.h>

int game_is_running = 0;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void destroy_window(void)
{
  if (renderer)
  {
    SDL_DestroyRenderer(renderer);
    renderer = NULL;
  }
  if (window)
  {
    SDL_DestroyWindow(window);
    window = NULL;
  }
  SDL_Quit();
}

int create_window(void)
{
  if(!SDL_Init(SDL_INIT_VIDEO))
  {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return 0;
  }

  window = SDL_CreateWindow("Candy crush clone", 1080, 720, SDL_WINDOW_BORDERLESS);
  
  if(!window)
  {
    fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 0;
  }

  renderer = SDL_CreateRenderer(window, NULL);
  if(!renderer)
  {
    fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
    destroy_window();
    return 0;
  }

  return 1;
}

void process_input()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
      case SDL_EVENT_QUIT:
        game_is_running = 0;
        break;
      case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_ESCAPE)
          game_is_running = 0;
        break;
    }
  }
}

void update()
{
  // TODO
}

void render()
{
  SDL_SetRenderDrawColor(renderer, 10, 40, 50, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{  
  game_is_running = create_window();
  
  while (game_is_running)
  {
    process_input();
    update();
    render();
  }
  
  destroy_window();

  return 0;
}
