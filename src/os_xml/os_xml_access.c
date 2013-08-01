/*   $OSSEC, os_xml_access.c, v0.3, 2005/02/11, Daniel B. Cid$   */

/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

/* os_xml C Library.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "os_xml.h"


/* Internal functions */
char **_GetElements(OS_XML *_lxml, char **element_name,int type);
char **_GetElementContent(OS_XML *_lxml, char **element_name, char *attr);


/* OS_ElementExist: v1.0: 2005/02/26
 * Check if a element exists 
 * The element_name must be NULL terminated (last char)
 */
int OS_ElementExist(OS_XML *_lxml, char **element_name)
{
    int i=0,j=0,matched=0,totalmatch=0;

    if(element_name == NULL)
        return(0);

    for(i=0,j=0;i<_lxml->cur;i++)
    {
        if(element_name[j] == NULL)
            j=0;
        if((_lxml->tp[i] == XML_ELEM)&&(_lxml->rl[i] == j))
        {
            if(strcmp(_lxml->el[i],element_name[j]) == 0)
            {
                j++;
                matched=1;
                if(element_name[j] == NULL)
                {
                    j=0;
                    totalmatch++;
                }
                continue;
            }
        }
        if((matched == 1) &&(j > _lxml->rl[i])&&
                (_lxml->tp[i] == XML_ELEM))
        {
            j=0;
            matched=0;
        }
    }
    return(totalmatch);
}


/* RootElementExist: v1.0: 2005/02/26
 * Check if a root element exists 
 */
int OS_RootElementExist(OS_XML *_lxml, char *element_name)
{
    char *(elements[])={element_name,NULL};
    return(OS_ElementExist(_lxml,elements));
}


/* GetAttributes: v.0.1: 2005/03/01
 * Get the attributes of the element_name
 */
char **OS_GetAttributes(OS_XML *_lxml, char **element_name)
{
    return(_GetElements(_lxml,element_name,XML_ATTR));
}




/* GetElements: v0.1: 2005/03/01
 * Get the elements children of the element_name
 */
char **OS_GetElements(OS_XML *_lxml, char **element_name)
{
    return(_GetElements(_lxml, element_name,XML_ELEM));
}




/* _GetElements: v0.1: 2005/03/01
 * Get the elements or attributes (internal use)
 */
char **_GetElements(OS_XML *_lxml, char **element_name,int type)
{
    int i=0,j=0,k=0,matched=0,ready=0,size=0;
    char **ret=NULL;

    if((type == XML_ELEM) && (element_name == NULL))
        ready=1;

    for(i=0,j=0;i<_lxml->cur;i++)
    {
        if((ready != 1) &&(element_name[j] == NULL))
        {
            if(matched ==1)
                ready=1;
            else
                break;
        }

        if(j > 16)
            return(ret);

        if((ready == 1)&&(_lxml->tp[i] == type))
        {
            if(((type == XML_ATTR)&&(_lxml->rl[i] == j-1)
                        &&(_lxml->el[i] != NULL))||
                    ((type == XML_ELEM)&&(_lxml->rl[i] == j)&&
                     (_lxml->el[i] != NULL)))
            {
                int el_size = strlen(_lxml->el[i])+1;
                size+=el_size;
                ret = (char**)realloc(ret,(k+1)*sizeof(char *));
                if(ret == NULL)
                    return(NULL);
                ret[k]=(char*)calloc(el_size,sizeof(char));
                if(ret[k] == NULL)
                {
                    free(ret);
                    return(NULL);
                }
                strncpy(ret[k],_lxml->el[i],el_size-1);
                k++;
            }
        }

        else if((_lxml->tp[i] == XML_ELEM)&&(_lxml->rl[i] == j)&&
                (element_name[j] != NULL))
        {
            if(strcmp(_lxml->el[i],element_name[j]) == 0)
            {
                j++;
                matched=1;
                continue;
            }
        }

        if(matched == 1)
        {
            if(((_lxml->tp[i]==XML_ATTR)&&(j > _lxml->rl[i]+1))||
                    ((_lxml->tp[i] == XML_ELEM)&&(j > _lxml->rl[i])))
            {
                j=0;
                matched=0;
                if(element_name == NULL)
                    ready=1;
                else
                    ready=0;
            }
        }
    }
    if(ret ==NULL)
        return(NULL);

    ret = (char**)realloc(ret,(k+1)*sizeof(char *));
    if(ret == NULL)
        return(NULL);
    ret[k]=NULL;
    return(ret);
}



/* OS_GetOneContentforElement: v0.1: 2005/03/01
 * Get one value for a specific element.
 */
char *OS_GetOneContentforElement(OS_XML *_lxml, char **element_name)
{
    int i = 1;
    char *uniqret = NULL;
    char **ret = NULL;

    _lxml->fol = 0;
    ret = _GetElementContent(_lxml, element_name, NULL);
    if(ret == NULL)
    {
        return(NULL);
    }

    if(ret[0] != NULL)
    {
        uniqret = ret[0];
    }
    
    /* Freeing memory */
    while(ret[i])
    {
        free(ret[i]);
        ret[i] = NULL;
        i++;
    }
    free(ret);
    
    return(uniqret);
}


