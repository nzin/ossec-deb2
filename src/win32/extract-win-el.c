/* @(#) $Id: ./src/win32/extract-win-el.c, 2011/09/08 dcid Exp $
 */

/* Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <windows.h>

#define BUFFER_SIZE 2048*64
#define DEFAULT_FILE 	"C:\\ossec-extracted-evt.log"

FILE *fp;
char *file = DEFAULT_FILE;
char *name = "ossec-extract-evtlog.exe";

/* Event logging local structure */
typedef struct _os_el
{
    int time_of_last;	
    char *name;

    EVENTLOGRECORD *er;
    HANDLE h;

    DWORD record;
}os_el;
os_el el[3];
int el_last = 0;


/** int startEL(char *app, os_el *el)
 * Starts the event logging for each el 
 */
int startEL(char *app, os_el *el)
{
    /* Opening the event log */
    el->h = OpenEventLog(NULL, app);
    if(!el->h)
    {
        return(0);	    
    }

    el->name = app;
    GetOldestEventLogRecord(el->h, &el->record);

    return(1);
}



/** char *el_getCategory(int category_id) 
 * Returns a string related to the category id of the log.
 */
char *el_getCategory(int category_id)
{
    char *cat;
    switch(category_id)
    {
        case EVENTLOG_ERROR_TYPE:
            cat = "ERROR";
            break;
        case EVENTLOG_WARNING_TYPE:
            cat = "WARNING";
            break;
        case EVENTLOG_INFORMATION_TYPE:
            cat = "INFORMATION";
            break;
        case EVENTLOG_AUDIT_SUCCESS:
            cat = "AUDIT_SUCCESS";
            break;
        case EVENTLOG_AUDIT_FAILURE:
            cat = "AUDIT_FAILURE";
            break;
        default:
            cat = "Unknown";
            break;
    }
    return(cat);
}


/** int el_getEventDLL(char *evt_name, char *source, char *event)
 * Returns the event.
 */
int el_getEventDLL(char *evt_name, char *source, char *event) 
{
    HKEY key;
    DWORD ret;
    char keyname[256];


    keyname[255] = '\0';

    snprintf(keyname, 254, 
            "System\\CurrentControlSet\\Services\\EventLog\\%s\\%s", 
            evt_name, 
            source);

    /* Opening registry */	    
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_ALL_ACCESS, &key)
            != ERROR_SUCCESS)
    {
        return(0);    
    }


    ret = MAX_PATH -1;	
    if (RegQueryValueEx(key, "EventMessageFile", NULL, 
                NULL, (LPBYTE)event, &ret) != ERROR_SUCCESS)
    {
        event[0] = '\0';	
        return(0);
    }

    RegCloseKey(key);
    return(1);
}



/** char *el_getmessage() 
 * Returns a descriptive message of the event.
 */
