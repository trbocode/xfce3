/*  xfsound
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
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

/* 
   19-SEP-2001: Huge update and almost complete rewrite.
   audiofile and aRts support added by Olivier Fourdan
 */

/* Linux sound support initially contributed by Alex Fiori :
 	Hi Oliver! :)
	I'm using XFCE, really rules.
	Well, this is my first contrib for you, and I
	hope I could help you with anything!

	uground ind. - sbase division
	Copyright (c) 1998 Alex Fiori
	[ http://sbase.uground.org - pmci@uground.org ]

	xfdsp internal sound driver for XFCE
	[ http://www.linux-kheops.com/pub/xfce ]

	NOTE: I hope xfsound doesn't need to use
	external players (like sox) anymore. 

	Compile: gcc -Wall -c xfdsp.c
	You can use -DDEBUG to see the errors
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "constant.h"

#if defined(HAVE_AUDIOFILE)
#include <audiofile.h>
#endif

#if defined(HAVE_ARTS)
#include <artsc.h>
#else
#if defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#else
#if defined(HAVE_LINUX_SOUNDCARD_H)
#include <linux/soundcard.h>
#else
#if defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#endif
#endif
#endif
#endif

#include "xfdsp.h"

#ifdef DEBUG			/* use perror() with -DDEBUG */
#include <errno.h>
#endif

#ifdef DMALLOC
#  include "dmalloc.h"
#endif

static char *dsp[] = {
	"/dev/dsp",
	"/dev/dsp0",
	"/dev/dsp1",
	"/dev/dsp2",
	"/dev/dsp3",
	"/dev/dsp4",
	"/dev/dsp5",
	"/dev/dsp6",
	"/dev/dsp7",
	NULL
};

void
sound_init (void)
{
#if defined(HAVE_ARTS)
  switch (fork ())
  {
    case 0:
      execl (DEFAULT_SHELL, DEFAULT_SHELL, "-c", ARTSD_CMD, NULL);
    case -1:
      fprintf (stderr, "xfsound : cannot execute fork()\n");
      break;
    default:
      break;
  }
#endif
}

#if defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_LINUX_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H) || defined(HAVE_ARTS)

#if !defined(HAVE_ARTS)
int masterfd;
#endif

/* BUFFER_FRAMES represents the size of the buffer in frames. */
#define BUFFER_FRAMES 4096

int
i_play (char *soundfile)
{
  void *buffer;
  ST_CONFIG curr;
  int sampleWidth, channelCount, frameRate;
#if defined(HAVE_AUDIOFILE)
  int frameSize, sampleFormat;
  AFfilehandle fp;
  AFframecount	frameCount, framesRead;
#else
  int fp;
  int framesRead;
#endif
#if defined(HAVE_ARTS)
  arts_stream_t 	stream;
  int errorcode;

  errorcode = arts_init();
  if (errorcode < 0)
  {
    fprintf(stderr, "arts_init error: %s\n", arts_error_text(errorcode));
    return -1;
  }
#else
  if (setcard () != 0)
  {
    return -1;
  }
#endif

#if defined(HAVE_AUDIOFILE)
  if ((fp = afOpenFile(soundfile, "r", NULL)) == NULL)
#else
  if ((fp = open (soundfile, O_RDONLY, 0)) == -1)
#endif
  {
#if !defined(HAVE_ARTS)
    close (masterfd);
#endif
#ifdef DEBUG
    perror ("open");
#endif
    return -1;
  }

#if defined(HAVE_AUDIOFILE)
  frameCount = afGetFrameCount(fp, AF_DEFAULT_TRACK);
  channelCount = afGetChannels(fp, AF_DEFAULT_TRACK);
  afGetSampleFormat(fp, AF_DEFAULT_TRACK, &sampleFormat, &sampleWidth);
  frameSize = afGetFrameSize(fp, AF_DEFAULT_TRACK, 1);
  frameRate = (int) afGetRate(fp, AF_DEFAULT_TRACK);
  if ((sampleFormat != AF_SAMPFMT_TWOSCOMP) && (sampleFormat != AF_SAMPFMT_UNSIGNED))
  {
	  printf("xfsound: The audio file must contain integer data in two's complement or unsigned format.\n");
          afCloseFile (fp);
          _exit(-1);
  }
  curr[0] = sampleWidth;
  curr[1] = channelCount;
  curr[2] = frameRate;
#else
  curr[0] = sampleWidth = 8;
  curr[1] = channelCount = 1;
  curr[2] = frameRate = 8000;
#endif

#if defined(HAVE_ARTS)
  stream = arts_play_stream(frameRate, sampleWidth, channelCount, "xfsound");
#else
  cardctl (masterfd, &curr);
  if ((curr[0] != sampleWidth) || (curr[1] != channelCount) || (curr[2] != frameRate))
  {
    fprintf (stderr, "xfsound: Sound format of file %s\n", soundfile);
    fprintf (stderr, "is not supported by the sound device. The sound will not be played.\n");
    fprintf (stderr, "Format requested: sampleWidth=%i channelCount=%i frameRate=%i\n", sampleWidth, channelCount, frameRate);
    fprintf (stderr, "Format supported: sampleWidth=%i channelCount=%i frameRate=%i\n", curr[0], curr[1], curr[2]);
    close (masterfd);
#if defined(HAVE_AUDIOFILE)
    afCloseFile (fp);
#else  
    close (fp);
#endif
    _exit(-1);
  }
#endif

#if defined(HAVE_AUDIOFILE)
  buffer = malloc(BUFFER_FRAMES * frameSize);
  framesRead = afReadFrames(fp, AF_DEFAULT_TRACK, buffer, BUFFER_FRAMES);
#else
  buffer = malloc(BUFFER_FRAMES);
  framesRead = read (fp, buffer, BUFFER_FRAMES);
#endif
  while (framesRead > 0)
  {
#if defined(HAVE_ARTS)
#if defined(HAVE_AUDIOFILE)
    arts_write(stream, buffer, framesRead * frameSize);
#else
    arts_write(stream, buffer, framesRead);
#endif
#else
#if defined(HAVE_AUDIOFILE)
    write (masterfd, buffer, framesRead * frameSize);
#else
    write (masterfd, buffer, framesRead);
#endif
#endif
#if defined(HAVE_AUDIOFILE)
    framesRead = afReadFrames(fp, AF_DEFAULT_TRACK, buffer, BUFFER_FRAMES);
#else
    framesRead = read (fp, buffer, BUFFER_FRAMES);
#endif
  }

#if defined(HAVE_ARTS)
  arts_close_stream(stream);
  arts_free();
#else
  close (masterfd);
#endif

#if defined(HAVE_AUDIOFILE)
  afCloseFile (fp);
#else  
  close (fp);
#endif
  free(buffer);
  return 0;
}

