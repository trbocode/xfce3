#include<stdio.h>
#include<gtk/gtk.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

void edit_appointments (GtkWidget *, gpointer);
void save_click (GtkWidget *, gpointer);
void close_click (GtkWidget *, gpointer);
void load_date ();
void make_appointments_dir ();
