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
#include "encoder.h"
#include "utils.h"

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    syslog(LOG_NOTICE, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}


int external_uploader(struct config *rc_config, MYSQL_ROW row, char *encoded_file, char *retfile)
{
    CURL *curl;
    CURLcode res;
    char url_external[DEFAULT_BUFFER];
    char extension[DEFAULT_BUFFER];
    int ret = 0;
    
    db_get_dir(rc_config, "external_upload", url_external);

    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";

    curl_global_init(CURL_GLOBAL_ALL);

    curl = curl_easy_init();
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;

    encoder_get_type(row[9], extension);
    
    if(curl) {
        struct string s;
        init_string(&s);
        /* Create the form */
        form = curl_mime_init(curl);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        /* Fill in the file upload field */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, encoded_file);

        /* Add all fields */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "user");
        curl_mime_data(field, row[14], CURL_ZERO_TERMINATED);

        field = curl_mime_addpart(form);
        curl_mime_name(field, "date");
        curl_mime_data(field, row[4], CURL_ZERO_TERMINATED);
        
        field = curl_mime_addpart(form);
        curl_mime_name(field, "type");
        curl_mime_data(field, extension, CURL_ZERO_TERMINATED);

        field = curl_mime_addpart(form);
        curl_mime_name(field, "title");
        curl_mime_data(field, row[2], CURL_ZERO_TERMINATED);       
       
        field = curl_mime_addpart(form);
        curl_mime_name(field, "text");
        curl_mime_data(field, row[3], CURL_ZERO_TERMINATED);     
    
        field = curl_mime_addpart(form);
        curl_mime_name(field, "name");
        curl_mime_data(field, row[19], CURL_ZERO_TERMINATED);     
        
        
        /* Fill in the submit field too, even if this is rarely needed */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "submit");
        curl_mime_data(field, "send", CURL_ZERO_TERMINATED);

        /* initialize custom header list (stating that Expect: 100-continue is not
        wanted */
        headerlist = curl_slist_append(headerlist, buf);
        /* what URL that receives this POST */
        curl_easy_setopt(curl, CURLOPT_URL, url_external);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK) {
            syslog(LOG_NOTICE, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            ret = 1;
        }

        
        if (!strcmp(s.ptr, "")) // NO OK
        {
            db_log(rc_config, "External upload wrong response - NO OK (!=1)", "error");
            syslog(LOG_NOTICE, "Wrong response from external server: %s", s.ptr);
            ret = 1;
        }
        strcpy(retfile, s.ptr);
        /* always cleanup */
        curl_easy_cleanup(curl);

        /* then cleanup the form */
        curl_mime_free(form);
        /* free slist */
        curl_slist_free_all(headerlist);
    }
    return ret;
}


        

// returns 1 if error
int plugin_uploader(struct config *rc_config, char *plugin_name, char *encoded_file, char *podcast_id, char *extra_params)
{
    char filepath[DEFAULT_BUFFER];
    char buffer[DEFAULT_BUFFER];
    char executor[DEFAULT_BUFFER];
    FILE *fp;
    int status;
    
    // Get executor to use with the plugin
    sprintf(buffer, "%s/%s.exec", rc_config->pluginpath, plugin_name);
    fp = fopen(buffer, "r");
    if (!fp) return 1;
    if (!fgets(executor, sizeof (executor), fp)) return 1;
    fclose(fp);
    strtok(executor, "\n");
    // Prepare plugin to execute
    sprintf(filepath, "%s %s/%s '%s' '%s' '%s' '%s' '%s' '%s' '%s' '%s'", executor, rc_config->pluginpath, plugin_name, rc_config->sqlhost, rc_config->sqlport, rc_config->sqluser, rc_config->sqlpass, rc_config->sqldb, encoded_file, podcast_id, extra_params);
    syslog(LOG_NOTICE, "exec %s", filepath);
    fp = popen(filepath, "r");
    if (fp == NULL)
        return 1;

    while (fgets(buffer, 1024, fp) != NULL);
    

    status = pclose(fp);
    
    if (status == -1) return 1;
    
    if (atoi(buffer) != 1) {
        syslog(LOG_NOTICE, "Plugin error: %s", buffer);
        return 1;
    }
    
    return 0;
}


