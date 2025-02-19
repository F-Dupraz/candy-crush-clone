#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

FILE *fp = NULL;

SDL_Cursor* handCursor;
SDL_Cursor* arrowCursor;

void init_cursors() {
  handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
  arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
}

void cleanup_cursors() {
  SDL_DestroyCursor(handCursor);
  SDL_DestroyCursor(arrowCursor);
}

int isMouseOverButton(int mouseX, int mouseY, int buttonX, int buttonY, int buttonWidth, int buttonHeight)
{
  if((mouseX >= buttonX && mouseX <= buttonX + buttonWidth &&
    mouseY >= buttonY && mouseY <= buttonY + buttonHeight))
    return 1;
  else 
    return 0;
}

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

void playing_init(uint_least8_t level_id);
void playing_cleanup();
void playing_process_input();
void playing_update();
void playing_render();

typedef struct Game_object
{
  uint_least8_t id;
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
  SDL_FPoint point;

  switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      point.x = (float)event->button.x;
      point.y = (float)event->button.y;
      if (SDL_PointInRectFloat(&point, &self->position)) {
        game_state = LEVELS_MENU;
        main_menu_cleanup();
        levels_menu_init();
      }
      break;
    case SDL_EVENT_MOUSE_MOTION:
      point.x = (float)event->motion.x;
      point.y = (float)event->motion.y;
      if (SDL_PointInRectFloat(&point, &self->position))
        SDL_SetCursor(handCursor);
      else
        SDL_SetCursor(arrowCursor);
      break;
  }
}

Game_object init_play_button(SDL_Renderer *renderer)
{
  const char *play_button_path = "./assets/play_button.png";
  Game_object play_button = {
    .id = 0,
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
  const char *main_path = "./assets/candy_crush_clone.png";
  Game_object main = {
    .id = 1,
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

Game_object init_level_button(SDL_Renderer *renderer, float width, float height, uint_least8_t level_id)
{
  const char *level_path = "./assets/unfinished_level_button.png";
  Game_object level_button = {
    .id = level_id,
    .texture = IMG_LoadTexture(renderer, level_path),
    .dimensions = { 0, 0, 250, 250 },
    .position = { width, height, 30, 30 },
    .go_render = go_render,
    .go_quit = go_quit,
    .go_update = go_update,
    .go_handle_event = go_handle_event,
  };

  return level_button;
}

void go_pl_handle_event(SDL_Event *event, Game_object *self) {
  SDL_FPoint point;

  switch (event->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      point.x = (float)event->button.x;
      point.y = (float)event->button.y;
      if (SDL_PointInRectFloat(&point, &self->position)) {
        game_state = PLAYING;
        levels_menu_cleanup();
        playing_init(self->id);
      }
      break;
    case SDL_EVENT_MOUSE_MOTION:
      point.x = (float)event->motion.x;
      point.y = (float)event->motion.y;
      if (SDL_PointInRectFloat(&point, &self->position))
        SDL_SetCursor(handCursor);
      else
        SDL_SetCursor(arrowCursor);
      break;
  }
}

Game_object init_playable_level_button(SDL_Renderer *renderer, float width, float height, uint_least8_t level_id)
{
  const char *playable_level_path = "./assets/finished_level_button.png";
  Game_object playable_level_button = {
    .id = level_id,
    .texture = IMG_LoadTexture(renderer, playable_level_path),
    .dimensions = { 0, 0, 250, 250 },
    .position = { width, height, 30, 30 },
    .go_render = go_render,
    .go_quit = go_quit,
    .go_update = go_update,
    .go_handle_event = go_pl_handle_event,
  };

  return playable_level_button;
}

int game_object_count = 0;
Game_object game_objects[MAX_GAME_OBJECTS];

void destroy_window(void)
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit(&game_objects[i]);

  if (handCursor)
  {
    SDL_DestroyCursor(handCursor);
    handCursor = NULL;
  }
  if (arrowCursor)
  {
    SDL_DestroyCursor(arrowCursor);
    arrowCursor = NULL;
  }
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

  init_cursors();

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
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_quit(&game_objects[i]);

  game_object_count = 0;
}

// --------------------------------------------------
//  LEVELS MENU STATE FUNCTIONS
// --------------------------------------------------

void levels_menu_init()
{
  game_objects[game_object_count++] = init_main_texture(renderer);

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

  cJSON *playable_levels = cJSON_GetObjectItemCaseSensitive(json, "last_Level");
  cJSON *all_levels = cJSON_GetObjectItemCaseSensitive(json, "all_levels");

  int grid, all_levels_value, playable_levels_value;

  if (cJSON_IsNumber(all_levels)) {
    playable_levels_value = playable_levels->valueint;
    all_levels_value = all_levels->valueint;
    grid = sqrt(all_levels_value) + 1;
  }

  float height = (720 / 2) / (grid / 2) + 100;
  float width = (1080 / 4) / (grid / 2);
  int remaining_playable = playable_levels_value + 1;
  uint_least8_t level_id = 0;
  
  for(int j = 0; j < grid; ++j)
  {
    if(j * grid >= all_levels_value)
      break;
    for(int k = 0; k < grid; ++k)
    {
      if((j * grid) + k >= all_levels_value)
        break;
      if(remaining_playable > 0)
      {
        game_objects[game_object_count++] = init_playable_level_button(renderer, width, height, level_id);
        remaining_playable--;
      }
      else
        game_objects[game_object_count++] = init_level_button(renderer, width, height, level_id);

      level_id++;
      width += 30 + ((915 / 2) / (grid / 2));
    }
    width = (1080 / 4) / (grid / 2);
    height += 30 + ((620 / 4) / (grid / 2));
  }

  cJSON_Delete(json);
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
//  PLAYING STATE FUNCTIONS
// --------------------------------------------------

void playing_init(uint_least8_t level_id)
{
  game_objects[game_object_count++] = init_main_texture(renderer);
  SDL_SetCursor(arrowCursor);

  printf("Level id: %d\n", level_id);

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

  cJSON *levels = cJSON_GetObjectItemCaseSensitive(json, "levels");

  cJSON *my_level;

  if (cJSON_IsArray(levels))
  {
    my_level = cJSON_DetachItemFromArray(levels, level_id);
    if (my_level != NULL)
    {
      char *string = cJSON_Print(my_level);
      printf("%s\n", string);
      free(string);
    }
  }

  cJSON_Delete(json);
}

void playing_process_input()
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

void playing_update()
{
  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_update();
}

void playing_render()
{
  SDL_RenderClear(renderer);

  for(int i = 0; i < game_object_count; i++)
    game_objects[i].go_render(renderer, &game_objects[i]);

  SDL_RenderPresent(renderer);
}

void playing_cleanup()
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
      playing_process_input();
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
      playing_update();
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
      playing_render();
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
