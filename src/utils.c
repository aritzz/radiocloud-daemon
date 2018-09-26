/*
    RadioCloud daemon - Part of RadioCloud automation system
    Copyright (C) 2018 - Aritz Olea Zubikarai <aritzolea@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
*/

#include "main.h"

// Random generator from https://www.codeproject.com/Questions/640193/Random-string-in-language-C
void gen_random(char *s, const int len) {
    time_t t;
    srand((unsigned) time(&t)); // init random engine with localtime
    
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < len; ++i) 
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    s[len] = 0;
}



// Copies a file to another one
// Returns 1 if error
int copy_file(char *fromfile, char  *tofile)
{
    FILE *from, *to;
    int ch;
    
    from = fopen(fromfile, "rb");
    if (!from) return 1;
    
    to = fopen(tofile, "wb");
    if (!to) {
        fclose(from);
        return 1;
    }
    
    while (1) {
        ch = fgetc(from);
        if (feof(from)) 
            break;
        
        fputc(ch, to);    
    }
    fclose(from);
    fclose(to);
    
    return 0;
}

char *replace_str(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

void sighandler(int signum)
{
   syslog(LOG_NOTICE, "Signal received: %d\n", signum);
   RC_STATUS = STATUS_DIE;
}

// base64_encode(const void *src, size_t src_len, char *dst) from mysql.h
