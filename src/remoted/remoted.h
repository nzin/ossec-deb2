/* @(#) $Id: ./src/remoted/remoted.h, 2011/09/08 dcid Exp $
 */

/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#ifndef __LOGREMOTE_H

#define __LOGREMOTE_H

#ifndef ARGV0
#define ARGV0 "ossec-remoted"
#endif

#include "config/remote-config.h"
#include "sec.h"


/*** Function prototypes ***/

/* Read remoted config */
int RemotedConfig(char *cfgfile, remoted *logr);

/* Handle Remote connections */
void HandleRemote(int position, int uid);

/* Handle Syslog */
void HandleSyslog();

/* Handle Syslog TCP */
void HandleSyslogTCP();

/* Handle Secure connections */
void HandleSecure();

/* Forward active response events */
void *AR_Forward(void *arg);

/* Initialize the manager */
void manager_init(int isUpdate);

/* Wait for messages from the agent to analyze */
void *wait_for_msgs(void *none);

/* Save control messages */
void save_controlmsg(int agentid, char *msg);

/* Send message to agent */
int send_msg(int agentid, char *msg);

/* Initializing send_msg */
void send_msg_init();

int check_keyupdate();

void key_lock();

void key_unlock();

void keyupdate_init();


/*** Global variables ***/

keystore keys;
remoted logr;

#endif
