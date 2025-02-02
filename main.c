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

void main_menu_init();
void main_menu_cleanup();
void main_menu_process_input();
void main_menu_update();
void main_menu_render();

void levels_menu_init();
void levels_menu_cleanup();
void levels_menu_process_input();
void levels_menu_update();
void levels_menu_render();

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
  SDL_FRect play_button_dimensions = { 250, 190, 790, 270 };
  SDL_FRect play_button_position = { (1080/2)-(320/2), (2*720/3)-(110/2), 320, 110 };

  SDL_RenderTexture(renderer, play_button_texture, &play_button_dimensions, &play_button_position);
}

void go_pb_handle_event(SDL_Event *event)
{
  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    SDL_FPoint point = { (float)event->button.x, (float)event->button.y };
    SDL_FRect play_button_rect = { (1080/2)-(320/2), (2*720/3)-(110/2), 320, 110 };

    if (SDL_PointInRectFloat(&point, &play_button_rect)) {
      game_state = LEVELS_MENU;
      levels_menu_init();
      main_menu_cleanup();
    }
  }
}

Game_object init_play_button(SDL_Renderer *renderer)
{
  const char *play_button_path = "./assets/play_button.png";
  play_button_texture = IMG_LoadTexture(renderer, play_button_path);
  // SDL_SetTextureScaleMode(play_button_texture, SDL_SCALEMODE_NEAREST);

  Game_object play_button = {
    .go_render = go_pb_render,
    .go_quit = go_quit,
    .go_update = go_update,
    .go_handle_event = go_pb_handle_event,
  };

  return play_button;
}

void go_main_render(SDL_Renderer *renderer)
{
  SDL_RenderTexture(renderer, main_texture, NULL, NULL);
}

Game_object init_main_texture(SDL_Renderer *renderer)
{
  const char *main_path = "./assets/main_ccc.png";
  main_texture = IMG_LoadTexture(renderer, main_path);
  SDL_SetTextureScaleMode(main_texture, SDL_SCALEMODE_NEAREST);

  Game_object main = {
    .go_render = go_main_render,
    .go_quit = go_quit,
    .go_update = go_update,
    .go_handle_event = go_handle_event,
  };

  return main;
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
  game_objects[game_object_count++] = init_main_texture(renderer);
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
    if(game_object_count == 0)
      continue;
    for (int i = 0; i < game_object_count; i++) {
      game_objects[i].go_handle_event(&event);
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

  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_render(renderer);

  SDL_RenderPresent(renderer);
}

void main_menu_cleanup()
{
  if (main_texture) {
    SDL_DestroyTexture(main_texture);
    main_texture = NULL;
  }
    
  if (play_button_texture) {
    SDL_DestroyTexture(play_button_texture);
    play_button_texture = NULL;
  }

  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit();

  game_object_count = 0;
}

// --------------------------------------------------
//  LEVELS MENU STATE FUNCTIONS
// --------------------------------------------------

void levels_menu_init()
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 250, 250);
  // game_objects[game_object_count++] = init_levels_menu_texture(renderer);
}

void levels_menu_process_input()
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
    if(game_object_count == 0)
      continue;
    for (int i = 0; i < game_object_count; i++) {
      game_objects[i].go_handle_event(&event);
    }
  }
}

void levels_menu_update()
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_update();
}

void levels_menu_render()
{
  SDL_RenderClear(renderer);

  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_render(renderer);

  SDL_RenderPresent(renderer);
}

void levels_menu_cleanup()
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit();

  game_object_count = 0;
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
      levels_menu_process_input();
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
      levels_menu_update();
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
      levels_menu_render();
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
