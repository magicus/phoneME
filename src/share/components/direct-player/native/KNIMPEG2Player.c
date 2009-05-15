/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <kni.h>
#include "sni.h"

#include "midpMalloc.h"
#include "midpUtilKni.h"
#include "midpError.h"

#include <gst/gst.h>
// lib for g_* functions
#include <glib.h>

typedef struct MPEG2Player {
  GMainLoop *loop;
  GstElement *playbin;
} MPEG2Player;


/*
  Callback to process gstreamer messages.
 */
static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

  case GST_MESSAGE_EOS:
    g_print ("End of stream\n");
    g_main_loop_quit (loop);
    break;

  case GST_MESSAGE_ERROR: {
    gchar  *debug;
    GError *error;

    gst_message_parse_error (msg, &error, &debug);
    g_free (debug);

    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);

    g_main_loop_quit (loop);
    break;
  }
  default:
    g_print("Unprocessed message %s from %s\n", GST_MESSAGE_TYPE_NAME (msg), GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)));
    break;
  }

  return TRUE;
}

static gpointer message_pump(gpointer data) {
  GMainLoop* loop = (GMainLoop*)data;

  g_print ("Pumping...\n");
  g_main_loop_run (loop);

  return g_thread_self();
}

static MPEG2Player* player_init(char* url){
  GstBus *bus;
  GstElement *video_sink, *audio_sink;
  GError* error;
  MPEG2Player* player = midpMalloc(sizeof(MPEG2Player));
  if(NULL == player) {
    g_printerr("KNIMPEG2Player: can't allocate memory for player structure\n");
    return KNI_FALSE;
  }
  memset(player, 0, sizeof(player));

  do {
    if (KNI_FALSE == gst_init_check(NULL, NULL, NULL)) {
      break;
     }

     /* separate thread will be used for message pump */
     if (!g_thread_supported ()) {
       g_thread_init (NULL);
     }

     /* TODO:  need to check how to share one loop between different players */
     if (!(player->loop = g_main_loop_new (NULL, FALSE))){
       break;
     }

     /* Create gstreamer elements */
     player->playbin  = gst_element_factory_make ("playbin",  "player");

     if (!player->playbin) {
       g_printerr ("Can't create playbin. Exiting.\n");
       break;
     }

     /* we set the input filename to the source element */
     g_object_set (G_OBJECT (player->playbin), "uri", url, NULL);

#ifdef ARM
     video_sink = gst_element_factory_make("fbdevsink", "sink");
     if (!video_sink){
       g_printerr("Can't create fbdevsink\n");
       break;
     }
     /* override default video sink */
     g_object_set(G_OBJECT (player->playbin), "video_sink", 
                  video_sink, NULL);
     //gst_object_unref (video_sink);
#endif  

     audio_sink = gst_element_factory_make("fakesink", "sink");
     if (!audio_sink){
       g_printerr("Can't create fakesink");
       break;
     }
     /* override default audio sink */
     g_print("setting audio sink to NULL: alsa bug\n");
     g_object_set(G_OBJECT (player->playbin), "audio_sink", 
                  audio_sink, NULL);
     //gst_object_unref (audio_sink);

     /* we add a message handler */
     bus = gst_pipeline_get_bus (GST_PIPELINE (player->playbin));
     gst_bus_add_watch (bus, bus_call, player->loop);
     gst_object_unref (bus);

     /* TODO: need to start message pump in separate thread */
     g_print("Creating new loop thread: ");
     if (g_thread_create(message_pump,
                         (gpointer)player->loop,
                         FALSE,
                         &error)) {
       g_print("OK\n");
     } else {
       g_printerr("FAILED\n");
       break;
     }
     return player;
  } while (0);

  g_print ("Error occured. Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (player->playbin));
  return NULL;
}

static jboolean player_start(MPEG2Player* player) {
  g_print ("Start playing\n");
  gst_element_set_state (player->playbin, GST_STATE_PLAYING);
  return KNI_TRUE;
}

static jboolean player_stop(MPEG2Player* player) {
  g_print ("Stop playing\n");
  gst_element_set_state (player->playbin, GST_STATE_NULL);
  return KNI_TRUE;
}

static jboolean player_delete(MPEG2Player* player) {
  g_print("Delete player\n");
  gst_object_unref (GST_OBJECT (player->playbin));  
  player->playbin = NULL;
  /* TODO: need to stop message pump? */
  return KNI_TRUE;
}
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_MPEG2Player_nCreate) {
  int player = 0;
  KNI_StartHandles(1);
   
  /* Get URI object parameter */
  GET_PARAMETER_AS_PCSL_STRING(1, URI)
  jbyte* url = pcsl_string_get_utf8_data(&URI);
  player = (int)player_init(url);
  pcsl_string_release_utf8_data(url, &URI);
  RELEASE_PCSL_STRING_PARAMETER

  KNI_EndHandles();
  KNI_ReturnInt(player);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_MPEG2Player_nStart) {
  MPEG2Player* player = (MPEG2Player*)KNI_GetParameterAsInt(1);
  player_start(player);  
  KNI_ReturnBoolean( KNI_TRUE );
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_mmedia_MPEG2Player_nDestroy) {
  MPEG2Player* player = (MPEG2Player*)KNI_GetParameterAsInt(1);
  player_stop(player);
  player_delete(player);  
  KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_mmedia_MPEG2Player_nSetVideoLocation) {
  MPEG2Player* player = (MPEG2Player*)KNI_GetParameterAsInt(1);
  jint x = KNI_GetParameterAsInt(2);
  jint y = KNI_GetParameterAsInt(3);
  jint width = KNI_GetParameterAsInt(4);
  jint height = KNI_GetParameterAsInt(5);
  
  KNI_ReturnBoolean( KNI_TRUE );
}

