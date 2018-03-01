#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "appointments.h"
#include "constant.h"

#ifndef HAVE_SNPRINTF
#  include "snprintf.h"
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static GtkWidget *editbox = NULL;
static GtkWidget *appointments_window = NULL;
char appointments_file[12];

static gint appnt_delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  close_click (widget, data);
  return (TRUE);
}

void
edit_appointments (GtkWidget * widget, gpointer data)
{
  GtkWidget *save_btn;
  GtkWidget *close_btn;
  GtkWidget *vbox;
  GtkWidget *hbuttonbox;
  GtkWidget *scrolled_window;

  gchar title[26];
  guint day, year, month;
  gtk_calendar_get_date (GTK_CALENDAR (widget), &year, &month, &day);
  snprintf (title, 25, "%d/%d/%d ~ Appointments", year, (month + 1), day);
  snprintf (appointments_file, 11, "%d-%d-%d", year, (month + 1), day);

  if (!appointments_window)
  {
    appointments_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (appointments_window), vbox);
    gtk_widget_show (vbox);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_show (scrolled_window);

    editbox = gtk_text_new (NULL, NULL);
    gtk_widget_set_usize (editbox, 300, 100);
    gtk_text_set_editable (GTK_TEXT (editbox), TRUE);
    gtk_container_add (GTK_CONTAINER (scrolled_window), editbox);
    gtk_widget_show (editbox);

    hbuttonbox = gtk_hbutton_box_new ();
    gtk_box_pack_start (GTK_BOX (vbox), hbuttonbox, FALSE, TRUE, 0);
    gtk_widget_show (hbuttonbox);

    save_btn = gtk_button_new_with_label ("Save");
    gtk_box_pack_start (GTK_BOX (hbuttonbox), save_btn, TRUE, FALSE, 0);
    gtk_widget_show (save_btn);

    close_btn = gtk_button_new_with_label ("Close");
    gtk_box_pack_start (GTK_BOX (hbuttonbox), close_btn, TRUE, FALSE, 0);
    gtk_widget_show (close_btn);
    /*
       gtk_signal_connect (GTK_OBJECT (save_btn), "clicked",
       GTK_SIGNAL_FUNC (save_click), NULL);
     */
    gtk_signal_connect (GTK_OBJECT (close_btn), "clicked", GTK_SIGNAL_FUNC (close_click), NULL);
    gtk_signal_connect (GTK_OBJECT (appointments_window), "delete_event", GTK_SIGNAL_FUNC (appnt_delete_event), NULL);
  }
  load_date ();
  gtk_window_set_title (GTK_WINDOW (appointments_window), title);
  gtk_widget_show (appointments_window);
  gtk_widget_grab_focus (editbox);
}

void
save_click (GtkWidget * widget, gpointer data)
{
  FILE *theFile;
  char theFileName[MAXSTRLEN + 1];
  int len, i;
  snprintf (theFileName, MAXSTRLEN, "%s/.xfce/appointments/%s", getenv ("HOME"), appointments_file);
  if ((theFile = fopen (theFileName, "w")) == NULL)
  {
    make_appointments_dir ();
    if ((theFile = fopen (theFileName, "w")) == NULL)
    {
      /* ERROR MESSAGE */
      return;
    }
  }
  len = gtk_text_get_length (GTK_TEXT (editbox));
  if (len == 0)
  {
    fclose (theFile);
    remove (theFileName);
  }
  else
  {
    for (i = 0; i < len; i++)
    {
      fputc (GTK_TEXT_INDEX (GTK_TEXT (editbox), i), theFile);
    }
    fflush (theFile);
    fclose (theFile);
  }
  return;
}

void
close_click (GtkWidget * widget, gpointer data)
{
  guint lg;
  save_click (NULL, NULL);
  gtk_widget_hide (appointments_window);
  gdk_window_withdraw ((GTK_WIDGET (appointments_window))->window);
  lg = gtk_text_get_length (GTK_TEXT (editbox));
  gtk_text_backward_delete (GTK_TEXT (editbox), lg);
}

void
load_date ()
{
  FILE *theFile;
  char theFileName[MAXSTRLEN + 1];
  guint lg;
  snprintf (theFileName, MAXSTRLEN, "%s/.xfce/appointments/%s", getenv ("HOME"), appointments_file);
  lg = gtk_text_get_length (GTK_TEXT (editbox));
  gtk_text_backward_delete (GTK_TEXT (editbox), lg);
  if ((theFile = fopen (theFileName, "r")) == NULL)
  {
    return;
  }
  else
  {
    while (!feof (theFile))
    {
      char line[256];
      char *j;
      j = fgets (line, sizeof (line), theFile);
      if ((j) && (strlen (line)))
      {
	gtk_text_insert (GTK_TEXT (editbox), NULL, NULL, NULL, j, strlen (j));
      }
    }
    fclose (theFile);
    return;
  }
}

void
make_appointments_dir ()
{
  char theFileName[MAXSTRLEN + 1];
  snprintf (theFileName, MAXSTRLEN, "%s/.xfce", getenv ("HOME"));
  mkdir (theFileName, S_IRWXU);
  snprintf (theFileName, MAXSTRLEN, "%s/.xfce/appointments", getenv ("HOME"));
  mkdir (theFileName, S_IRWXU);
}
