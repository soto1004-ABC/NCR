#include <switch.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <unistd.h>
#include <cstdio>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>}
#include "jycompat.h"
extern void jy_set_sdl(SDL_Window*,SDL_Renderer*);
static int run(lua_State*L,const char*f){if(luaL_dofile(L,f)){std::fprintf(stderr,"Lua error: %s\n",lua_tostring(L,-1));return -1;}return 0;}
int main(int,char**){
 romfsInit(); chdir("romfs:/game");
 SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER); IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG); TTF_Init(); Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,1024);
 SDL_Window*w=SDL_CreateWindow("Yingjie Remake 4.0",0,0,1280,720,SDL_WINDOW_SHOWN); SDL_Renderer*r=w?SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC):nullptr; jy_set_sdl(w,r);
 lua_State*L=luaL_newstate(); luaL_openlibs(L); jy_register_lib(L);
 int rc=run(L,"config_switch.lua"); if(!rc) rc=run(L,"script/jymain.lua"); if(!rc){lua_getglobal(L,"JY_Main"); if(lua_isfunction(L,-1)&&lua_pcall(L,0,0,0)!=0){std::fprintf(stderr,"JY_Main: %s\n",lua_tostring(L,-1));rc=-1;}}
 while(rc && appletMainLoop()){SDL_Event e;while(SDL_PollEvent(&e))if(e.type==SDL_QUIT)goto out; SDL_SetRenderDrawColor(r,80,0,0,255);SDL_RenderClear(r);SDL_RenderPresent(r);SDL_Delay(16);} out:
 jy_free_pics(); lua_close(L); if(r)SDL_DestroyRenderer(r);if(w)SDL_DestroyWindow(w);Mix_CloseAudio();TTF_Quit();IMG_Quit();SDL_Quit();romfsExit();return rc?1:0;
}
