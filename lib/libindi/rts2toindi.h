/* 
 * Bridge functions to INDI protocol.
 * Copyright (C) 2008 Markus Wildi, Observatory Vermes
 *
 * This code is heavily based on the work of Jasem Mutlaq and
 * Elwood C. Downey, see indi.sf.net
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

#include <pthread.h>

#include "indiapi.h"
#include "lilxml.h"
#include "base64.h"

// used to exchange values from INDI to RTS2
float indi_ra ;
float indi_dec ;

typedef struct
{
	char *ge, *gv;	   /* element name and value */
} GetEV;

typedef struct
{
	char *d;					 /* device to seek */
	char *p;					 /* property to seek */
	char *e;					 /* element to seek */
	GetEV *gev;					 /* retrieved elements and values  */
	int ngev;					 /* n retrieved elements */
	int wc : 1;					 /* whether pattern uses wild cards */
	int ok : 1;					 /* something matched this query */
	char *pstate ;				 /* state of the property */
} SearchDef;

typedef struct
{
	char *d;					 /* device to seek */
	char *t ;					 /* type, Switch, Number, Next */
	char *p;					 /* property to seek */
	char *e;					 /* element to seek */
	char *v ;					 /* value */
	int wc : 1;					 /* whether pattern uses wild cards */
	int ok : 1;					 /* something matched this query */
} GetPars;

typedef struct
{
	char *e, *v;				 /* element name and value */
	int ok;						 /* set when found */
} SetEV;

/* table of INDI definition elements we can set
 * N.B. do not change defs[] order, they are indexed via -x/-n/-s args
 */
typedef struct
{
	char *defType;				 /* defXXXVector name */
	char *defOne;				 /* defXXX name */
	char *newType;				 /* newXXXVector name */
	char *oneType;				 /* oneXXX name */
} set_INDIDef;

typedef struct
{
	char *d;				 /* device */
	char *p;				 /* property */
	SetEV *ev;				 /* elements */
	int nev;				 /* n elements */
	set_INDIDef *dp;			 /* one of defs if known, else NULL */
} SetPars;

#ifdef __cplusplus
extern "C"
{
#endif
// The "offical" API of the library
	int        rts2getINDI( char *device, const char *data_type, const char *property, const char *elements, int *cnsrchs, SearchDef **csrchs) ;
	int        rts2setINDI( char *device, const char *data_type, const char *property, const char *elements, const char *values) ;
	void       rts2openINDIServer( const char *host, int port);
	void       rts2closeINDIServer();
        void       *rts2listenINDIthread (void * arg) ;
	void       rts2free_getINDIproperty( SearchDef *csrchs, int ncsrchs) ;
#ifdef __cplusplus
}
#endif
