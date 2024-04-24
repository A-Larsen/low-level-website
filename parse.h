#ifndef _PARSE_H_
#define _PARSE_H_
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>
#include <regex.h>

#include "file.h"

#define STARTTAG "<lua>"
#define ENDTAG "</lua>"

typedef struct _HeaderKV
  // ALLOCATE_FUNCTION:     htmlFile2Header_alloc
  // DEALLOCATED_FUNCTION:  htmlFile2Header_dealloc
  //
  // Key value pairs for the http header.
{
    char key[255];
    char value[255];
} 
HeaderKV;

typedef struct _LuaTag 
  // ALLOCATE_FUNCTION:     luaTag_alloc
  // DEALLOCATED_FUNCTION:  luaTag_dealloc
{
    lua_State *state;
    bool in_tag;
    /* char *h_buffer; */
    /* uint32_t content_len; */
    char h_buffer[2048];
    uint32_t lines_count;
} 
LuaTag;

typedef struct _HttpHeader 
  // ALLOCATE_FUNCTION:     htmlFile2Header_alloc
  // DEALLOCATED_FUNCTION:  htmlFile2Header_dealloc
  //
  // The values attribute in @h_parsed are not parsed, just the top level key
  // value pairs 
{
    FILE *fp;
    char h_buffer[2048];       /* heap allocated string for the header */
    HeaderKV *h_parsed;   /* heap allocated key value pairs for the header*/
    uint16_t parsed_size; /* size of the @h_parsed attribute */
    char protocol[255];
} 

HttpHeader;

typedef struct _Tag
  // ALLOCATE_FUNCTION:
  // DEALLOCATED_FUNCTION:
{
    char name[10];
    uint32_t startline;
    uint32_t endline;
    regmatch_t start;
    regmatch_t end;
} 
Tag;

typedef struct _Html 
  // ALLOCATE_FUNCTION:     htmlFile2Str_alloc
  // DEALLOCATED_FUNCTION:  htmlFile2Str_dealloc
{
    FILE *fp;
    HttpHeader header;    /* header info for the http */
    char *path;           /* the path of the file provided in the path 
                           * argument */
    char **h_lines;
    uint32_t lines_count;
    uint32_t lines_size;
    Tag *h_tags;          /* heap allocated tags, that specify that name of that
                           * tag along with their starting and ending locations */
    uint16_t tag_count;   /* number of h_tags */
}
Html;

void allocHeaderKV(out_header, line, len)
    // SYNOPSIS: 
    //     allocates the key value pairs for the http header

    HttpHeader *out_header; /*HttpHeader struct */
    char *line;             /*current line of header file */
    uint16_t len;           /*length of the current len */
{
    size_t nlen = strlen(line) -2;
     for (int i = 0; i < nlen; ++i) {
         if (line[i] == ':') {
             HeaderKV *headerkv = out_header->h_parsed;
             memcpy(headerkv[out_header->parsed_size - 1].key, line, i + 1);
             headerkv[out_header->parsed_size - 1].key[i] = '\0';

             size_t value_size = nlen - i - 1;
             memcpy(headerkv[out_header->parsed_size - 1].value, line + i + 1,
                    value_size);
             return;
         }
     }
} //allocHeaderKV()

void allocHeaderCallback(line, len, data, linenum)
    char *line;
    size_t len;
    void *data;
    uint32_t linenum;
{
    HttpHeader *out_header = (HttpHeader *) data;
    if (linenum == 1) {
        memcpy(out_header->protocol, line, strlen(line) - 2);
        return;
    };

     out_header->parsed_size++;

     if (out_header->parsed_size == 1) {
         out_header->h_parsed = (HeaderKV *)malloc(sizeof(HeaderKV) *
                                (out_header->parsed_size));
     } else {
         out_header->h_parsed = (HeaderKV *)realloc(out_header->h_parsed,
                                sizeof(HeaderKV) * 
                                (out_header->parsed_size));
     }

     allocHeaderKV(out_header, line, len);
} // allocHeaderCallback()