char *el_getMessage(EVENTLOGRECORD *er,  char *name, 
		    char * source, LPTSTR *el_sstring) 
{
    DWORD fm_flags = 0;
    char tmp_str[257];
    char event[MAX_PATH +1];
    char *curr_str;
    char *next_str;
    LPSTR message = NULL;

    HMODULE hevt;

    /* Initializing variables */
    event[MAX_PATH] = '\0';
    tmp_str[256] = '\0';

    /* Flags for format event */
    fm_flags |= FORMAT_MESSAGE_FROM_HMODULE;
    fm_flags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
    fm_flags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;

    /* Get the file name from the registry (stored on event) */
    if(!el_getEventDLL(name, source, event))
    {
        return(NULL);	    
    }	    

    curr_str = event;

    /* If our event has multiple libraries, try each one of them */ 
    while((next_str = strchr(curr_str, ';')))
    {
        *next_str = '\0';
        next_str++;

        ExpandEnvironmentStrings(curr_str, tmp_str, 255);
        hevt = LoadLibraryEx(tmp_str, NULL, DONT_RESOLVE_DLL_REFERENCES);
        if(hevt)
        {
            if(!FormatMessage(fm_flags, hevt, er->EventID, 
                        0,
                        (LPTSTR) &message, 0, el_sstring))
            {
                message = NULL;		  
            }
            FreeLibrary(hevt);

            /* If we have a message, we can return it */
            if(message)
                return(message);
        }

        curr_str = next_str;		
    }

    ExpandEnvironmentStrings(curr_str, tmp_str, 255);
    hevt = LoadLibraryEx(tmp_str, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if(hevt)
    {
        int hr;    
        if(!(hr = FormatMessage(fm_flags, hevt, er->EventID, 
                        0,
                        (LPTSTR) &message, 0, el_sstring)))
        {
            message = NULL;		  
        }
        FreeLibrary(hevt);

        /* If we have a message, we can return it */
        if(message)
            return(message);
    }

    return(NULL);
}



/** void readel(os_el *el)
 * Reads the event log.
 */ 
void readel(os_el *el, int printit)
{
    DWORD nstr;
    DWORD user_size;
    DWORD domain_size;
    DWORD read, needed;
    int size_left;
    int str_size;

    char mbuffer[BUFFER_SIZE];
    LPSTR sstr = NULL;

    char *tmp_str = NULL;
    char *category;
    char *source;
    char *computer_name;
    char *descriptive_msg;

    char el_user[257];
    char el_domain[257];
    char el_string[1025];
    char final_msg[1024];
    LPSTR el_sstring[57];

    /* Er must point to the mbuffer */
    el->er = (EVENTLOGRECORD *) &mbuffer; 

    /* Zeroing the last values */
    el_string[1024] = '\0';
    el_user[256] = '\0';
    el_domain[256] = '\0';
    final_msg[1023] = '\0';
    el_sstring[56] = NULL;

    /* Reading the event log */	    
    while(ReadEventLog(el->h, 
                EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                0,
                el->er, BUFFER_SIZE -1, &read, &needed))
    {
        while(read > 0)
        {

            /* We need to initialize every variable before the loop */
            category = el_getCategory(el->er->EventType);
            source = (LPSTR) ((LPBYTE) el->er + sizeof(EVENTLOGRECORD));
            computer_name = source + strlen(source) + 1;
            descriptive_msg = NULL;


            /* Initialing domain/user size */
            user_size = 255; domain_size = 255;
            el_domain[0] = '\0';
            el_user[0] = '\0';


            /* We must have some description */
            if(el->er->NumStrings)
            {	
                size_left = 1020;	

                sstr = (LPSTR)((LPBYTE)el->er + el->er->StringOffset);
                el_string[0] = '\0';

                for (nstr = 0;nstr < el->er->NumStrings;nstr++)
                {
                    str_size = strlen(sstr);	
                    strncat(el_string, sstr, size_left);

                    tmp_str= strchr(el_string, '\0');
                    if(tmp_str)
                    {
                        *tmp_str = ' ';		
                        tmp_str++; *tmp_str = '\0';
                    }
                    size_left-=str_size + 1;

                    if(nstr <= 54)
                        el_sstring[nstr] = (LPSTR)sstr;

                    sstr = strchr( (LPSTR)sstr, '\0');
                    sstr++; 
                }

                /* Get a more descriptive message (if available) */
                descriptive_msg = el_getMessage(el->er, el->name, source, 
                                                        el_sstring);
                if(descriptive_msg != NULL)
                {
                    /* Remove any \n or \r */
                    tmp_str = descriptive_msg;    
                    while((tmp_str = strchr(tmp_str, '\n')))
                    {
                        *tmp_str = ' ';
                        tmp_str++;		    
                    }			

                    tmp_str = descriptive_msg;    
                    while((tmp_str = strchr(tmp_str, '\r')))
                    {
                        *tmp_str = ' ';
                        tmp_str++;		    
                    }			
                }
            }
            else
            {
                strncpy(el_string, "(no message)", 1020);	
            }


            /* Getting username */
            if (el->er->UserSidLength)
            {
                SID_NAME_USE account_type;
                if(!LookupAccountSid(NULL, (SID *)((LPSTR)el->er + el->er->UserSidOffset),
                            el_user, &user_size, el_domain, &domain_size, &account_type))		
                {
                    strncpy(el_user, "(no user)", 255);
                    strncpy(el_domain, "no domain", 255);
                }

            }

            else
            {
                strncpy(el_user, "(no user)", 255);	
                strncpy(el_domain, "no domain", 255);	
            }


            if(printit)
            {
                DWORD _evtid = 65535;   
                int id = (int)el->er->EventID & _evtid;                  
                
                snprintf(final_msg, 1022, 
                        "%d WinEvtLog: %s: %s(%d): %s: %s(%s): %s",
        		        (int)el->er->TimeGenerated,	
                        el->name,
                        category, 
                        id,
                        source,
                        el_user,
                        el_domain,
                        descriptive_msg != NULL?descriptive_msg:el_string);	
               
	       	fprintf(fp, "%s\n", final_msg);	
            }

            if(descriptive_msg != NULL)
                LocalFree(descriptive_msg);

            /* Changing the point to the er */
            read -= el->er->Length;
            el->er = (EVENTLOGRECORD *)((LPBYTE) el->er + el->er->Length);
        }		

        /* Setting er to the beginning of the buffer */	
        el->er = (EVENTLOGRECORD *)&mbuffer;
    }
}


/** void win_startel()
 * Starts the event logging for windows
 */
void win_startel(char *evt_log)
{
    startEL(evt_log, &el[el_last]);
    readel(&el[el_last],1);
    el_last++;
}

void help()
{
   printf(" OSSEC HIDS - Windows event log extract\n");	
   printf("%s -h		Shows this help message\n", name);	
   printf("%s -e		Extract logs to '%s'\n", name, DEFAULT_FILE);
   printf("%s -f	<file>	Extract logs to the file specified\n", name);	
   exit(0);	
}
/** main **/
int main(int argc, char **argv)
{
   name = argv[0];	
   if((argc == 2)&&(strcmp(argv[1], "-e") == 0))
   {
   }
   else if((argc == 3)&&(strcmp(argv[1], "-f") == 0))
   {
      file = argv[2];	   
   }  
   else
      help();
   
   fp = fopen(file, "w");
   if(!fp)
   {
      printf("Unable to open file '%s'\n", file);
      exit(1);
   }
   
   win_startel("Application");	
   win_startel("System");	
   win_startel("Security");	

   fclose(fp);
   return(0);
}

/* EOF */
