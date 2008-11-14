/* 
 * Bridge functions to INDI protocol.
 * Copyright (C) 2008 Markus Wildi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "indiapi.h"
#include "lilxml.h"
#include "base64.h"

typedef struct {
    char *ge, *gv;			/* element name and value */
    //int ok;  ToDo				/* set when found */
} GetEV;

typedef struct {
    char *d;				/* device to seek */
    char *p;				/* property to seek */
    char *e;				/* element to seek */
    GetEV *gev;				/* retrieved elements and values  */
    int ngev;				/* n retrieved elements */
    int wc : 1;				/* whether pattern uses wild cards */
    int ok : 1;				/* something matched this query */
    char *pstate ;                      /* state of the property */
} SearchDef;

typedef struct {
    char *d;				/* device to seek */
    char *t ;                           /* type, Switch, Number, Next */ 
    char *p;				/* property to seek */
    char *e;				/* element to seek */
    char *v ;                           /* value */
    int wc : 1;				/* whether pattern uses wild cards */
    int ok : 1;				/* something matched this query */
} GetPars;

typedef struct {
    char *e, *v;			/* element name and value */
    int ok;				/* set when found */
} SetEV;


/* table of INDI definition elements we can set
 * N.B. do not change defs[] order, they are indexed via -x/-n/-s args
 */
typedef struct {
    char *defType;			/* defXXXVector name */
    char *defOne;			/* defXXX name */
    char *newType;			/* newXXXVector name */
    char *oneType;			/* oneXXX name */
} set_INDIDef;

typedef struct {
    char *d;				/* device */
    char *p;				/* property */
    SetEV *ev;				/* elements */
    int nev;				/* n elements */
    set_INDIDef *dp;			/* one of defs if known, else NULL */
} SetPars;


#define INDIPORT        7624            /* default port */

#ifdef __cplusplus
extern "C" {
#endif
XMLEle *getINDI( FILE *svrwfp, FILE *svrrfp) ;
int setINDI ( SetPars *bsets, int bnsets, FILE *svrwfp, FILE *svrrfp) ;
int        rts2getINDI( char *device, char *data_type, char *property, char *elements, int *cnsrchs, SearchDef **csrchs, FILE *svrwfp, FILE *svrrfp) ;
int        rts2setINDI( char *device, char *data_type, char *property, char *elements, char *values, FILE *svrwfp, FILE *svrrfp) ;
int        fill_getINDIproperty( char *dev, char *type, char *prop, char *ele) ;
int        fill_setINDIproperty( char *dev, char *type, char *prop, char *ele, char *val, SetPars *dsets, int *dnsets) ;
SearchDef *malloc_getINDIproperty() ;
SetPars   *malloc_setINDIproperty( SetPars *csets, int *ncsets) ;
void       free_getINDIproperty( SearchDef *csrchs, int ncsrchs) ;
void       free_setINDIproperty( SetPars *csets, int ncsets) ;
void       openINDIServer( char *host, int port, FILE **svrwfp, FILE **svrrfp);
void       closeINDIServer( FILE *svrwfp, FILE *svrrfp);
#ifdef __cplusplus
}
#endif
