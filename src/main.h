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


// Main headers
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <my_global.h>
#include <mysql.h>
#include <libxml/parser.h>
#include <curl/curl.h>
#include <gst/gst.h>
#include <glib.h>


// Default parameters
// Note: Increase RC_WAIT timeout value in slow machines
#define RC_WAIT 60
#define RC_VERSION "1.2"
#define DEFAULT_BUFFER 1024

// Main daemon status
#define STATUS_DBCONNECT    0
#define STATUS_DOWNLOADER   1
#define STATUS_UPLOADER     2
#define STATUS_ERROR        3
#define STATUS_DIE          4

extern int RC_STATUS;
extern int weekday[7];


struct config {
    /* SQL server info */
    char sqlhost[DEFAULT_BUFFER];
    char sqlport[DEFAULT_BUFFER];
    char sqluser[DEFAULT_BUFFER];
    char sqlpass[DEFAULT_BUFFER];
    char sqldb[DEFAULT_BUFFER];
    
    /* RC daemon data */
    char logfile[DEFAULT_BUFFER];
    
    /* Temp directory */
    char tempdir[DEFAULT_BUFFER];
    
    /* External plugin path */
    char pluginpath[DEFAULT_BUFFER];
    
    /* MySQL connection info */
    MYSQL *dbcon;
};
