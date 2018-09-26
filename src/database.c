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

int db_connect(struct config *data) {
  data->dbcon = mysql_init(NULL);

  if (data->dbcon == NULL)
  {
      fprintf(stderr, "RadioCloud: MySQL Init error: %s\n", mysql_error(data->dbcon));
      return 0;
  }

  if (mysql_real_connect(data->dbcon, data->sqlhost, data->sqluser, data->sqlpass,
          data->sqldb, atoi(data->sqlport), NULL, 0) == NULL)
  {
      fprintf(stderr, "RadioCloud: MySQL Connect error: %s\n", mysql_error(data->dbcon));
      mysql_close(data->dbcon);
      return 0;
  }

  return 1;
}

void db_disconnect(struct config *data) {
  printf("RadioCloud: Closing database\n");
  mysql_close(data->dbcon);
}

void db_log(struct config *data, char *info, char *type) {
  char dbquery[800];
  sprintf(dbquery, "INSERT INTO log (id, date, type, data) VALUES (NULL, NOW(), '%s', '[RC-daemon] %s')", type, info);
  if (mysql_query(data->dbcon, dbquery)) {
      fprintf(stderr, "[RC-daemon] LOG error: %s\n", mysql_error(data->dbcon));
  }
}


void db_get_dir(struct config *data, char *val, char *ret)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char val_filtered[DEFAULT_BUFFER];
    char query[DEFAULT_BUFFER];

    mysql_real_escape_string(data->dbcon, val_filtered, val, strlen(val));
    sprintf(query, "SELECT dirpath FROM dirs WHERE dirname='%s'", val_filtered);
    
  if (!mysql_query(data->dbcon, query)) {
    res = mysql_store_result(data->dbcon);
    if ((row = mysql_fetch_row(res)) != NULL) 
      strcpy(ret, row[0]);  
    mysql_free_result(res);
  } else
      strcpy(ret, "");
}

void db_get_config(struct config *data, char *val, char *ret)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char val_filtered[DEFAULT_BUFFER];
    char query[DEFAULT_BUFFER];

    mysql_real_escape_string(data->dbcon, val_filtered, val, strlen(val));
    sprintf(query, "SELECT value FROM config WHERE var='%s'", val_filtered);
    
  if (!mysql_query(data->dbcon, query)) {
    res = mysql_store_result(data->dbcon);
    if ((row = mysql_fetch_row(res)) != NULL) 
      strcpy(ret, row[0]);  
    mysql_free_result(res);
  } else
      strcpy(ret, "");
}