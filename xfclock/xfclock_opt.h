/* cb section: */
void toggle_sunday_cb (void);
void toggle_saturday_cb (void);
void toggle_friday_cb (void);
void show_weeks_cb (void);
void show_days_cb (void);
void show_heading_cb (void);
void start_monday_cb (void);

/* xfclock_opt section :*/
#ifdef XFCLOCK_OPT
gboolean timeout;
#else
extern gboolean timeout;
#endif
void monthchanged (GtkWidget * widget);
void mark_holy_days (GtkWidget * widget);
