#ifndef MODULE_H
#define MODULE_H

struct queue_buff_struct
{
  struct queue_buff_struct *next;
  unsigned long *data;
  int size;
  int done;
};

extern int npipes;
extern int *readPipes;
extern int *writePipes;
extern struct queue_buff_struct **pipeQueue;

#define START_FLAG 0xffffffff

#define XFCE_M_NEW_DESK           (1<<1)
#define XFCE_M_ADD_WINDOW         (1<<2)
#define XFCE_M_RAISE_WINDOW       (1<<3)
#define XFCE_M_LOWER_WINDOW       (1<<4)
#define XFCE_M_CONFIGURE_WINDOW   (1<<5)
#define XFCE_M_FOCUS_CHANGE       (1<<6)
#define XFCE_M_DESTROY_WINDOW     (1<<7)
#define XFCE_M_ICONIFY            (1<<8)
#define XFCE_M_DEICONIFY          (1<<9)
#define XFCE_M_WINDOW_NAME        (1<<10)
#define XFCE_M_ICON_NAME          (1<<11)
#define XFCE_M_RES_CLASS          (1<<12)
#define XFCE_M_RES_NAME           (1<<13)
#define XFCE_M_END_WINDOWLIST     (1<<14)
#define XFCE_M_ICON_LOCATION      (1<<15)
#define XFCE_M_MAP                (1<<16)
#define XFCE_M_ERROR              (1<<17)
#define XFCE_M_CONFIG_INFO        (1<<18)
#define XFCE_M_END_CONFIG_INFO    (1<<19)
#define XFCE_M_ICON_FILE          (1<<20)
#define XFCE_M_DEFAULTICON        (1<<21)
#define XFCE_M_STRING             (1<<22)
#define XFCE_M_MINI_ICON          (1<<23)
#define XFCE_M_SHADE              (1<<24)
#define XFCE_M_UNSHADE            (1<<25)
#define XFCE_M_MAXIMIZE           (1<<26)
#define XFCE_M_DEMAXIMIZE         (1<<27)
#define XFCE_M_SHADE_LOCATION     (1<<28)
#define XFCE_M_RESTACK            (1<<29)
#define MAX_MESSAGES              30
#define MAX_MASK                  ((1 << MAX_MESSAGES)-1)

#define HEADER_SIZE                4
#define MAX_BODY_SIZE             (28)
#define MAX_PACKET_SIZE           (HEADER_SIZE+MAX_BODY_SIZE)


#endif /* MODULE_H */