/* OS_GetElementContent: v0.1: 2005/03/01
 * Get all values for a specific element
 */
char **OS_GetElementContent(OS_XML *_lxml, char **element_name)
{
    _lxml->fol=0;
    return(_GetElementContent(_lxml, element_name, NULL));
}


/* OS_GetContents: v0.1: 2005/03/01
 * Get the contents for a specific element
 * Use element_name = NULL to start the state
 */
char **OS_GetContents(OS_XML *_lxml, char **element_name)
{
    if(element_name == NULL)
    {
        _lxml->fol = -1;
        return(NULL);
    }
    return(_GetElementContent(_lxml, element_name, NULL));
}



/* OS_GetAttributeContent: v0.1: 2005/03/01
 * Get one value for a specific attribute 
 */
char *OS_GetAttributeContent(OS_XML *_lxml, char **element_name,
	                         char *attribute_name)
{
    int success = 0;
    char *uniqret = NULL;
    char **ret = NULL;

    _lxml->fol=0;

    ret = _GetElementContent(_lxml, element_name,attribute_name);

    if(ret == NULL)
        return(NULL);

    if(ret[0] != NULL)
    {
        int retsize= strlen(ret[0])+1;
        if((retsize < XML_MAXSIZE) && (retsize > 0))
        {
            uniqret = (char *)calloc(retsize,sizeof(char));
            if(uniqret != NULL)
            {
                strncpy(uniqret, ret[0], retsize-1);
                success = 1;
            }
        }
    }
    while(1)
    {
        if(*ret == NULL)
            break;
        free(*ret++);	
    }
    if(success)
        return(uniqret);
        
    return(NULL);
}


/* _GetElementContent: v0.1: 2005/03/01
 *  Get the values for an element or attribute
 */
char **_GetElementContent(OS_XML *_lxml, char **element_name, char *attr)
{
    int i = 0,j = 0,k = 0,matched = 0;
    char **ret = NULL;

    /* Element name can not be null. */
    if(element_name == NULL)
        return(NULL);

    if(_lxml->fol == _lxml->cur)
    {
        _lxml->fol = 0;
        return(NULL);
    }

    if(_lxml->fol > 0)
    {
        for(i=_lxml->fol;i>=0;i--)
        {
            _lxml->fol = i;
            if(_lxml->rl[i] == 0)
                break;
        }
        i = _lxml->fol;
    }
    else 
    {
        i = 0;
    }


    /* Looping through all nodes */
    for(j=0; i<_lxml->cur; i++)
    {
        if(element_name[j] == NULL)
        {
            if(matched !=1)
                break;
        }
        
        /* Setting maximum depth of 16. */
        if(j > 16)
            return(NULL);


        /* If the type is not an element and the relation doesn't match,
         * keep going.
         */
        if((_lxml->tp[i] != XML_ELEM) || (_lxml->rl[i] != j))
        {
            /* If the node relation is higher than we currently xml
             * node, zero the position and look at it again (i--).
             */
            if(j > _lxml->rl[i])
            {
                j = 0;
                matched = 0;
                i--;
            }
            else
            {
                continue;
            }
        }

        
        /* If the element name matches what we are looking for. */
        else if(strcmp(_lxml->el[i], element_name[j]) == 0)
        {
            j++;
            matched = 1;

            /* Get content if we are at the end of the array. */
            if(element_name[j] == NULL)
            {
                /* If we have an attribute to match. */	
                if(attr != NULL)
                {
                    int k=0;
                    for(k=i+1; k<_lxml->cur; k++)
                    {
                        if(_lxml->tp[k] == XML_ELEM)
                        {
                            break;
                        }
                        
                        if(strcmp(attr, _lxml->el[k]) == 0)
                        {
                            i = k;
                            break;
                        }
                    }
                }

                if(_lxml->ct[i] != NULL)
                {
                    /* Increasing the size of the array. */
                    ret = (char**) realloc(ret,(k+2) * sizeof(char*));
                    if(ret == NULL)
                    {
                        return(NULL);
                    }
                    
                    /* Adding new entry. */
                    ret[k] = strdup(_lxml->ct[i]);
                    ret[k + 1] = NULL;
                    if(ret[k] == NULL)
                    {
                        free(ret);
                        return(NULL);
                    }
                    
                    matched = 1;
                    k++;
                    
                    if(attr != NULL)
                    {
                        break; 
                    }
                    
                    else if(_lxml->fol != 0)
                    {
                        _lxml->fol = i+1;
                        break;
                    }
                }

                /* Setting new array pointer. */
                if((i<_lxml->cur-1) && (_lxml->tp[i+1] == XML_ELEM))
                {
                    j = _lxml->rl[i+1];
                }
            }
            continue;
        }

        if(j > _lxml->rl[i])
        {
            j = 0;
            matched = 0;
        }
    }
    
    if(ret == NULL)
        return(NULL);

    return(ret);
}

/* EOF */
