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

void read_configfile(char *filename, struct config *cfg)
{
//    struct config cfg;
    
    FILE *file = fopen (filename, "r");

    if (file != NULL)
    { 
        char line[DEFAULT_BUFFER];
        char param1[DEFAULT_BUFFER], param2[DEFAULT_BUFFER];
        while(!feof(file))
        {
            fgets(line, DEFAULT_BUFFER, file);
            if (line[0] == '#' || line[0] == ';')
                continue;
            sscanf(line, "%s %s", param1, param2);
            
            if (!strcmp(param1, "SQLHost"))
                strcpy(cfg->sqlhost, param2);
            else if (!strcmp(param1, "SQLPort"))
                strcpy(cfg->sqlport, param2);
            else if (!strcmp(param1, "SQLUser"))
                strcpy(cfg->sqluser, param2);
            else if (!strcmp(param1, "SQLPass"))
                strcpy(cfg->sqlpass, param2);
            else if (!strcmp(param1, "SQLDB"))
                strcpy(cfg->sqldb, param2);
            else if (!strcmp(param1, "LogFile"))
                strcpy(cfg->logfile, param2);
            else if (!strcmp(param1, "TempDir"))
                strcpy(cfg->tempdir, param2);
            else if (!strcmp(param1, "PluginPath"))
                strcpy(cfg->pluginpath, param2);
        }
        fclose(file);
    } 
    
    
    
}

int is_config_valid(struct config *cfg)
{
    if (cfg->sqlhost[0] == '\0' || cfg->sqlport[0] == '\0' || cfg->sqluser[0] == '\0' || cfg->sqlpass[0] == '\0' || cfg->sqldb[0] == '\0' || cfg->logfile[0] == '\0')
        return 0;
    
    return 1;
}
