/* @(#) $Id$ */

/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */
       
/*
 * Rootcheck v 0.3
 * Copyright (C) 2003 Daniel B. Cid <daniel@underlinux.com.br>
 * http://www.ossec.net/rootcheck/
 *
 */

/* Included from the Rootcheck project */


#include "headers/shared.h"

#include "rootcheck.h"

#ifndef ARGV0
#define ARGV0 "rootcheck"
#endif



/** Prototypes **/
/* Read the new XML config */
int Read_Rootcheck_Config(char * cfgfile, rkconfig *cfg);


#ifndef OSSECHIDS

void rootcheck_help()
{
    printf("\n");
    printf("Rootcheck v0.8 (Mar/12/2008):\n");
    printf("http://www.ossec.net/rootcheck/\n");
    printf("Available options:\n");
    printf("\t\t-h\t  This Help message\n");
    printf("\t\t-c <file> Configuration file\n");
    printf("\t\t-d\t  Enable debug\n");
    printf("\t\t-D <dir>  Set the working directory\n");
    printf("\t\t-s\t  Scans the whole system\n");
    printf("\t\t-r\t  Read all the files for kernel-based detection\n");
    printf("\n");
    exit(0);
}

/* main v0.1
 *
 */
int main(int argc, char **argv)
{
    int c;
    int test_config = 0;

#else

int rootcheck_init(int test_config)
{
    int c;
    
#endif    
   
    #ifdef OSSECHIDS 
    char *cfg = DEFAULTCPATH;
    #else
    char *cfg = "./rootcheck.conf";
    #endif
    
    /* Zeroing the structure */
    rootcheck.workdir = NULL;
    rootcheck.basedir = NULL;
    rootcheck.unixaudit = NULL;
    rootcheck.ignore = NULL;
    rootcheck.rootkit_files = NULL;
    rootcheck.rootkit_trojans = NULL;
    rootcheck.winaudit = NULL;
    rootcheck.winmalware = NULL;
    rootcheck.winapps = NULL;
    rootcheck.daemon = 1;
    rootcheck.notify = QUEUE;
    rootcheck.scanall = 0;
    rootcheck.readall = 0;
    rootcheck.disabled = 0;
    rootcheck.alert_msg = NULL;
    rootcheck.time = ROOTCHECK_WAIT;


    /* We store up to 255 alerts in there. */
    os_calloc(256, sizeof(char *), rootcheck.alert_msg);
    c = 0;
    while(c <= 255)
    {
        rootcheck.alert_msg[c] = NULL;
        c++;
    }
    

    #ifndef OSSECHIDS
    rootcheck.notify = SYSLOG;
    rootcheck.daemon = 0;
    while((c = getopt(argc, argv, "VstrdhD:c:")) != -1)
    {
        switch(c)
        {
            case 'V':
                print_version();
                break;
            case 'h':
                rootcheck_help();
                break;
            case 'd':
                nowDebug();
                break;
            case 'D':
                if(!optarg)
                    ErrorExit("%s: -D needs an argument",ARGV0);
                rootcheck.workdir = optarg;
                break;
            case 'c':
                if(!optarg)
                    ErrorExit("%s: -c needs an argument",ARGV0);
                cfg = optarg;
                break;
            case 's':
                rootcheck.scanall = 1;
                break;
            case 't':
                test_config = 1;
                break;        
            case 'r':
                rootcheck.readall = 1;
                break;    
            default:
                rootcheck_help();
                break;   
        }

    }

    
    #ifdef WIN32
    /* Starting Winsock */
    {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
        {
            ErrorExit("%s: WSAStartup() failed", ARGV0);
        }
    }
    #endif
    
                                    
    #endif /* OSSECHIDS */
    

    /* Staring message */
    debug1(STARTED_MSG,ARGV0);


    /* Checking if the configuration is present */
    if(File_DateofChange(cfg) < 0)
    {
        merror("%s: Configuration file '%s' not found",ARGV0,cfg);
        return(-1);
    }


    /* Reading configuration  --function specified twice (check makefile) */
    if(Read_Rootcheck_Config(cfg, &rootcheck) < 0)
    {
        ErrorExit(CONFIG_ERROR, ARGV0, cfg);
    }


    /* If testing config, exit here */
    if(test_config)
        return(0);


    /* Return 1 disables rootcheck */
    if(rootcheck.disabled == 1)
    {
        verbose("%s: Rootcheck disabled. Exiting.", ARGV0);
        return(1);
    }
    
    
    /* Checking if Unix audit file is configured. */
    if(!rootcheck.unixaudit)
    {
        #ifndef WIN32
        log2file("%s: System audit file not configured.", ARGV0);
        #endif
    }
    
    
    /* Setting default values */
    if(rootcheck.workdir == NULL)
        rootcheck.workdir = DEFAULTDIR;


    #ifdef OSSECHIDS
    

    /* Start up message */
    #ifdef WIN32
    verbose(STARTUP_MSG, "ossec-rootcheck", getpid());
    #else

        
    /* Connect to the queue if configured to do so */
    if(rootcheck.notify == QUEUE)
    {
        debug1("%s: Starting queue ...",ARGV0);
        
        /* Starting the queue. */
        if((rootcheck.queue = StartMQ(DEFAULTQPATH,WRITE)) < 0)
        {   
            merror(QUEUE_ERROR,ARGV0,DEFAULTQPATH, strerror(errno));
            
            /* 5 seconds to see if the agent starts */
            sleep(5);
            if((rootcheck.queue = StartMQ(DEFAULTQPATH,WRITE)) < 0)
            {
                /* more 10 seconds wait.. */
                merror(QUEUE_ERROR,ARGV0,DEFAULTQPATH, strerror(errno));
                sleep(10);
                if((rootcheck.queue = StartMQ(DEFAULTQPATH,WRITE)) < 0)
                    ErrorExit(QUEUE_FATAL,ARGV0,DEFAULTQPATH);
            }
        }
    }

    #endif /* Not win32 */
    
    #endif /* ossec hids */


    /* Initializing rk list */
    rk_sys_name = calloc(MAX_RK_SYS +2, sizeof(char *));
    rk_sys_file = calloc(MAX_RK_SYS +2, sizeof(char *));
    if(!rk_sys_name || !rk_sys_file)
    {
        ErrorExit(MEM_ERROR, ARGV0);
    }
    rk_sys_name[0] = NULL;
    rk_sys_file[0] = NULL;


    #ifndef OSSECHIDS
    
    #ifndef WIN32
    /* Start the signal handling */
    StartSIG(ARGV0);
    #endif

    #else
    return(0);
        
    #endif

    
    debug1("%s: DEBUG: Running run_rk_check",ARGV0);
    run_rk_check(); 

   
    debug1("%s: DEBUG:  Leaving...",ARGV0); 

    return(0);        
}

/* EOF */
