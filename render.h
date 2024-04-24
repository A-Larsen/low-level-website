#ifndef _RENDER_H_
#define _RENDER_H_

#define _GNU_SOURCE

#include "config.h"
#include "net.h"
#include "parse.h"

typedef struct _Render 
{
    char h_buffer[4096];
    LuaTag *luaTag;
    Html *html;
} 
Render;

void getRenderInfoCallback(line, len, data, linenum)
    char *line;
    size_t len;
    void *data;
    uint32_t linenum;
{
    Render *render = (Render *)data;
    size_t rlen = strlen(line) + 1;

    if (strstr(line, STARTTAG)) {
        render->luaTag->in_tag = true;
        return;
    }

    if (strstr(line, ENDTAG)) {
        render->luaTag->in_tag = false;
        return;
    }

    if (render->luaTag->in_tag) {
        strncat(render->luaTag->h_buffer, line, rlen);
        render->luaTag->lines_count++;

    } else {
        strncat(render->h_buffer, line, rlen + 1);
    }
} // getRenderInfoCallback()

void renderInit(render, luaTag, html, header)
    Render *render;
    LuaTag *luaTag;
    Html *html;
    HttpHeader *header;
{
    render->luaTag = luaTag;
    render->html = html;

    char proto[500];
    sprintf(proto, "%s\r\n", header->protocol);
    int plen = strlen(proto);
    strncat(render->h_buffer, proto, plen);
    /* printf("%s\n",  header->protocol); */


    for (int i = 0; i < header->parsed_size; ++i) {
        char kv[512];
        memset(kv, 0, 512);
        sprintf(kv, "%s:%s\r\n", header->h_parsed[i].key,
                header->h_parsed[i].value);

        int kvlen = strlen(kv);

        strncat(render->h_buffer, kv, kvlen);
    }

    strncat(render->h_buffer, "\r\n", 3);
}


void renderFree(render)
    Render render;
{
}



#endif // _RENDER_H_
