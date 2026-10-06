#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include "jycompat.h"

static SDL_Window* g_win=nullptr; static SDL_Renderer* g_ren=nullptr;
void jy_set_sdl(SDL_Window*w, SDL_Renderer*r){g_win=w;g_ren=r;}
struct PicBank { std::string idxPath, grpPath; std::vector<uint32_t> ends; std::vector<unsigned char> grp; std::unordered_map<int,SDL_Texture*> tex; std::unordered_map<int,SDL_Point> wh; };
static std::unordered_map<int,std::unique_ptr<PicBank>> banks;
static bool readAll(const char* path,std::vector<unsigned char>& out){FILE*f=fopen(path,"rb");if(!f)return false;fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);if(n<0){fclose(f);return false;}out.resize((size_t)n);bool ok=n==0||fread(out.data(),1,(size_t)n,f)==(size_t)n;fclose(f);return ok;}
static SDL_Texture* getPic(PicBank&b,int pic){if(pic<0||pic>=(int)b.ends.size())return nullptr;auto it=b.tex.find(pic);if(it!=b.tex.end())return it->second;uint32_t st=pic?b.ends[pic-1]:0,en=b.ends[pic];if(en<=st||en>b.grp.size())return nullptr;const unsigned char*p=b.grp.data()+st;size_t n=en-st; // 4.0 stores PNG payloads in GRP; tolerate padding before PNG
 size_t off=0;const unsigned char sig[8]={137,80,78,71,13,10,26,10};while(off+8<=n&&memcmp(p+off,sig,8)!=0)++off;if(off+8>n)return nullptr;SDL_RWops*rw=SDL_RWFromConstMem(p+off,(int)(n-off));if(!rw)return nullptr;SDL_Surface*s=IMG_Load_RW(rw,1);if(!s)return nullptr;SDL_Texture*t=SDL_CreateTextureFromSurface(g_ren,s);if(t){SDL_SetTextureBlendMode(t,SDL_BLENDMODE_BLEND);b.wh[pic]=SDL_Point{s->w,s->h};b.tex[pic]=t;}SDL_FreeSurface(s);return t;}
