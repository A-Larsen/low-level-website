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
#ifndef _FILE_H_
#define _GNU_SOURCE
#define _FILE_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void readLines(fp, data, callback) 
    // SYNOPSIS: 
    //     reads lines from a file descriptor and performs job based on those
    //     lines

    FILE *fp;               /* File descriptor */
    void *data;             /* data that can be passed to callback*/
    void (*callback)        /* callback to be called for each line of file*/
        (char *line,        /* string of the current line in the loop */
         size_t len,        /* length of the current line in the loop */
         void *data,        /* passed to callback from readLines */
         uint32_t linenum);
{
    rewind(fp);
    size_t read;
    size_t len = 0;
    uint32_t linenum = 0;

    while(1) {
         char *line = NULL;
         read = getline(&line, &len, fp);
         if (read == (size_t)-1) {free(line); return;}
         linenum++;
         callback(line, read, data, linenum);
         free(line);
    }
} // readLines()

#endif // _FILE_H_
