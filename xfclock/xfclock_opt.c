#define XFCLOCK_OPT

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_GDK_IMLIB
#include <fcntl.h>
#include <gdk_imlib.h>
#endif

#include <gtk/gtk.h>
#include <time.h>
#include <string.h>
#include "my_intl.h"
#include "mygtkclock.h"
#include "xfclock_colorsel.h"
#include "xfce-common.h"
#include "gnome_protocol.h"
#include "xfclock_menus.h"
#include "xfclock_config.h"
#include "xfclock_style.h"
#include "xfclock_cb.h"
#include "xfclock.h"
#include "xfclock_opt.h"

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

/* cb section: */
void
toggle_friday_cb (void)
{
  current_config.holy_days ^= 0x04;
  monthchanged ((GtkWidget *) calendar);	/* to force an update */
}

void
toggle_saturday_cb (void)
{
  current_config.holy_days ^= 0x02;
  monthchanged ((GtkWidget *) calendar);	/* to force an update */
}

void
toggle_sunday_cb (void)
{
  current_config.holy_days ^= 0x01;
  monthchanged ((GtkWidget *) calendar);	/* to force an update */
}

/* xfclock_opt section: */
void
mark_days (GtkWidget * widget, struct tm *tm, gint holy_day)
{
  gint d, i;
  d = tm->tm_mday + holy_day - tm->tm_wday;
  if (d < 0)
    d += 7;
  while (d >= 7)
    d -= 7;
  for (i = d; i <= 31; i += 7)
  {
    gtk_calendar_mark_day (GTK_CALENDAR (widget), i);
  }
  return;
}


/* to parse holidays, define PARSE_HOLIDAYS for your particular holiday file */
/*#define PARSE_HOLIDAYS "/src/kde-src/generic/kdepim-2.0/korganizer/holidays/holiday_mx"
*/
#ifdef PARSE_HOLIDAYS
/*  to use the open source code to parse holiday files
 *  (as in korganizer from KDE-2)
 *  define PARSE_HOLIDAYS and modify makefile to link in
 *  scanholiday.o
 *  Files needed: parseholiday.c, parseholiday.y, parseholiday.h,
 *                scanholiday.lex, scanholiday.c, and  the holiday files
 *                   
 *  I will put them at ftp://tsekub.imp.mx/
 *
 *  You can also get them from kde.org, as part of the korganizer module     
 *      
 *        */
#include "parseholiday.c"


void
mark_holidays (GtkWidget * widget, int j, guint y)
{
  parse_holidays (PARSE_HOLIDAYS, y, 0);
  for (d = 0; d < 31; d++)
  {
    if (holiday[d + j].string)
    {
      gtk_calendar_mark_day (GTK_CALENDAR (widget), d + 1);
    }
  }
}
#endif


void
mark_holy_days (GtkWidget * widget)
{
  guint i, j, m, y;
  struct tm t;

  gtk_calendar_freeze ((GtkCalendar *) widget);
  gtk_calendar_clear_marks ((GtkCalendar *) widget);
  gtk_calendar_get_date ((GtkCalendar *) widget, &y, &m, NULL);
  t.tm_sec = t.tm_min = t.tm_hour = t.tm_mday = 1;
  t.tm_isdst = 0;
  t.tm_mon = m;
  t.tm_year = y - 1900;
  mktime (&t);
  gtk_calendar_clear_marks ((GtkCalendar *) widget);
#ifdef PARSE_HOLIDAYS
  mark_holidays (widget, t.tm_yday, y - 1900);
#endif
  for (j = 0x01, i = 7; j < 0x08; j <<= 1, i--)
  {
    if (j & current_config.holy_days)
      mark_days (widget, &t, i);
  }
  gtk_calendar_thaw ((GtkCalendar *) widget);
  return;
}

void
monthchanged (GtkWidget * widget)
{
  timeout = FALSE;
  mark_holy_days (widget);
  return;
}

void
show_weeks_cb (void)
{
  current_config.calendar_opt ^= GTK_CALENDAR_SHOW_WEEK_NUMBERS;
  gtk_calendar_display_options (GTK_CALENDAR (calendar), current_config.calendar_opt);
}

void
show_days_cb (void)
{
  current_config.calendar_opt ^= GTK_CALENDAR_SHOW_DAY_NAMES;
  gtk_calendar_display_options (GTK_CALENDAR (calendar), current_config.calendar_opt);
}

void
show_heading_cb (void)
{
  current_config.calendar_opt ^= GTK_CALENDAR_SHOW_HEADING;
  gtk_calendar_display_options (GTK_CALENDAR (calendar), current_config.calendar_opt);
}

void
start_monday_cb (void)
{
  current_config.calendar_opt ^= GTK_CALENDAR_WEEK_START_MONDAY;
  gtk_calendar_display_options (GTK_CALENDAR (calendar), current_config.calendar_opt);
}
