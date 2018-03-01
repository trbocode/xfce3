#ifndef __XTREE_ICONS_H__
#define __XTREE_ICONS_H__

#include <gtk/gtk.h>
typedef struct icon_pix {
	GdkPixmap *pixmap;
	GdkBitmap *pixmask;
	GdkPixmap *open;
	GdkBitmap *openmask;
} icon_pix;

gboolean set_icon_pix(icon_pix *pix,int type,char *label,int flags); 
void init_pixmaps(void);

void create_pixmaps(int h,GtkWidget * ctree);
#endif
