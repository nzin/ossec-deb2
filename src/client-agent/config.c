/* @(#) $Id$ */

/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */



#include "shared.h"

#include "os_xml/os_xml.h"
#include "os_regex/os_regex.h"
#include "os_net/os_net.h"

#include "agentd.h"


/* Relocated from config_op.c */

/* ClientConf v0.2, 2005/03/03
 * Read the config file (for the remote client)
 * v0.2: New OS_XML
 */ 
int ClientConf(char *cfgfile)
{
    int modules = 0;
    logr->port = DEFAULT_SECURE;
    logr->rip = NULL;
    logr->lip = NULL;
    logr->rip_id = 0;
    logr->execdq = 0;

    modules|= CCLIENT;

    if(ReadConfig(modules, cfgfile, logr, NULL) < 0)
    {
        return(OS_INVALID);
    }

    return(1);
}


/* EOF */