void htmlFile2Header_alloc(out_header, header_path)
    // SYNOPSIS: 
    //     Gets info from a http header file to a @out_header stgruct

    HttpHeader *out_header;
    char *header_path;
{
    out_header->fp = fopen(header_path, "r");
    out_header->parsed_size = 0;
    readLines(out_header->fp, out_header, allocHeaderCallback);
} // htmlFile2Header_alloc()

void htmlFile2Header_dealloc(header)
    HttpHeader *header;
{
    free(header->h_parsed);
    fclose(header->fp);
} // htmlFile2Header_dealloc()

bool match(char *comp, char *str, regmatch_t *pmatch)
    // SYNOPSIS: 
    //     Sees if str matches the comp regex. if it does it returns true and
    //     stores the locat
{
    regex_t regex;
	regmatch_t lpmatch[1];
    int reti = regcomp(&regex, comp, 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex at\n%s line %d\n", __FILE__, 
                __LINE__);
    }
    bool match = (regexec(&regex, str, sizeof(lpmatch) / sizeof(lpmatch[0]), 
                  lpmatch, 0) == 0);
    if (pmatch) {
        memcpy(&pmatch->rm_so, &lpmatch[0].rm_so, sizeof(lpmatch[0].rm_so));
        memcpy(&pmatch->rm_eo, &lpmatch[0].rm_eo, sizeof(lpmatch[0].rm_eo));
    }
    regfree(&regex);
    return match;
} // match()

void getHtmlInfoCallback(line, len, data, linenum)
    char *line;
    size_t len;
    void *data;
    uint32_t linenum;
{
    Html *out_html = (Html *)data;
    char startTagReg[] = "<[a-z0-9]*>";
    char endTagReg[] = "</[a-z0-9]*>";
    regmatch_t pmatch;
    out_html->lines_count++;
    uint32_t rlen = strlen(line);
    out_html->lines_size += rlen;


    int j = out_html->lines_count - 1;

    if (out_html->lines_count == 1) {
        out_html->h_lines = (char **)malloc(out_html->lines_size);
        out_html->h_lines[j] = (char *)malloc(rlen);
    } else {
        out_html->h_lines = (char **)realloc(out_html->h_lines,
                                             out_html->lines_size);
        out_html->h_lines[j] = (char *)malloc(rlen);
    }

    memcpy(out_html->h_lines[j], line, rlen);

    int i = out_html->tag_count - 1;

    if (match(startTagReg, line, &pmatch)) {
        uint16_t matchlen = pmatch.rm_eo - pmatch.rm_so;

        out_html->tag_count++;
        i = out_html->tag_count - 1;

        if (out_html->tag_count == 1) {
            out_html->h_tags = malloc(sizeof(Tag) * out_html->tag_count);
        } else {
            out_html->h_tags = realloc(out_html->h_tags, sizeof(Tag) *
                                       out_html->tag_count);
        }

        out_html->h_tags[i].startline = j;

        char name[50];

        memset(name, 0, 10);
        memset(out_html->h_tags[i].name, 0, 10);
        memcpy(out_html->h_tags[i].name, 
               line + pmatch.rm_so + 1, matchlen - 2);

        memcpy(&out_html->h_tags[i].start, &pmatch, 
               sizeof(regmatch_t));
    }

    if (match(endTagReg, line, &pmatch)) {
        uint16_t matchlen = pmatch.rm_eo - pmatch.rm_so;
        char name[50];
        memset(name, 0, 10);
        memcpy(name, line + pmatch.rm_so + 2, matchlen - 3);
        
        for (int k = 0; k < out_html->tag_count; ++k) {
            if (!strcmp(out_html->h_tags[k].name, name)) {
                out_html->h_tags[k].endline = j;
                memcpy(&out_html->h_tags[k].end, &pmatch, 
                       sizeof(regmatch_t));
            }
        }
    }

} // getHtmlInfoCallback()


