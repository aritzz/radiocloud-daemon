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


int db_connect(struct config *);
void db_log(struct config *data, char *info, char *type);
char* db_get_dir(struct config *, char *, char*);
void db_get_config(struct config *data, char *val, char *ret);