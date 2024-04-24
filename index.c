/* 
 * Copyright (C) 2024  Austin Larsen
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <regex.h>
#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>
#include <sqlite3.h>
#include <pthread.h>

#include "render.h"

void htmlRenderCallback(file_path, file_name, linenum) 
    char *file_path; 
    char *file_name;
    uint32_t linenum;
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if (strcmp(file_name, "index.html")) return;

    Html html;
    HttpHeader header;

    htmlFile2Header_alloc(&header, "./http_headers/basic");

    htmlFile2Str_alloc(&html, file_path);

    LuaTag luaTag;
    luaTag_alloc(&luaTag, L);
    Render render;
    renderInit(&render, &luaTag, &html, &header);

    readLines(html.fp, &render, getRenderInfoCallback);


    lua_pushlightuserdata(L, &html);
    lua_setglobal(L, "html");


    lua_pushcfunction(L, l_tagToStr);
    lua_setglobal(L, "tagToStr");

    if(luaL_dostring(luaTag.state, luaTag.h_buffer) != LUA_OK) {
        fprintf(stderr, "[C] error reading lua at\n%s line %d\n",
                __FILE__, __LINE__);
    }

    lua_getglobal(luaTag.state, "var");

    /* const char *var = lua_tostring(luaTag.state, -1); */
    /* printf("var: %s\n", var); */

    /* printf("%s\n", render.h_buffer); */
    NetServer netServer;
    netInit(&netServer, PEMFILE);
    /* netAssign(&netServer, "/", "GET", "<h1>hello!<h1>"); */
    /* netAssign(&netServer, "/", "GET", render.h_buffer); */
    netAssign(&netServer, "/other", "GET", "<h1>bye</h1>");
    netAssign(&netServer, "/next", "GET", "<h1>hello then</h1>");
    netAssign(&netServer, "/", "GET", render.h_buffer);
    netAssign(&netServer, "/", "POST", "might work");

    netListen(&netServer);
    netClose(netServer);

    luaTag_dealloc(&luaTag, L);
    htmlFile2Str_dealloc(&html);
    htmlFile2Header_dealloc(&header);
    renderFree(render);
} // htmlRenderCallback()

int main(int args, char **argv)
{
    searchFileType("./public", "html", htmlRenderCallback);
    return 0;

} // main()