void htmlFile2Str_alloc (out_html, html_path) 
    // SYNOPSIS: 
    //     Gets info from a html file.
    //
    //  It gets the html_path from @html_path, reads the file, and then places
    //  information about the file into the @out_html struct
      
    Html *out_html;     /* struct the gets allocated with information from the
                         * html file */
    char *html_path;    /* path of the html file */
{
    out_html->tag_count = 0;
    out_html->lines_count = 0;
    out_html->lines_size = 0;
    out_html->h_tags = NULL;
    out_html->fp = fopen(html_path, "r");
    readLines(out_html->fp, (void *)out_html, getHtmlInfoCallback);
    rewind(out_html->fp);
} // htmlFile2Str_alloc()

void htmlFile2Str_dealloc (out_html) 
    Html *out_html; /* struct that gets its contents deallocated*/
{
    if (out_html->h_tags) free(out_html->h_tags);

    for (uint32_t i = 0; i < out_html->lines_count; ++i) {
        if (out_html->h_lines[i])
            free(out_html->h_lines[i]);
    }

    if (out_html->h_lines)
        free(out_html->h_lines);

    fclose(out_html->fp);
} // htmlFile2Str_dealloc()


void searchFileType(search_path, filetype, callback)
    // SYNOPSIS: 
    //     SearchsByFileType and then performs job based on the search

    char *search_path;     /* path to search for files */
    char *filetype;        /* type of file to search for  ex. html, js, css*/
    void (*callback)       /* job to be done once the filetype is found */
        (char *file_path,  /* full path for the file found in the loop */
        char *file_name);  /* file name for the file found in the loop */
    
{
    regex_t regex;
    struct dirent *files;

    DIR *dir = opendir(search_path);

    if (!dir) {
        fprintf(stderr, "cannot open public directory at\n%s line %d\n", 
                __FILE__, __LINE__);
        return;
    }

    char type[255] = ".";
    strcat(type, filetype);
    strcat(type, "$");

    int reti = regcomp(&regex, type, 0);
    if (reti) {
        fprintf(stderr, "Could not compile regex at\n%s line %d\n", __FILE__, 
                __LINE__);
    }

    while ((files = readdir(dir)) != NULL) {
        reti = regexec(&regex, files->d_name, 0, NULL, 0);
        if (!reti) {
            char file_path[255] = "./public/";
            strcat(file_path, files->d_name);
            callback(file_path, files->d_name);
        }
    }

    regfree(&regex);
    closedir(dir);
} // searchFileType()


void luaTag_alloc(LuaTag *luaTag, lua_State *L)
{
    memset(luaTag, 0, sizeof(LuaTag));
    luaTag->in_tag = false;
    luaTag->state = L;
    luaTag->lines_count = 0;
} // luaTag_alloc()

void luaTag_dealloc(LuaTag *luaTag, lua_State *L)
{
    lua_close(L);
} // luaTag_dealloc()

int l_tagToStr(L)
    lua_State *L;
{
    // because everything is pushed onto the stack
    // -1 if second argumnet
    // -2 is first argument
    const bool isInner = lua_toboolean(L, -1);
    const char *tagname = lua_tostring(L, -2);
    lua_getglobal(L, "html");
    Html *html = (Html *)lua_touserdata(L, -1);

    uint8_t outlen = 0;
    char *out = NULL;

    bool startcontent = false;
    for (uint32_t i = 0; i < html->lines_count; ++i) {
        char *line = html->h_lines[i];

        for (int j = 0; j < html->tag_count; ++j) {
            char *name = html->h_tags[j].name;
            if (!strcmp(tagname, name)) {

                if (html->h_tags[j].startline == i) {
                    startcontent = true;
                    if (isInner) continue;
                }
                if (html->h_tags[j].endline == i) {
                    if (isInner) {
                        startcontent = false;
                        break;
                    };
                }

                if (startcontent) {
                    uint16_t len = strlen(line);
                    outlen += len;

                    if (!out) {
                        out = malloc(len);
                    } else {
                        out = realloc(out, outlen);
                    }

                    memcpy(out + outlen - len, line, len);
                }

                if (html->h_tags[j].endline == i) {
                    startcontent = false;
                }
            }
        }
    }
    lua_pushstring(L, out);
    free(out);
    return 1;
} // l_tagToStr()
  //

#endif // _PARSE_H_