static int decodePicArg(lua_Integer v){return v>=0?(int)(v/2):(int)v;}
static int nret0(lua_State*){return 0;}
static int l_Debug(lua_State*L){const char*s=lua_tostring(L,1);if(s)std::fprintf(stderr,"[lua] %s\n",s);return 0;}
static int l_GetTime(lua_State*L){lua_pushinteger(L,(lua_Integer)SDL_GetTicks());return 1;}
static int l_Delay(lua_State*L){SDL_Delay((Uint32)luaL_optinteger(L,1,0));return 0;}
static int l_ShowSurface(lua_State*){if(g_ren)SDL_RenderPresent(g_ren);return 0;}
static int l_FillColor(lua_State*L){if(!g_ren)return 0;int x1=luaL_optinteger(L,1,0),y1=luaL_optinteger(L,2,0),x2=luaL_optinteger(L,3,0),y2=luaL_optinteger(L,4,0);Uint32 c=(Uint32)luaL_optinteger(L,5,0);SDL_SetRenderDrawColor(g_ren,(c>>16)&255,(c>>8)&255,c&255,255);if(x1==0&&y1==0&&x2==0&&y2==0)SDL_RenderClear(g_ren);else{SDL_Rect rc{x1,y1,x2-x1,y2-y1};SDL_RenderFillRect(g_ren,&rc);}return 0;}
static int l_DrawRect(lua_State*L){if(!g_ren)return 0;int x1=luaL_checkinteger(L,1),y1=luaL_checkinteger(L,2),x2=luaL_checkinteger(L,3),y2=luaL_checkinteger(L,4);Uint32 c=(Uint32)luaL_optinteger(L,5,0xffffff);SDL_SetRenderDrawColor(g_ren,(c>>16)&255,(c>>8)&255,c&255,255);SDL_Rect rc{x1,y1,x2-x1+1,y2-y1+1};SDL_RenderDrawRect(g_ren,&rc);return 0;}
static int l_SetClip(lua_State*L){if(!g_ren)return 0;int x1=luaL_optinteger(L,1,0),y1=luaL_optinteger(L,2,0),x2=luaL_optinteger(L,3,0),y2=luaL_optinteger(L,4,0);if(x1==0&&y1==0&&x2==0&&y2==0)SDL_RenderSetClipRect(g_ren,nullptr);else{SDL_Rect r{x1,y1,x2-x1,y2-y1};SDL_RenderSetClipRect(g_ren,&r);}return 0;}
static int l_GetKey(lua_State*L){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT){lua_pushinteger(L,27);return 1;}if(e.type==SDL_CONTROLLERBUTTONDOWN||e.type==SDL_KEYDOWN){int k=0;if(e.type==SDL_CONTROLLERBUTTONDOWN){switch(e.cbutton.button){case SDL_CONTROLLER_BUTTON_A:k=13;break;case SDL_CONTROLLER_BUTTON_B:k=27;break;case SDL_CONTROLLER_BUTTON_X:k=32;break;case SDL_CONTROLLER_BUTTON_Y:k='y';break;case SDL_CONTROLLER_BUTTON_DPAD_UP:k=273;break;case SDL_CONTROLLER_BUTTON_DPAD_DOWN:k=274;break;case SDL_CONTROLLER_BUTTON_DPAD_LEFT:k=276;break;case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:k=275;break;default:k=0;}}else k=e.key.keysym.sym;lua_pushinteger(L,k);return 1;}}lua_pushinteger(L,0);return 1;}
static int l_GetMouse(lua_State*L){lua_pushinteger(L,0);lua_pushinteger(L,0);lua_pushinteger(L,0);return 3;}
static int l_PicLoadFile(lua_State*L){const char*idx=luaL_checkstring(L,1);const char*grp=luaL_checkstring(L,2);int id=(int)luaL_checkinteger(L,3);std::vector<unsigned char> ib,gb;if(!readAll(idx,ib)||!readAll(grp,gb)||ib.size()%4){std::fprintf(stderr,"PicLoadFile failed: %s %s\n",idx,grp);lua_pushinteger(L,-1);return 1;}auto b=std::unique_ptr<PicBank>(new PicBank);b->idxPath=idx;b->grpPath=grp;b->grp.swap(gb);b->ends.resize(ib.size()/4);for(size_t i=0;i<b->ends.size();++i)b->ends[i]=(uint32_t)ib[i*4]|((uint32_t)ib[i*4+1]<<8)|((uint32_t)ib[i*4+2]<<16)|((uint32_t)ib[i*4+3]<<24);banks[id]=std::move(b);std::fprintf(stderr,"Pic bank %d: %zu entries\n",id,banks[id]->ends.size());lua_pushinteger(L,(lua_Integer)banks[id]->ends.size());return 1;}
static int l_PicLoadCache(lua_State*L){int id=(int)luaL_checkinteger(L,1),pic=decodePicArg(luaL_checkinteger(L,2)),x=(int)luaL_checkinteger(L,3),y=(int)luaL_checkinteger(L,4);auto bi=banks.find(id);if(bi==banks.end()||!g_ren)return 0;SDL_Texture*t=getPic(*bi->second,pic);if(!t)return 0;int w=0,h=0;SDL_QueryTexture(t,nullptr,nullptr,&w,&h);SDL_Rect d{x,y,w,h};SDL_RenderCopy(g_ren,t,nullptr,&d);return 0;}
static int l_PicGetXY(lua_State*L){int id=(int)luaL_checkinteger(L,1),pic=decodePicArg(luaL_checkinteger(L,2));auto bi=banks.find(id);if(bi==banks.end()){lua_pushinteger(L,0);lua_pushinteger(L,0);return 2;}SDL_Texture*t=getPic(*bi->second,pic);int w=0,h=0;if(t)SDL_QueryTexture(t,nullptr,nullptr,&w,&h);lua_pushinteger(L,w);lua_pushinteger(L,h);return 2;}
static int l_LoadConfig(lua_State*L){lua_pushinteger(L,0);return 1;}static int l_KoSaveIO(lua_State*L){lua_pushinteger(L,0);return 1;}
static const luaL_Reg funcs[]={{"Debug",l_Debug},{"GetTime",l_GetTime},{"Delay",l_Delay},{"ShowSurface",l_ShowSurface},{"FillColor",l_FillColor},{"DrawRect",l_DrawRect},{"SetClip",l_SetClip},{"GetKey",l_GetKey},{"GetMouse",l_GetMouse},{"PicGetXY",l_PicGetXY},{"PicLoadFile",l_PicLoadFile},{"PicLoadCache",l_PicLoadCache},{"LoadConfig",l_LoadConfig},{"KoSaveIO",l_KoSaveIO},{"Background",nret0},{"CharSet",nret0},{"EnableKeyRepeat",nret0},{"FreeSur",nret0},{"LoadSoundConfig",nret0},{"LoadSur",nret0},{"PlayMIDI",nret0},{"PlayWAV",nret0},{"SaveSur",nret0},{"ShowSlow",nret0},{"DrawStr",nret0},{nullptr,nullptr}};
void jy_register_lib(lua_State*L){lua_newtable(L);for(const luaL_Reg*p=funcs;p->name;++p){lua_pushcfunction(L,p->func);lua_setfield(L,-2,p->name);}lua_setglobal(L,"lib");}
void jy_free_pics(){for(auto&kv:banks)for(auto&t:kv.second->tex)if(t.second)SDL_DestroyTexture(t.second);banks.clear();}
