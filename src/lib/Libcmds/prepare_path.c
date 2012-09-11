/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/
/*
 * perpare_path
 *
 * Perpare a full path name to give to the server.
 *
 */

#include <pbs_config.h>   /* the master config generated by configure */

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "pbs_ifl.h"
#include "net_connect.h"


int prepare_path(

  char *path_in,   /* I */
  char *path_out,  /* O */
  char *host)      /* I */

  {
  int          i;

  char        *c;

  char         host_name[PBS_MAXSERVERNAME+1];
  int          h_pos;
  char         path_name[MAXPATHLEN+1];
  int          p_pos;
  char         cwd[MAXPATHLEN+1];
  char        *host_given;

  struct stat  statbuf;
  dev_t        dev;
  ino_t        ino;

  if (path_out != NULL)
    path_out[0] = '\0';

  if ((path_in == NULL) || (path_out == NULL))
    {
    return(1);
    }

  /* initialize data for this parsing call */

  for (i = 0;i <= PBS_MAXSERVERNAME;i++)
    host_name[i] = '\0';

  h_pos = 0;

  for (i = 0;i <= MAXPATHLEN;i++)
    path_name[i] = '\0';

  p_pos = 0;

  cwd[MAXPATHLEN] = '\0';

  /* Begin the parse */

  c = path_in;

  while ((int)isspace(*c))
    c++;

  if (strlen(c) == 0)
    {
    return(1);
    }

  /* Looking for a hostname :  */

  if ((host_given = strchr(path_in, ':')) != NULL)
    {
    while ((*c != ':') && (*c != '\0'))
      {
      if (isalnum(*c) || (*c == '.') || (*c == '-') || (*c == '_'))
        host_name[h_pos++] = *c;
      else
        break;

      c++;
      }
    }

  /* Looking for a posix path */

  if ((*c == ':') || (c == path_in))
    {
    if (*c == ':')
      c++;

    while (*c != '\0')
      {
      if (!isgraph(*c))
        break;

      path_name[p_pos++] = *c;

      c++;
      }
    }

  if (*c != '\0')
    {
    /* FAILURE - we had trailing trash, or a parse error */

    return(1);
    }

  if (strlen(path_name) == 0)
    {
    /* Accept just host name for path */

    if (host_given != NULL)
      {
      strcpy(path_out, host_name);
      strcat(path_out, ":");
      }
    return(3);
    }

  /* get full host name */

  if (host_name[0] == '\0')
    {
    if (host != NULL)
      snprintf(host_name,sizeof(host_name),"%s",host);

    else if (gethostname(host_name, PBS_MAXSERVERNAME) != 0)
      {
      /* FAILURE */

      return(2);
      }
    }

  /*
    This is obnoxious.  If you are submitting from outside of the cluster, you
    tend to get the wrong hostname.  Instead, just stop surprising the user
    and just do what they said.

    if (get_fullhostname(host_name,host_name,PBS_MAXSERVERNAME,NULL) != 0)
      return(2);
  */

  /* prepare complete path name */

  strcpy(path_out, host_name);

  strcat(path_out, ":");

  if ((path_name[0] != '/') && 
      (strncmp(path_name,"$HOME",strlen("$HOME")) != 0) && 
      (strncmp(path_name,"${HOME}",strlen("${HOME}")) != 0) &&
      (host_given == NULL))
    {
    c = getenv("PWD");  /* PWD carries a name that will cause */

    if (c != NULL)
      {
      /* the NFS to mount */

      if (stat(c, &statbuf) < 0)
        {
        /* can't stat PWD */

        c = NULL;
        }
      else
        {
        dev = statbuf.st_dev;
        ino = statbuf.st_ino;

        if (stat(".", &statbuf) < 0)
          {
          /* FAILURE */

          perror("prepare_path: cannot stat current directory:");

          return(1);
          }
        }

      if ((!memcmp(&dev, &statbuf.st_dev, sizeof(dev_t))) &&
          (!memcmp(&ino, &statbuf.st_ino, sizeof(ino_t))))
        {
        if (c != NULL)
          snprintf(cwd, MAXPATHLEN, "%s", c);
        }
      else
        {
        c = NULL;
        }
      }

    if (c == NULL)
      {
      c = getcwd(cwd, MAXPATHLEN);

      if (c == NULL)
        {
        /* FAILURE */

        perror("prepare_path: getcwd failed: ");

        return(1);
        }
      }

    strcat(path_out, cwd);

    strcat(path_out, "/");
    }

  /* try to determine if this is a directory, if it is end it with a '/' */
  
  if ((host_given == NULL) && (path_name[strlen(path_name) - 1] != '/'))
    {
    if ((stat(path_name, &statbuf) == 0) && (S_ISDIR(statbuf.st_mode)))
      {
      strcat(path_name, "/");
      }
    }

  if (strncmp(path_name,"$HOME",strlen("$HOME")) == 0)
    {
    /* get the actual value for $HOME */
    char *HomeVal = getenv("HOME");
    char *NamePtr = path_name+strlen("$HOME");

    /* add the string to the path correctly */
    strcat(path_out,HomeVal);
    strcat(path_out,NamePtr);
    }
  else
    {
    strcat(path_out, path_name);
    }
  
  /* SUCCESS */

  return(0);
  }  /* END prepare_path() */


/* END prepare_path.c */