int uploader_main(struct config *rc_config) 
{
    char buffer[DEFAULT_BUFFER];
    char encoded_file[DEFAULT_BUFFER];
    char return_file[DEFAULT_BUFFER];
    char random[DEFAULT_BUFFER];
    char radioname[DEFAULT_BUFFER];
    char dir_radiocloud[DEFAULT_BUFFER];
    char dir_repeat[DEFAULT_BUFFER];
    char dir_upload[DEFAULT_BUFFER];
    char dir_upload_temp[DEFAULT_BUFFER];
    int status = 0;
    char filebuffer[DEFAULT_BUFFER];
    
    MYSQL_RES *res;
    MYSQL_ROW row;
    
    gen_random(random, 10);
    sprintf(encoded_file, "%s/%s", rc_config->tempdir, random);
    
   // syslog(LOG_NOTICE, "Starting uploader");
    
    /* Get directories and configuration */
    db_get_dir(rc_config, "radiocloud_dir", dir_radiocloud);
    db_get_dir(rc_config, "podcast_upload", dir_upload);

    db_get_dir(rc_config, "radiocore_dir", dir_repeat);
    db_get_config(rc_config, "radioname", radioname);
    
    //copy_file("/home/aritz/radiocore/probak.mp3", "/home/aritz/radiocore/probak_kopi.mp3");
    
    if (!mysql_query(rc_config->dbcon, "SELECT podcast_upload.*, users.username as username, users.image as image, programs.arrosa_user as arrosa_user, programs.arrosa_pass as arrosa_pass, programs.arrosa_category as arrosa_category, programs.name as progname, blocks.vars as destfile FROM ((podcast_upload INNER JOIN users ON users.id=podcast_upload.userid) INNER JOIN programs ON programs.id=users.programid) INNER JOIN blocks ON programs.blockid=blocks.id WHERE podcast_upload.is_trash=0 AND users.enabled=1 AND podcast_upload.uploaded=0 AND DATE(podcast_upload.date) <= CURDATE() ORDER BY podcast_upload.id ASC limit 5")){
    res = mysql_store_result(rc_config->dbcon);
//    db_log(rc_config, "Processing uploads", "info");
    while ((row = mysql_fetch_row(res)) != NULL) {
        /*
        id row[0]
        title row[2]
        text row[3]
        date row[4]
        repeat row[5]
        podcast row[6]
        arrosa row[7]
        tempfile row[9]
        uploaded row[10]
        repeat_added row[12]
        username row[14]
        image row[15]
        arrosau row[16]
        arrosap row[17]
        arrosac row[18]
        progname row[19]
        destfile row[20]
        */
        status = 0;
        /* Where is the file? */
        sprintf(filebuffer, "%s/%s/%s", dir_radiocloud, dir_upload, row[9]);
        syslog(LOG_NOTICE, "dir is %s", filebuffer);

        sprintf(buffer, "UPDATE podcast_upload SET uploaded=2 WHERE id=%s", row[0]); // status 2 for processing files
//        syslog(LOG_NOTICE, "upload %s", buffer);
        mysql_query(rc_config->dbcon, buffer);

 	sprintf(buffer, "Processing upload for %s", row[19]);
        db_log(rc_config, buffer, "info");

        /* Encode file */
        if (encode_file(rc_config, radioname, row[19], filebuffer, encoded_file) == -1) // error
	{
		db_log(rc_config, "Encoding error", "error");
		sprintf(buffer, "UPDATE podcast_upload SET uploaded=0 WHERE id=%s", row[0]);
        	mysql_query(rc_config->dbcon, buffer);
		status = 0;
	        continue;
 	}
       
        /* Copy file if needed */
        if (atoi(row[12]) == 0)
            if (atoi(row[5]))
            {
                sprintf(filebuffer, "%s/%s", dir_repeat, row[20]);
                if (!copy_file(encoded_file, filebuffer))
                { // all ok
                    sprintf(buffer, "UPDATE podcast_upload SET repeat_added=1 WHERE id=%s", row[0]);
                    mysql_query(rc_config->dbcon, buffer);
		    status = 1;
                } else {
                    db_log(rc_config, "Uploader: File copy error", "error");
		     status = 0;
                    continue;
                }
            }
        
        /* Upload to external server */
        if (atoi(row[10]) == 0)
            if (atoi(row[6]))
                if (!external_uploader(rc_config, row, encoded_file, return_file)) {
                    if (atoi(row[7])) 
                        plugin_uploader(rc_config, "arrosa", encoded_file, row[0], return_file);
		status = 1;
                } else {
               db_log(rc_config, "Upload error", "error");
                sprintf(buffer, "UPDATE podcast_upload SET uploaded=0 WHERE id=%s", row[0]);
                mysql_query(rc_config->dbcon, buffer);
	status = 0;
}

if (status == 1) {
                    sprintf(buffer, "UPDATE podcast_upload SET uploaded=1,uploaded_date=NOW() WHERE id=%s", row[0]);
                    mysql_query(rc_config->dbcon, buffer);
        sprintf(filebuffer, "%s/%s/%s", dir_radiocloud, dir_upload, row[9]);
        remove(encoded_file);
        remove(filebuffer);

}
        

        
    }
    } else return 0;
    
   
    
    
    
   // syslog(LOG_NOTICE, "Uploader stopped");

    return 1;

}
