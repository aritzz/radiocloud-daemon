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
#include "daemon.h"
#include "config.h"
#include "database.h"
#include "downloader.h"
#include "uploader.h"
#include "utils.h"

int RC_STATUS = STATUS_DBCONNECT;

int main(int argc, char *argv[])
{
    pid_t pid_fork, sid;
    int loop_true = 1;
    struct config *rc_config;
    rc_config = (struct config*)malloc(sizeof(struct config));
    struct sigaction doit;
    
    
    
    read_configfile("daemon.conf", rc_config);

    if (!is_config_valid(rc_config))
    {
        printf("RadioCloud: Invalid config file, exiting.\n");
        return -1;
    } else
        printf("RadioCloud: Config file loaded.\n");
    
    if (!db_connect(rc_config)) {
        printf("Exiting daemon\n");
        return -1;
    } else 
        printf("RadioCloud: Database connected.\n");
    

    // Create radiocloud daemon
    if (create_daemon(&pid_fork, &sid) == EXIT_FAILURE)
    {
        printf("RadioCloud: Error creating daemon\n");
        return EXIT_FAILURE; 
    }
    
    if (pid_fork > 0)
    {
        printf("RadioCloud: Going to background, bye!\n");
        return EXIT_SUCCESS; // Exit main program
    }
    /* Sigterm handler */
    memset(&doit, 0, sizeof(doit));
    doit.sa_handler = sighandler;
    sigaction(SIGTERM, &doit, NULL);
    
    /* Start children loop */
    syslog (LOG_NOTICE, "RadioCloud daemon started.");
    gst_init(&argc, &argv);

    // Daemon loop
    while(loop_true)
    {
       
        switch (RC_STATUS) {
            case STATUS_DBCONNECT:
                if (db_connect(rc_config)) {
                    RC_STATUS = STATUS_DOWNLOADER;
                    db_log(rc_config, "Connected to database", "info");
                } else
                    syslog(LOG_NOTICE, "RadioCloud DB connect error, retrying...");
                break;
            case STATUS_DOWNLOADER:
                if (downloader_main(rc_config)) RC_STATUS = STATUS_UPLOADER;
                else RC_STATUS = STATUS_DBCONNECT;
                break;
            case STATUS_UPLOADER:
                if (uploader_main(rc_config)) RC_STATUS = STATUS_DOWNLOADER;
                else RC_STATUS = STATUS_DBCONNECT;
                break;
            case STATUS_ERROR:
                syslog(LOG_NOTICE, "Error status, ending loop.");
                loop_true = !loop_true;
                break;
            case STATUS_DIE:
                syslog(LOG_NOTICE, "I am going to die! :(");
                loop_true = 0;
                break;
    
        }
        
        if (RC_STATUS != STATUS_DIE)
         sleep(RC_WAIT);
        
    }
    syslog (LOG_NOTICE, "RadioCloud daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
