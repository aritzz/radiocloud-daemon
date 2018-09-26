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

int create_daemon(pid_t *pid_fork, pid_t *sid) {
    char cwd[1024]; int i;
    // Create daemon
    *pid_fork = fork();
    
    if (*pid_fork < 0) // error
        return EXIT_FAILURE;
    
    if (*pid_fork > 0) // end main program
        return EXIT_SUCCESS;
    
    umask(027);
    
    *sid = setsid();
    if (*sid < 0)
        return EXIT_FAILURE;
    
    if (getcwd(cwd, sizeof(cwd)) == NULL)
       return EXIT_FAILURE;
    

    chdir(cwd);
    openlog ("rc-daemon", LOG_PID, LOG_DAEMON);

    /* Close descriptors and redirect to null */
    for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
    i=open("/dev/null",O_RDWR); /* open stdin */
	dup(i); /* stdout */
	dup(i); /* stderr */
    
    return EXIT_SUCCESS;
}