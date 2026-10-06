#pragma once
struct lua_State; struct SDL_Window; struct SDL_Renderer;
void jy_register_lib(lua_State* L);
void jy_set_sdl(SDL_Window*, SDL_Renderer*);
void jy_free_pics();
