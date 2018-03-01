 /* xfdiff_misc.h */

/* Copyright 2001 Edscott Wilson Garcia. GNU/GPL license */


/* function prototypes: */
char *drawA_width (void);
void xfdiff_abort (int why);
void finish (int sig);
void cleanF (void);
polygon *pushP (int topR, int botR, int topL, int botL);
void cleanP (void);
void cleanA (void);
void cleanT (void);
int max_strip (char *file);
char *strip_it (char *file, int strip_level);
int check_patch_dir (void);
int check_reject (void);
int checkdir (char *file);
int checknotdir (char *file);
char *get_the_file (char *file);
char *assign (char *dst, char *src);
char *prompt_path (char *message, char *file);
char *prompt_outfile (char *message, char *file);
int first_diff (void);
void set_the_style (GtkWidget * widget);
void get_defaults (void);
void set_highlight (void);
void update_titlesP (void);
