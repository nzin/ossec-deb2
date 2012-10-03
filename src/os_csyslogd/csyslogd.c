/* @(#) $Id: ./src/os_csyslogd/csyslogd.c, 2011/09/08 dcid Exp $
 */

/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 *
 * License details at the LICENSE file included with OSSEC or
 * online at: http://www.ossec.net/en/licensing.html
 */



/* strnlen is a GNU extension */
#ifdef __linux__
 #define _GNU_SOURCE
 #include <string.h>
#endif
#include "csyslogd.h"
#include "os_net/os_net.h"



/* OS_SyslogD: Monitor the alerts and sends them via syslog.
 * Only return in case of error.
 */
void OS_CSyslogD(SyslogConfig **syslog_config)
{
    int s = 0;
    time_t tm;
    struct tm *p;

    file_queue *fileq;
    alert_data *al_data;


    /* Getting currently time before starting */
    tm = time(NULL);
    p = localtime(&tm);


    /* Initating file queue - to read the alerts */
    os_calloc(1, sizeof(file_queue), fileq);
    Init_FileQueue(fileq, p, 0);


    /* Connecting to syslog. */
    s = 0;
    while(syslog_config[s])
    {
        syslog_config[s]->socket = OS_ConnectUDP(syslog_config[s]->port,
                                                 syslog_config[s]->server, 0);
        if(syslog_config[s]->socket < 0)
        {
            merror(CONNS_ERROR, ARGV0, syslog_config[s]->server);
        }
        else
        {
            merror("%s: INFO: Forwarding alerts via syslog to: '%s:%d'.",
                   ARGV0, syslog_config[s]->server, syslog_config[s]->port);
        }

        s++;
    }



    /* Infinite loop reading the alerts and inserting them. */
    while(1)
    {
        tm = time(NULL);
        p = localtime(&tm);


        /* Get message if available (timeout of 5 seconds) */
        al_data = Read_FileMon(fileq, p, 5);
        if(!al_data)
        {
            continue;
        }



        /* Sending via syslog */
        s = 0;
        while(syslog_config[s])
        {
            OS_Alert_SendSyslog(al_data, syslog_config[s]);
            s++;
        }


        /* Clearing the memory */
        FreeAlertData(al_data);
    }
}

/* Format Field for output */
int field_add_string(char *dest, int size, const char *format, const char *value ) {
    char buffer[OS_SIZE_2048];
    int len = 0;
    int dest_sz = size - strnlen(dest, OS_SIZE_2048);

    if(dest_sz <= 0 ) {
        // Not enough room in the buffer
        return -1;
    }

    if(value != NULL &&
            (
                ((value[0] != '(') && (value[1] != 'n') && (value[2] != 'o')) ||
                ((value[0] != '(') && (value[1] != 'u') && (value[2] != 'n')) ||
                ((value[0] != 'u') && (value[1] != 'n') && (value[4] != 'k'))
            )
    ) {
        len = snprintf(buffer, sizeof(buffer) - dest_sz - 1, format, value);
        strncat(dest, buffer, dest_sz);
    }

    return len;
}

/* Add a field, but truncate if too long */
int field_add_truncated(char *dest, int size, const char *format, const char *value, int fmt_size ) {
    char buffer[OS_SIZE_2048];

    int available_sz = size - strnlen(dest, OS_SIZE_2048);
    int total_sz = strlen(value) + strlen(format) - fmt_size;
    int field_sz = available_sz - strlen(format) + fmt_size;

    int len = 0;
    char trailer[] = "...";
    char *truncated;

    if(available_sz <= 0 ) {
        // Not enough room in the buffer
        return -1;
    }

    if(value != NULL &&
            (
                ((value[0] != '(') && (value[1] != 'n') && (value[2] != 'o')) ||
                ((value[0] != '(') && (value[1] != 'u') && (value[2] != 'n')) ||
                ((value[0] != 'u') && (value[1] != 'n') && (value[4] != 'k'))
            )
    ) {
        if( (truncated=malloc(field_sz)) == NULL ) {
            // Memory error
            return -3;
        }

        if( total_sz > available_sz ) {
            // Truncate and add a trailer
            os_substr(truncated, value, 0, field_sz - strlen(trailer) - 1);
            strcat(truncated, trailer);
        }
        else {
            strncpy(truncated,value,field_sz);
        }

        len = snprintf(buffer, available_sz, format, truncated);
        strncat(dest, buffer, available_sz);
        free(truncated);
    }

    return len;
}

/* Handle integers in the second position */
int field_add_int(char *dest, int size, const char *format, const int value ) {
    char buffer[255];
    int len = 0;
    int dest_sz = size - strnlen(dest, OS_SIZE_2048);

    if(dest_sz <= 0 ) {
        // Not enough room in the buffer
        return -1;
    }

    if( value > 0 ) {
        len = snprintf(buffer, sizeof(buffer), format, value);
        strncat(dest, buffer, dest_sz);
    }

    return len;
}
/* EOF */
