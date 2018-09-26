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
#include "database.h"

int weekday[7] = {6, 0, 1, 2, 3, 4, 5};




// returns 0 if ok. downloads url to path
int curl_downloader(char *path, char *url)
{
  CURLcode ret;
  CURL *hnd;
  FILE *fp;
  char agent[DEFAULT_BUFFER];
    
  sprintf(agent, "radiocloud/%d", RC_VERSION);

  hnd = curl_easy_init ();
  fp = fopen(path,"wb");
  curl_easy_setopt (hnd, CURLOPT_URL, url);
  curl_easy_setopt (hnd, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt (hnd, CURLOPT_USERAGENT, agent);
  curl_easy_setopt (hnd, CURLOPT_MAXREDIRS, 50L);
  curl_easy_setopt (hnd, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt (hnd, CURLOPT_WRITEFUNCTION, NULL);
  curl_easy_setopt (hnd, CURLOPT_WRITEDATA, fp);

  ret = curl_easy_perform (hnd);

  curl_easy_cleanup (hnd);
  hnd = NULL;
  fclose(fp);
  return (int) ret;
}

        

/* Kodeak
    0 -> Errorea
    1 -> Eguneraketa egin da
    2 -> Eguneratuta dago
*/
int plugin_downloader(struct config *rc_config, char *plugin_name, char *url, int all, char *last, char *dir, char *file, char *lastfile)
{
    char filepath[DEFAULT_BUFFER];
    char buffer[DEFAULT_BUFFER];
    char executor[DEFAULT_BUFFER];
    FILE *fp;
    char *token;
    int status;
    char retstatus[DEFAULT_BUFFER];
    
    // Get executor to use with the plugin
    sprintf(buffer, "%s/%s.exec", rc_config->pluginpath, plugin_name);
    fp = fopen(buffer, "r");
    if (!fp) return 1;
    if (!fgets(executor, sizeof (executor), fp)) return 1;
    fclose(fp);
    strtok(executor, "\n");
    // Prepare plugin to execute
    sprintf(filepath, "%s %s/%s '%s' '%d' '%s' '%s' '%s'", executor, rc_config->pluginpath, plugin_name, url, all, last, dir, file);
    syslog(LOG_NOTICE, "exec %s", filepath);
    fp = popen(filepath, "r");
    if (fp == NULL)
        return 1;

    while (fgets(buffer, DEFAULT_BUFFER, fp) != NULL);
    

    status = pclose(fp);
    
    if (status == -1) return 0;
    
    // Return code: "signal:new_file"
    token = strtok (buffer, ":");
    strcpy(retstatus, token);
    token = strtok (NULL, ":");
    strcpy(lastfile, token);

    syslog(LOG_NOTICE, "Plugin response: %s:%s", retstatus, lastfile);

    if (atoi(retstatus) == 0) {
        syslog(LOG_NOTICE, "Plugin error: %s", retstatus);
        return 0;
    } else
        return atoi(retstatus);
    
    return 0;
}





int downloader_main(struct config *rc_config) {
    MYSQL_RES *res;
    MYSQL_ROW row;
    char lastfile[DEFAULT_BUFFER];
    time_t timet;
    struct tm *todayinfo;
    char buffer[DEFAULT_BUFFER];
    char radiocore_dir[DEFAULT_BUFFER];
    char url[DEFAULT_BUFFER];
    char last[DEFAULT_BUFFER];
    char file[DEFAULT_BUFFER];
    // Get today info
    time(&timet);
    todayinfo = localtime(&timet);
    
    db_get_dir(rc_config, "radiocore_dir", radiocore_dir);
    
   // syslog(LOG_NOTICE, "Downloader working, download dir is %s", radiocore_dir);
    if (!mysql_query(rc_config->dbcon, "SELECT podcast_download.*, blocks.vars as file, blocks.desc as izen FROM podcast_download INNER JOIN blocks ON podcast_download.blockid=blocks.id")){
    res = mysql_store_result(rc_config->dbcon);

        
    while ((row = mysql_fetch_row(res)) != NULL) {
        // url row[1] 
        // dday row[2] dhour row[3]
        // all row[5]
        // last row[6]
        // file row[8]
	   strcpy(url, row[1]);
	   strcpy(last, row[6]);
       strcpy(file, row[8]);
       if (((atoi(row[2]) == weekday[todayinfo->tm_wday]) && (atoi(row[3]) == todayinfo->tm_hour)) || !strcmp(last, "FORCE"))
       { // must be downloaded

            if (plugin_downloader(rc_config, "pdownloader", url, atoi(row[5]), last, radiocore_dir, file, lastfile) == 1)
            {
		              syslog(LOG_NOTICE, "Podcast %s: %s", row[9], lastfile);
	    		      sprintf(buffer, "Processed download %s: %s", row[9], lastfile);
	    		      db_log(rc_config, buffer, "info");
                      // update last file info
                      sprintf(buffer, "UPDATE podcast_download SET last_file='%s', last_update=NOW() WHERE id=%d", lastfile, atoi(row[0]));
                      if (mysql_query(rc_config->dbcon, buffer)) return -1;
            }
            
            
        }
        
    }
    mysql_free_result(res);
    } else {
        syslog(LOG_NOTICE, "%s", mysql_error(rc_config->dbcon));
        return -1; // RECONNECT
    }
    
   // syslog(LOG_NOTICE, "Downloader stopped");

    
    return 1;
}


