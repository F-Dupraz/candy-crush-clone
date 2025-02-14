#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "libs/cJSON.h"

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

// SDL_Texture *main_texture = NULL;
// SDL_Texture *play_button_texture = NULL;

FILE *fp = NULL;

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
  SDL_Texture *texture;
  SDL_FRect dimensions;
  SDL_FRect position;
  void (*go_quit)(struct Game_object*);
  void (*go_update)(void);
  void (*go_render)(SDL_Renderer*, struct Game_object*);
  void (*go_handle_event)(SDL_Event*, struct Game_object*);
} Game_object;

void go_render(SDL_Renderer *renderer, Game_object *self) {
  if(self->texture == NULL)
    return;

  SDL_RenderTexture(renderer, self->texture, &self->dimensions, &self->position);
}

void go_quit(Game_object* self) {
  if (self->texture != NULL) {
    SDL_DestroyTexture(self->texture);
    self->texture = NULL;
  }
}

void go_update(void) {}
void go_handle_event(SDL_Event *event, Game_object *self) {}

void go_pb_handle_event(SDL_Event *event, Game_object *self)
{
  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    SDL_FPoint point = { (float)event->button.x, (float)event->button.y };

    if (SDL_PointInRectFloat(&point, &self->position)) {
      game_state = LEVELS_MENU;
      levels_menu_init();
      main_menu_cleanup();
    }
  }
}

Game_object init_play_button(SDL_Renderer *renderer)
{
  const char *play_button_path = "./assets/play_button.png";
  Game_object play_button = {
    .texture = IMG_LoadTexture(renderer, play_button_path),
    .dimensions = { 250, 190, 790, 270 },
    .position = { (1080/2)-(320/2), (2*720/3)-(110/2), 320, 110 },
    .go_render = go_render,
    .go_quit = go_quit,
    .go_update = go_update,
    .go_handle_event = go_pb_handle_event,
  };

  return play_button;
}

Game_object init_main_texture(SDL_Renderer *renderer)
{
  const char *main_path = "./assets/main_ccc.png";
  Game_object main = {
    .texture = IMG_LoadTexture(renderer, main_path),
    .dimensions = { 0, 0, 1080, 720 },
    .position = { 0, 0, 1080, 720 },
    .go_render = go_render,
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
    game_objects[i].go_quit(&game_objects[i]);

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

  SDL_DisplayID display = SDL_GetPrimaryDisplay();
  SDL_Rect bounds;
  SDL_GetDisplayBounds(display, &bounds);

  int posX = (bounds.w - 1080) / 2;
  int posY = (bounds.h - 720) / 2;

  SDL_SetWindowPosition(window, posX, posY);

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
      game_objects[i].go_handle_event(&event, &game_objects[i]);
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
    game_objects[i].go_render(renderer, &game_objects[i]);

  SDL_RenderPresent(renderer);
}

void main_menu_cleanup()
{
  // if (main_texture) {
  //   SDL_DestroyTexture(main_texture);
  //   main_texture = NULL;
  // }
    
  // if (play_button_texture) {
  //   SDL_DestroyTexture(play_button_texture);
  //   play_button_texture = NULL;
  // }

  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit(&game_objects[i]);

  game_object_count = 0;
}

// --------------------------------------------------
//  LEVELS MENU STATE FUNCTIONS
// --------------------------------------------------

void levels_menu_init()
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 250, 250);

  fp = fopen("./levels.json", "r");
  
  if (fp == NULL) { 
    printf("Error: Unable to open the file.\n"); 
    return; 
  }

  char buffer[10000];
  int len = fread(buffer, 1, sizeof(buffer), fp);

  if (len <= 0) { 
    printf("Error: Unable to read the file.\n");
    fclose(fp);
    return; 
  }

  fclose(fp);

  cJSON *json = cJSON_Parse(buffer);
  if (json == NULL) { 
    const char *error_ptr = cJSON_GetErrorPtr(); 
    if (error_ptr != NULL) { 
      printf("Error: %s\n", error_ptr); 
    } 
    cJSON_Delete(json); 
    return; 
  }

  cJSON *all_levels = cJSON_GetObjectItemCaseSensitive(json, "all_levels");

  if (cJSON_IsNumber(all_levels)) {
    int valor = all_levels->valueint;

    printf("El valor es: %d\n", valor);
  }

  cJSON_Delete(json);
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
      game_objects[i].go_handle_event(&event, &game_objects[i]);
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
    game_objects[i].go_render(renderer, &game_objects[i]);

  SDL_RenderPresent(renderer);
}

void levels_menu_cleanup()
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit(&game_objects[i]);

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
