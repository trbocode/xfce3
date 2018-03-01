/* copywrite 2001 edscott wilson garcia under GNU/GPL*/

#ifdef XTREE_CPY_MAIN  
#else /* XTREE_CPY_MAIN */
#endif /* XTREE_CPY_MAIN */

/* byte 1 */
#define RW_ERROR_MALLOC		0x01
#define RW_ERROR_OPENING_SRC	0x02
#define RW_ERROR_OPENING_TGT	0x04
#define RW_ERROR_TOO_FEW	0x08
#define RW_ERROR_TOO_MANY	0x10
#define RW_ERROR_READING_SRC	0x20
#define RW_ERROR_WRITING_TGT	0x40
#define RW_ERROR_STAT_READ_SRC	0x80
/* byte 2 */
#define RW_ERROR_STAT_READ_TGT	0x100
#define RW_ERROR_STAT_WRITE_TGT	0x200
#define RW_ERROR_MULTIPLE2FILE	0x400
#define RW_ERROR_DIR2FILE	0x800
#define RW_ERROR_ENTRY_NEW	0x1000
#define RW_ERROR_CLOSE_SRC	0x2000
#define RW_ERROR_CLOSE_TGT	0x4000
#define RW_ERROR_UNLINK_SRC	0x8000
/* byte 3 */
#define RW_ERRNO		0x10000
#define RW_ERROR_FIFO		0x20000
#define RW_ERROR_DEVICE		0x40000
#define RW_ERROR_SOCKET		0x80000
#define RW_ERROR_WRITING_SRC	0x100000
#define RW_OK			0x200000
#define RW_ERROR_WRITING_DIR 	0x400000

#define TR_COPY		0x01
#define TR_MOVE		0x02
#define TR_LINK		0x04
#define TR_OVERRIDE	0x08

void cb_touch (GtkWidget * item, GtkCTree * ctree);
void cb_symlink (GtkWidget * item, GtkCTree * ctree);
void cb_duplicate (GtkWidget * item, GtkCTree * ctree);
GtkWidget *show_cpy(GtkWidget *parent,gboolean show,int mode);
int rw_file(char *target,char *source);
void set_show_cpy(char *target,char *source);
void set_show_cpy_bar(int item,int nitems);
char *CreateTmpList(GtkWidget *parent,GList *list,entry *t_en);
gboolean DeleteTmpList(char *tmpfile);
gboolean IndirectTransfer(GtkWidget *ctree,int mode,char *tmpfile);
gboolean DirectTransfer(GtkWidget *ctree,int mode,char *tmpfile);
void set_override(gboolean state);
gboolean on_same_device(void);
char *randomTmpName(char *ext);
int rsync(GtkCTree *ctree,char *src,char *tgt);


/* Block size definition, from FSF cp */
/* Get or fake the disk device blocksize.
   Usually defined by sys/param.h (if at all).  */
#if !defined DEV_BSIZE && defined BSIZE
# define DEV_BSIZE BSIZE
#endif
#if !defined DEV_BSIZE && defined BBSIZE /* SGI */
# define DEV_BSIZE BBSIZE
#endif
#ifndef DEV_BSIZE
# define DEV_BSIZE 4096
#endif

/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Preferred I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
   ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
#ifndef HAVE_STRUCT_STAT_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined(_POSIX_SOURCE) || !defined(BSIZE) /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0) : 0)
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? st_blocks ((statbuf).st_size) : 0)
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_STRUCT_STAT_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
			       ? (statbuf).st_blksize : DEV_BSIZE)
# if defined(hpux) || defined(__hpux__) || defined(__hpux)
/* HP-UX counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and BSD filesystems with NFS.  */
#  define ST_NBLOCKSIZE 1024
# else /* !hpux */
#  if defined(_AIX) && defined(_I386)
/* AIX PS/2 counts st_blocks in 4K units.  */
#   define ST_NBLOCKSIZE (4 * 1024)
#  else /* not AIX PS/2 */
#   if defined(_CRAY)
#    define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks * ST_BLKSIZE(statbuf)/ST_NBLOCKSIZE : 0)
#   endif /* _CRAY */
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_STRUCT_STAT_ST_BLOCKS */


#ifndef ST_NBLOCKS
# define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks : 0)
#endif

#ifndef ST_NBLOCKSIZE
# define ST_NBLOCKSIZE 512
#endif