int
setcard (void)
{
#if !defined(HAVE_ARTS)
  char **device = dsp;
  int mode = O_WRONLY | O_NONBLOCK;
  while (*device && ((masterfd = open (*device, mode, 0)) == -1))
  {
#ifdef DEBUG
    perror ("open");
#endif
    device++;
  }
  if (masterfd < 0)
  {
    return -1;
  }
  mode = fcntl(masterfd, F_GETFL);
  mode &= ~O_NONBLOCK;
  fcntl(masterfd, F_SETFL, mode);
#endif
  return 0;
}

void
cardctl (int fp, ST_CONFIG *parm)
{
  int format, frequency, channels;
  
  format    = (*parm)[0];
  channels  = (*parm)[1];
  frequency = (*parm)[2];

#if !defined(HAVE_ARTS)
#if 0
  /* Reset device... */    
  if (ioctl(fp, SNDCTL_DSP_SYNC, 0) == -1)
  {
#ifdef DEBUG
    perror ("ioctl");
#endif
  }
  if (ioctl(fp, SNDCTL_DSP_RESET, 0) == -1)
  {
#ifdef DEBUG
    perror ("ioctl");
#endif
  }
#endif
  /* ...then change params */    
  if (ioctl (fp, SNDCTL_DSP_SETFMT, &format) == -1)
  {
#ifdef DEBUG
    perror ("ioctl");
#endif
  }
  if (ioctl (fp, SNDCTL_DSP_CHANNELS, &channels) == -1)
  {
#ifdef DEBUG
    perror ("ioctl");
#endif
  }
  if (ioctl (fp, SNDCTL_DSP_SPEED, &frequency) == -1)
  {
#ifdef DEBUG
    perror ("ioctl");
#endif
  }
#endif
  (*parm)[0] = format;
  (*parm)[1] = channels;
  (*parm)[2] = frequency;
}

int
cardbusy (void)
{
  int testfd;
#if !defined(HAVE_ARTS)
  if ((testfd = open (DSP_NAME, O_WRONLY | O_NONBLOCK, 0)) == -1)
  {
    return 1;
  }
#endif
  close (testfd);
  return 0;
}
#else
int
i_play (char *soundfile)
{
  return -1;
}

int
setcard (void)
{
  return -1;
}

void
cardctl (int fp, ST_CONFIG * parm)
{
  ;
}

int
cardbusy (void)
{
  return 0;
}
#endif
