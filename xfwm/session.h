/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SESSION_H
#define SESSION_H

/*
**  Load and save window states.
*/
void LoadWindowStates (char *filename);

/*
** Free allocated structude. Should be called before xfwm dies 
*/
void FreeWindowStates (void);

/*
** Save state to the named file, and if running under SM,
** make the SM properly restart xfwm. 
*/
void RestartInSession (char *filename);

/*
**  Fill in the XfwmWindow struct with information saved from
**  the last session. This expects the fields
**    t->w
**    t->name
**    t->class
 */
void MatchWinToSM (XfwmWindow * t);


/*
**  Try to open a connection to the session manager. If non-NULL,
**  reuse the client_id.
 */
void SessionInit (char *client_id);


/*
**  The file number of the session manager connection or -1
**  if no session manager was found.
 */
extern int sm_fd;

/*
**  Process messages received from the session manager. Call this
**  from the main event loop when there is input waiting sm_fd.
 */
void ProcessICEMsgs (void);

/*
 * Close ICE connection on exit
 */
void CloseICEConn (void);

void LogoutICEConn (void);

void builtin_session_startup (void);

int builtin_save_session (void);

#endif
