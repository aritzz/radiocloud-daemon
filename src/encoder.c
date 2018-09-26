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


// Part of code extracted from gstreamer documentation

static gboolean bus_call (GstBus     *bus, GstMessage *msg, gpointer    data)
{
    GMainLoop *loop = (GMainLoop *) data;

    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_EOS:
          g_main_loop_quit (loop);
          break;

        case GST_MESSAGE_ERROR: {
          gchar  *debug;
          GError *error;
          gst_message_parse_error (msg, &error, &debug);
          g_free (debug);
          g_error_free (error);
          g_main_loop_quit (loop);
          break;
        }
        default:
          break;
    }

    return TRUE;
}

int file_exists(const char * filename){
    /* try to open file to read */
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return 1;
    }
    return 0;
}

static void on_pad_added (GstElement *element, GstPad     *pad, gpointer    data)
{
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
}

void encoder_get_type(char *name, char *ext)
{
    char *point;
    if((point = strrchr(name,'.')) != NULL ) {
        if(strcmp(point,".mp3") == 0) 
            strcpy(ext, "mp3");
        if(strcmp(point,".ogg") == 0) 
            strcpy(ext, "ogg");
    }
}

void encoder_get_settings(struct config *data, int *format, int *quality)
{
    char form_db[DEFAULT_BUFFER];
    char qual_db[DEFAULT_BUFFER];
    
    // Get config info from db
    db_get_config(data, "audioformat", form_db);
    db_get_config(data, "audioquality", qual_db);
    
    if (!strcmp(form_db, "mp3"))
        *format = ENCODER_MP3;
    else
        *format = ENCODER_VORBIS;
    
    // Adjust
    if (atoi(qual_db) > 320)
        *quality = 320;
    else if (atoi(qual_db) < 32)
        *quality = 32;
    else
        *quality = atoi(qual_db);

}


int encode_file(struct config *data, char *title, char *artist, char *input, char *output)
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *oggdemux, *decoder, *conv, *sink, *tomp3, *taginject, *tagmp3, *conv2, *oggdec, *toogg, *oggmux, *normalize, *decoder2;
    GstBus *bus;
    char *point;
    guint bus_watch_id;
    int fromformat = ENCODER_MP3;
    char buffer[DEFAULT_BUFFER];
    int outformat;
    int outquality;
    float oggquality;

    
    /* Initialisation */
   // gst_init (&argc, &argv);
   if (!file_exists(input)) return -1;    

    loop = g_main_loop_new (NULL, FALSE);


    if((point = strrchr(input,'.')) != NULL ) {
        if(strcmp(point,".mp3") == 0) 
            fromformat = ENCODER_MP3;
        if(strcmp(point,".ogg") == 0) 
            fromformat = ENCODER_VORBIS;
    }

    
    /* Create gstreamer elements */
    pipeline = gst_pipeline_new ("audio-converter");
    source   = gst_element_factory_make ("filesrc",       "file-source");
    decoder  = gst_element_factory_make ("mpegaudioparse",     "mpegaudioparse");
    decoder2 = gst_element_factory_make ("mpg123audiodec", "mpg123audiodec");
    oggdemux = gst_element_factory_make ("oggdemux",     "oggdemux");
    oggdec = gst_element_factory_make ("vorbisdec",     "vorbisdec");
    conv     = gst_element_factory_make ("audioconvert",  "converter");
    normalize     = gst_element_factory_make ("audiodynamic",  "audiodynamic");

    conv2     = gst_element_factory_make ("audioconvert",  "converter2");
    toogg = gst_element_factory_make ("vorbisenc",  "vorbisenc");
    oggmux = gst_element_factory_make ("oggmux",  "oggmux");
    tomp3     = gst_element_factory_make ("lamemp3enc",  "lamemp3enc");
    taginject = gst_element_factory_make("taginject", "taginject");
    tagmp3 = gst_element_factory_make("id3v2mux", "id3v2mux");

    sink     = gst_element_factory_make ("filesink", "filesink");

    if (!pipeline || !source || !decoder || !conv || !sink || !tomp3|| !tagmp3 || !taginject || !conv2 || !normalize || !oggdemux || !oggdec || !oggmux || !toogg) 
        return -1;
    

    /* Set up the pipeline */
    encoder_get_settings(data, &outformat, &outquality);
       
    syslog(LOG_NOTICE, "format %d quality %d", outformat, outquality);
    /* we set the input filename to the source element */
    g_object_set (G_OBJECT (source), "location", input, NULL);
    g_object_set(G_OBJECT(sink), "location", output, NULL);
    g_object_set(G_OBJECT(tomp3), "target", 1, NULL);
    g_object_set(G_OBJECT(tomp3), "bitrate", outquality, NULL);
    g_object_set(G_OBJECT(tomp3), "cbr", 1, NULL);

    // normalize quality between 32-320kbps
    oggquality = ((float)outquality-(float)32)/(float)288;
   // oggquality = roundf(10 * oggquality) / 10;
    g_object_set(G_OBJECT(toogg), "quality", oggquality, NULL);
    sprintf(buffer, "title=%s,artist=%s", title, artist);
    g_object_set(G_OBJECT(taginject), "tags", buffer, NULL);


    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

/*
 * Pipe for MP3-MP3: source->mad->audioconvert->audiodynamic->audioconvert->lamemp3enc->taginject->idv2mux->filesink
 * Pipe for OGG-MP3: source->oggdemux->vorbisdec->audioconvert->audiodynamic->audioconvert->lamemp3enc->taginject->idv2mux->filesink
 * Pipe ending with OGG: audioconvert->audiodynamic->vorbisenc->oggmux->filesink
 */    
    
    if (fromformat == ENCODER_MP3)
        if (outformat == ENCODER_MP3) // MP3 to MP3
        {
            gst_bin_add_many (GST_BIN (pipeline),
                    source, decoder, decoder2, conv, normalize, conv2, tomp3, sink, NULL);
            gst_element_link (source, decoder);
            gst_element_link_many (decoder, decoder2, conv, normalize, conv2, tomp3, sink, NULL);
        }
        else { // MP3 to OGG
            gst_bin_add_many (GST_BIN (pipeline),
                    source, decoder, conv, normalize, conv2, toogg, oggmux, sink, NULL);
            gst_element_link (source, decoder);
            gst_element_link_many (decoder, conv, normalize, conv2, toogg, oggmux, sink, NULL);
        }
    else // from OGG
        if (outformat == ENCODER_MP3) // OGG to MP3
        {
            gst_bin_add_many (GST_BIN (pipeline),
                    source, oggdemux, oggdec, conv, normalize, conv2, tomp3, taginject, tagmp3, sink, NULL);
            gst_element_link (source, decoder);
            gst_element_link_many (oggdemux, oggdec, conv, normalize, conv2, tomp3, taginject, tagmp3, sink, NULL);
        }
        else { // OGG to OGG
            gst_bin_add_many (GST_BIN (pipeline),
                    source, oggdemux, oggdec, conv, normalize, conv2, toogg, oggmux, sink, NULL);
            gst_element_link (source, decoder);
            gst_element_link_many (oggdemux, oggdec, conv, normalize, conv2, toogg, oggmux, sink, NULL);
        }
    

    //  g_signal_connect (decoder, "pad-added", G_CALLBACK (on_pad_added), conv);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);

    return 0;
}
