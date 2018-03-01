/*  gxfce
 *  Copyright (C) 2000 Olivier Fourdan (fourdan@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* Inspired by GNOME implementation of session management */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

#ifdef HAVE_SESSION
#include <X11/SM/SMlib.h>

static SmcConn sm_conn = NULL;
static char *sm_client_id = NULL;
static int sm_fd = -1;

#endif

/* This is called when data is available on an ICE sm_fd.  */
static gboolean
process_ice_messages (gpointer client_data, gint source, GdkInputCondition condition)
{
#ifdef HAVE_SESSION
  IceProcessMessagesStatus status;

  status = IceProcessMessages (SmcGetIceConnection (sm_conn), NULL, NULL);

  if (status == IceProcessMessagesIOError)
  {
    IcePointer context = IceGetConnectionContext (SmcGetIceConnection (sm_conn));

    if (context && GTK_IS_OBJECT (context))
    {
      guint disconnect_id = gtk_signal_lookup ("disconnect",
					       GTK_OBJECT_TYPE (context));

      if (disconnect_id > 0)
	gtk_signal_emit (GTK_OBJECT (context), disconnect_id);
    }
    else
    {
      IceSetShutdownNegotiation (SmcGetIceConnection (sm_conn), False);
      IceCloseConnection (SmcGetIceConnection (sm_conn));
    }
  }
#endif
  return TRUE;
}

#ifdef HAVE_SESSION

static IceIOErrorHandler prev_handler;

static void
MyIoErrorHandler (IceConn ice_conn)
{
  if (prev_handler)
    (*prev_handler) (ice_conn);
}

static void
InstallIOErrorHandler (void)
{
  IceIOErrorHandler default_handler;

  prev_handler = IceSetIOErrorHandler (NULL);
  default_handler = IceSetIOErrorHandler (MyIoErrorHandler);
  if (prev_handler == default_handler)
    prev_handler = NULL;
}

static void
callback_save_yourself (SmcConn sm_conn, SmPointer client_data, int save_style, Bool shutdown, int interact_style, Bool fast)
{
  SmcSaveYourselfDone (sm_conn, False);
}

static void
callback_die (SmcConn sm_conn, SmPointer client_data)
{
  SmcCloseConnection (sm_conn, 0, NULL);
}

static void
callback_save_complete (SmcConn sm_conn, SmPointer client_data)
{
}

static void
callback_shutdown_cancelled (SmcConn sm_conn, SmPointer client_data)
{
  SmcSaveYourselfDone (sm_conn, False);
}

#endif

int
OpenICEConn (void)
{
#ifdef HAVE_SESSION
  static SmPointer context;
  gint input_id;
  SmcCallbacks callbacks;
  char error_string_ret[4096] = "";

  InstallIOErrorHandler ();

  callbacks.save_yourself.callback = callback_save_yourself;
  callbacks.die.callback = callback_die;
  callbacks.save_complete.callback = callback_save_complete;
  callbacks.shutdown_cancelled.callback = callback_shutdown_cancelled;

  callbacks.save_yourself.client_data = callbacks.die.client_data = callbacks.save_complete.client_data = callbacks.shutdown_cancelled.client_data = (SmPointer) NULL;

  sm_conn = SmcOpenConnection (NULL, &context, SmProtoMajor, SmProtoMinor, SmcSaveYourselfProcMask | SmcDieProcMask | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask, &callbacks, NULL, &sm_client_id, 4096, error_string_ret);
  if (sm_conn)
  {
    gdk_set_sm_client_id ((gchar *) sm_client_id);
    sm_fd = IceConnectionNumber (SmcGetIceConnection (sm_conn));
    /* Make sure we don't pass on these file descriptors to any
       exec'ed children */
    fcntl (IceConnectionNumber (SmcGetIceConnection (sm_conn)), F_SETFD, fcntl (IceConnectionNumber (SmcGetIceConnection (sm_conn)), F_GETFD, 0) | FD_CLOEXEC);

    input_id = gdk_input_add (IceConnectionNumber (SmcGetIceConnection (sm_conn)), GDK_INPUT_READ | GDK_INPUT_EXCEPTION, (GdkInputFunction) process_ice_messages, (gpointer) (long) IceConnectionNumber (SmcGetIceConnection (sm_conn)));
  }

  return (sm_conn != NULL);
#endif
  return (0);
}

void
CloseICEConn (void)
{
#ifdef HAVE_SESSION
  if (!sm_conn)
    return;
  IceCloseConnection (SmcGetIceConnection (sm_conn));
#endif
}

void
LogoutICEConn (void)
{
#ifdef HAVE_SESSION
  if (!sm_conn)
    return;
  SmcRequestSaveYourself (sm_conn, SmSaveBoth, 1, SmInteractStyleAny, 0, 1);
#endif
}

int
ICEConn (void)
{
#ifdef HAVE_SESSION
  return (sm_conn != NULL);
#endif
  return (0);
}
