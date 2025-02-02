#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_image.h>

#define MAX_GAME_OBJECTS 100

int game_is_running = 0;

typedef enum Game_states {
  MAIN_MENU,
  LEVELS_MENU,
  PLAYING
} Game_states;

Game_states game_state = MAIN_MENU;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

SDL_Texture *main_texture = NULL;
SDL_Texture *play_button_texture = NULL;

typedef struct Game_object
{
  void (*go_quit)(void);
  void (*go_update)(void);
  void (*go_render)(SDL_Renderer*);
  void (*go_handle_event)(SDL_Event*);
} Game_object;

void go_render(SDL_Renderer *renderer) {}
void go_quit(void) {}
void go_update(void) {}
void go_handle_event(SDL_Event *event) {}

void go_pb_render(SDL_Renderer *renderer)
{
  SDL_FRect play_button_position = { (1080/2)-(485/2), (2*720/3)-(300/2), 485, 300 };

  SDL_SetTextureScaleMode(play_button_texture, SDL_SCALEMODE_NEAREST);
  SDL_RenderTexture(renderer, play_button_texture, NULL, &play_button_position);
}

Game_object init_play_button(SDL_Renderer *renderer)
{
  const char *play_button_path = "./assets/play_button.png";
  play_button_texture = IMG_LoadTexture(renderer, play_button_path);

  Game_object play_button = {
    .go_render = go_pb_render,
    .go_quit = go_quit,
    .go_update = go_update,
    .go_handle_event = go_handle_event,
  };

  return play_button;
}

int game_object_count = 0;
Game_object game_objects[MAX_GAME_OBJECTS];

void destroy_window(void)
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit();

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

// --------------------------------------------------
//  MAIN MENU STATE FUNCTIONS
// --------------------------------------------------

void main_menu_init(void)
{
  main_texture = IMG_LoadTexture(renderer, "./assets/main_ccc.png");

  game_objects[game_object_count++] = init_play_button(renderer);
}

void main_menu_process_input()
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

void main_menu_update()
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_update();
}

void main_menu_render()
{
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, main_texture, NULL, NULL);

  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_render(renderer);

  SDL_RenderPresent(renderer);
}

// --------------------------------------------------
//  GENERAL FUNCTIONS FOR EACH STATE
// --------------------------------------------------

void process_input()
{
  switch (game_state)
  {
    case MAIN_MENU:
      main_menu_process_input();
      break;
    case LEVELS_MENU:
      // levels_menu_process_input();
      break;
    case PLAYING:
      // playing_process_input();
      break;
  }
}

void update_state()
{
  switch (game_state)
  {
    case MAIN_MENU:
      main_menu_update();
      break;
    case LEVELS_MENU:
      // levels_menu_update();
      break;
    case PLAYING:
      // playing_update();
      break;
  }
}

void render_state()
{
  switch (game_state)
  {
    case MAIN_MENU:
      main_menu_render();
      break;
    case LEVELS_MENU:
      // levels_menu_render();
      break;
    case PLAYING:
      // playing_render();
      break;
  }
}

// --------------------------------------------------
//  MAIN GAME LOOP
// --------------------------------------------------

int main(int argc, char *argv[])
{  
  game_is_running = create_window();

  main_menu_init();
  
  while (game_is_running)
  {
    process_input();
    update_state();
    render_state();
  }
  
  destroy_window();

  return 0;
}
