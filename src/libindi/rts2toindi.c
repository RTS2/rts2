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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include <zlib.h>
#include "rts2toindi.h"

static int verbose=0;			/* report extra info */

typedef struct {
    char *type ;
    char *name ;
    char *value ;
} INDIelement ; 


/* ToDo clean up after this line */


/* used to efficiently manage growing malloced string space */
typedef struct {
    char *s;                            /* malloced memory for string */
    int sl;                             /* string length, sans trailing \0 */
    int sm;                             /* total malloced bytes */
} String;
#define MINMEM  64                      /* starting string length */


typedef enum  {
    LOOK4START = 0,                     /* looking for first element start */
    LOOK4TAG,                           /* looking for element tag */
    INTAG,                              /* reading tag */
    LOOK4ATTRN,                         /* looking for attr name, > or / */
    INATTRN,                            /* reading attr name */
    LOOK4ATTRV,                         /* looking for attr value */
    SAWSLASH,                           /* saw / in element opening */
    INATTRV,                            /* in attr value */
    ENTINATTRV,                         /* in entity in attr value */
    LOOK4CON,                           /* skipping leading content whitespc */
    INCON,                              /* reading content */
    ENTINCON,                           /* in entity in pcdata */
    SAWLTINCON,                         /* saw < in content */
    LOOK4CLOSETAG,                      /* looking for closing tag after < */
    INCLOSETAG                          /* reading closing tag */
} State;                                /* parsing states */

/* maintain state while parsing */
struct _LilXML {
    State cs;                           /* current state */
    int ln;                             /* line number for diags */
    XMLEle *ce;                         /* current element being built */
    String endtag;                      /* to check for match with opening tag*/
    String entity;                      /* collect entity seq */
    int delim;                          /* attribute value delimiter */
    int lastc;                          /* last char (just used wiht skipping)*/
    int skipping;                       /* in comment or declaration */
};

/* internal representation of a (possibly nested) XML element */
struct _xml_ele {
    String tag;                         /* element tag */
    XMLEle *pe;                         /* parent element, or NULL if root */
    XMLAtt **at;                        /* list of attributes */
    int nat;                            /* number of attributes */
    int ait;                            /* used to iterate over at[] */
    XMLEle **el;                        /* list of child elements */
    int nel;                            /* number of child elements */
    int eit;                            /* used to iterate over el[] */
    String pcdata;                      /* character data in this element */
    int pcdata_hasent;                  /* 1 if pcdata contains an entity char*/
};

/* internal representation of an attribute */
struct _xml_att {
    String name;                        /* name */
    String valu;                        /* value */
    XMLEle *ce;                         /* containing element */
};


/* table of INDI definition elements, plus setBLOB.
 * we also look for set* if -m
 */
typedef struct {
    char *vec;		/* vector name */
    char *one;		/* one element name */
} get_INDIDef;


static get_INDIDef get_defs[] = { /* ToDo unify it with set_INDIDef */
    {"defTextVector",   "defText"},
    {"defNumberVector", "defNumber"},
    {"defSwitchVector", "defSwitch"},
    {"defLightVector",  "defLight"},
    {"defBLOBVector",   "defBLOB"},
    {"setBLOBVector",   "oneBLOB"},
    {"setTextVector",   "oneText"},
    {"setNumberVector", "oneNumber"},
    {"setSwitchVector", "oneSwitch"},
    {"setLightVector",  "oneLight"},
};
static int ndefs = 10;			/* or 10 if -m */

/* table of keyword to use in query vs name of INDI defXXX attribute */
typedef struct {
    char *keyword;
    char *indiattr;
} INDIkwattr;

static INDIkwattr kwattr[] = {
    {"_LABEL", "label"},
    {"_GROUP", "group"},
    {"_STATE", "state"},
    {"_PERM", "perm"},
    {"_TO", "timeout"},
    {"_TS", "timestamp"},
};
#define NKWA   (sizeof(kwattr)/sizeof(kwattr[0]))

/* *********************************************************** */
/* *********************************************************** */
/* *********************************************************** */
/* *********************************************************** */
/* *********************************************************** */

static set_INDIDef set_defs[] = { /* ToDo unify it with get_INDIDef */
    {"defTextVector",   "defText",   "newTextVector",   "oneText"},
    {"defNumberVector", "defNumber", "newNumberVector", "oneNumber"},
    {"defSwitchVector", "defSwitch", "newSwitchVector", "oneSwitch"},
};
#define	NDEFS	(sizeof(set_defs)/sizeof(set_defs[0]))



/* ToDo */
static void getprops( FILE *svrwfp, FILE *svrrfp);
static XMLEle * get_listenINDI( FILE *svrwfp, FILE *svrrfp);
static int finished (void);
static void onAlarm (int dummy);
static int readServerChar(FILE *svrwfp, FILE *svrrfp);
static void findDPE (XMLEle *root, FILE *svrwfp, FILE *svrrfp);
static void findEle (XMLEle *root, const char *dev, const char *nam, char *defone, SearchDef *sp);
static void enableBLOBs(const char *dev, const char *nam, FILE *svrwfp, FILE *svrrfp);
static void oneBLOB (XMLEle *root, const char *dev, const char *nam, const char *enam, char *p, int plen);
static void sendNew (FILE *fp, set_INDIDef *dp, SetPars *sp);

static void sendSpecs(FILE *wfp, SetPars *fsets, int nsets);
static void printpc (FILE *fp, XMLEle *ep, int level) ;

#define TIMEOUT		35		/* default timeout, secs */
static int timeout = TIMEOUT;		/* working timeout, secs */
static LilXML *lillp;			/* XML parser context */
#define WILDCARD	'*'		/* show all in this category */
static int onematch;			/* only one possible match */
static int justvalue;			/* if just one match show only value */
static int monitor;			/* keep watching even after seen def */
static int wflag;			/* show wo properties too */
/*ToDo: put the variables into the parameter list */
static SearchDef *srchs;		/* properties to look for */
static int nsrchs;
static SetPars *sets;			/* set of properties to set */

int rts2getINDI( char *device, char *data_type, char *property, char *elements, int *cnsrchs, SearchDef **csrchs, FILE *svrwfp, FILE *svrrfp)
{ 
    int i, j ;
    nsrchs = 0 ;
    srchs=NULL; 
    srchs=malloc_getINDIproperty(NULL, &nsrchs) ;

    fill_getINDIproperty( device, data_type, property, elements) ;
    XMLEle *root_return1= getINDI( svrwfp, svrrfp) ;

    if( verbose) {
	for( i= 0; i < nsrchs; i++)
	{
	    for( j= 0; j < srchs[i].ngev; j++)
	    {
		fprintf( stderr, "GOT  %s.%s.%s=%s, state >%s<\n", srchs[i].d, srchs[i].p, srchs[i].gev[j].ge, srchs[i].gev[j].gv, srchs[i].pstate) ;
	    }
	}
    }
    delXMLEle (root_return1) ;
    *cnsrchs= nsrchs ;
    *csrchs= srchs;
    //was here free_getINDIproperty( srchs, nsrchs) ;
    return 0 ;
}
XMLEle *getINDI( FILE *svrwfp, FILE *svrrfp)
{
    
    if( verbose > 1)
	fprintf( stderr, "rts2getindi: nsrchs %d\n", nsrchs) ;
//ToDo: what does it (see original version)
    onematch = nsrchs == 1 && !srchs[0].wc;

    /* build a parser context for cracking XML responses */
    lillp = newLilXML();

    /* issue getProperties */
     getprops(svrwfp, svrrfp);

    /* listen for responses, looking for d.p.e or timeout */
    XMLEle *root_return= get_listenINDI( svrwfp, svrrfp);
    delLilXML(lillp);  //wildi
    
    return root_return;
}
int rts2setINDI ( char *device, char *data_type, char *property, char *elements, char *values, FILE *svrwfp, FILE *svrrfp)
{
    SetPars *xsets=NULL ;
    int xnsets = 0 ;

    xsets=malloc_setINDIproperty(NULL, &xnsets) ;
    fill_setINDIproperty( device, data_type, property, elements, values, xsets, &xnsets) ;
    setINDI( xsets,  xnsets, svrwfp, svrrfp) ;

    free_setINDIproperty( xsets, xnsets) ;
    return 0 ;
}
int setINDI ( SetPars *bsets, int bnsets, FILE *svrwfp, FILE *svrrfp)
{
    /* build a parser context for cracking XML responses */
    lillp = newLilXML();

    if( verbose > 1)
	fprintf (stderr, "sendSpecs(svrwfp)\n");
    sendSpecs(svrwfp, bsets, bnsets);

    delLilXML(lillp); //wildi
    return 0 ;
}


/* open a connection to the given host and port.
 * set svrwfp and svrrfp or die.
 */
void
openINDIServer (char *host, int port, FILE **svrwfp, FILE **svrrfp)
{
	struct sockaddr_in serv_addr;
	struct hostent *hp;
	int sockfd;

	/* lookup host address */
	hp = gethostbyname (host);
	if (!hp) {
	    herror ("gethostbyname");
	    exit (2);
	}

	/* create a socket to the INDI server */
	(void) memset ((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr =
			    ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
	serv_addr.sin_port = htons(port);
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
	    perror ("socket");
	    exit(2);
	}



	/* connect */
	if (connect (sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
	    perror ("connect");
	    exit(2);
	}

 
	/* prepare for line-oriented i/o with client */
	*svrwfp = fdopen (sockfd, "w");
	*svrrfp = fdopen (sockfd, "r");

	return ;
}
void closeINDIServer( FILE *svrwfp, FILE *svrrfp)
{
    int resw=0 ;
    int resr=0 ;
    resw= fclose( svrwfp) ;
    resr= fclose( svrrfp) ; // necessary see valgrind
    
    if(resw)
    {
	fprintf( stderr, "Close failed %d\n", resw) ;
	perror("Closing") ;
    }
    return ;
}

/* issue getProperties on one property and one device */
static void
getprops(FILE *svrwfp, FILE *svrrfp)
{
	char *onedev = NULL;
	int i;

	/* find if only need one device */
	for (i = 0; i < nsrchs; i++) {
	    char *d = srchs[i].d;
	    if (*d == WILDCARD || (onedev && strcmp(d,onedev))) {
		onedev = NULL;
		break;
	    } else
		onedev = d;
	}

	if (onedev)
	{
	    //fprintf(stderr, "onedev <getProperties version='%g' device='%s'/>\n",INDIV, onedev);
	    fprintf(svrwfp, "<getProperties version='%g' device='%s'/>\n",
								INDIV, onedev);
	}
	else
	{
	    fprintf(stderr, "NOT                 <getProperties version='%g' device='%s'/>\n",INDIV, onedev);
	    fprintf(svrwfp, "<getProperties version='%g'/>\n", INDIV);

	}
	fflush (svrwfp);
	if (verbose)
	    fprintf (stderr, "Queried properties from %s\n", onedev?onedev:"*");
}


/* { */
/*     fprintf(svrwfp, "<getProperties version='%g' device='%s'/>\n", INDIV, srchs[0].d); */
/*     fflush (svrwfp); */

/*     if (verbose) */
/* 	fprintf(stderr, "<getProperties version='%g' device='%s'/>\n", INDIV, srchs[0].d); */

/* } */

/* listen for INDI traffic on svrrfp.
 * print matching srchs[] and return when see all.
 * timeout and exit if any trouble.
 */
static XMLEle *
get_listenINDI ( FILE *svrwfp, FILE *svrrfp)
{
    char msg[1024];

    /* arrange to call onAlarm() if not seeing any more defXXX */
    //signal (SIGALRM, onAlarm);
    //alarm (timeout);




    /* read from server, exit if find all requested properties */
    while (1) {
        
	XMLEle *root = readXMLEle (lillp, readServerChar(svrwfp, svrrfp), msg);

	if (root) {

	    /* found a complete XML element */
	    if (verbose > 1)
		prXMLEle (stderr, root, -1);

	    findDPE (root, svrwfp, svrrfp);
	    if (finished() == 0)
	    {
// It is up to the caller to free the memory with delXMLEle see lilxmlc.c

		return root;		/* found all we want */
	    }
// see  Xephem indimenu.c 649

	    delXMLEle (root);	/* From original :*/	/* not yet, delete and continue */
	} else if (msg[0]) {
	    fprintf (stderr, "Bad XML %s\n", msg);
// ToDo wildi Do not exit
	    
	    exit(2);
	}
    }
}

/* return 0 if we are sure we have everything we are looking for, else -1 */
static int
finished ()
{
	int i;

	if (monitor)
	    return(-1);


	for (i = 0; i < nsrchs; i++)
	{
/* wildi might be wrong 	    if (srchs[i].wc || !srchs[i].ok) */
	    if (!srchs[i].ok)
	    {
		return (-1);
	    }
	}

	return (0);
}

/* called after timeout seconds either because we are matching wild cards or
 * there is something still not found
 */
static void
onAlarm (int dummy)
{
	int trouble = 0;
	int i;

	fprintf( stderr, "ONALARM <-------------------------\n") ;
	for (i = 0; i < nsrchs; i++) {
	    if (!srchs[i].ok) {
		trouble = 1;
		fprintf (stderr, "onAlaram No %s.%s.%s\n", srchs[i].d,
					srchs[i].p, srchs[i].e);
	    }
	}
	fprintf( stderr, "ONALARM <-------------------------\n") ;
	exit (trouble ? 1 : 0);
}

/* read one char from svrrfp */
static int
readServerChar (FILE *svrwfp, FILE *svrrfp)
{
	int c = fgetc (svrrfp);

	if (c == EOF) {
	    if (ferror(svrrfp))
	    {
		perror ("read");
	    }
	    else
	    {
	    fprintf (stderr,"EXITING on EOF\n") ;
	    }
	    exit (2);
	}

	if (verbose > 2)
	    fprintf (stderr, "Read %c\n", c);

	return (c);
}

/* print value if root is any srchs[] we are looking for*/
static void
findDPE (XMLEle *root, FILE *svrwfp, FILE *svrrfp)
{
    int i, j;
    
    //fprintf( stderr, "findDPE nsrchs %d, ndefs %d, \n", nsrchs, ndefs) ;

   for (i = 0; i < nsrchs; i++) {
	/* for each property we are looking for */
	for (j = 0; j < ndefs; j++) {
	    /* for each possible type */
	    //fprintf( stderr, "findDPE nsrchs %d, ndefs %d, get_defs[j].vec %s\n", nsrchs, ndefs, get_defs[j].vec) ;
	    if (strcmp (tagXMLEle (root), get_defs[j].vec) == 0) { //wildi
		/* legal defXXXVector, check device */
		const char *dev = findXMLAttValu (root, "device");
		const char *idev = srchs[i].d;

		//fprintf( stderr, "==========findDPE nsrchs %d, ndefs %d, get_defs[j].vec %s, dev %s, idev %s\n", nsrchs, ndefs, get_defs[j].vec, dev, idev) ;
		if (idev[0] == WILDCARD || !strcmp (dev,idev)) {
		    /* found device, check name */
		    const char *nam = findXMLAttValu (root, "name");
		    const char *iprop = srchs[i].p;
		    //fprintf( stderr, "==========findDPE nsrchs %d, ndefs %d, get_defs[j].vec %s, dev %s, idev %s, name %s\n", nsrchs, ndefs, get_defs[j].vec, dev, idev, nam) ;
    if (iprop[0] == WILDCARD || !strcmp (nam,iprop)) {
			/* found device and name, check perm */
			const char *perm = findXMLAttValu (root, "perm");
			//fprintf( stderr, ">>>>>>>>>>>>>>findDPE nsrchs %d, ndefs %d, get_defs[j].vec %s, dev %s, idev %s\n", nsrchs, ndefs, get_defs[j].vec, dev, idev) ;
			if (!wflag && perm[0] && !strchr (perm, 'r')) {
			    if (verbose)
			    {
				fprintf (stderr, "%s.%s is write-only\n",
								    dev, nam);
			    }
			} else {
			    /* check elements or attr keywords */
			    if (!strcmp (get_defs[j].vec, "defBLOBVector"))
				enableBLOBs (dev,nam, svrwfp, svrrfp);
			    else
			    {
				if( verbose > 1 )
				    fprintf( stderr, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<findDPE i=%d, j=%d, name %s, srchs %s OK %d\n", i, j, nam, srchs[i].e, srchs[i].ok) ;
				findEle(root,dev,nam,get_defs[j].one,&srchs[i]);
				
			    }
			    if (onematch)
			    {
				if( verbose)
				{
				    fprintf( stderr, "-------->> ONEMATCH\n") ;
				    printpc( stderr, root, 0) ;
				}
				return;		/* only one can match */
			    }
			    else
			    {
				if( verbose)
				{
				//fprintf( stderr, "-------->> WILDCARD MATCH\n") ;
				    printpc( stderr, root, 0) ;
				}
			    }
			    if (!strncmp (get_defs[j].vec, "def", 3))
			    {
			    	//fprintf( stderr, "-------->> reset timer %s\n", get_defs[j].vec) ;
					alarm (timeout);	/* reset timer if def */
			    }
			}
		    }
		}
	    }
	}
    }
}
#define PRINDENT        4               /* sample print indent each level */

void printpc (FILE *fp, XMLEle *ep, int level)
{

        int indent = level*PRINDENT;
        int i;
	if( verbose > 1)
	    fprintf (fp, "%*s<%s", indent, "", ep->tag.s);

        for (i = 0; i < ep->nat; i++)
	    
	    fprintf (fp, "%s=\"%s\"", ep->at[i]->name.s,
                                                entityXML(ep->at[i]->valu.s));
        if (ep->nel > 0) {
	    
//	    fprintf (fp, ">\n");
            for (i = 0; i < ep->nel; i++)
                printpc(fp, ep->el[i], level+1);
        }
	//fprintf (fp, ">%s\n", ep->pcdata.s);
        if (ep->pcdata.sl > 0) {
            //if (ep->nel == 0)
		//              fprintf (fp, ">\n");
            if (ep->pcdata_hasent)
                fprintf (fp, "%s", entityXML(ep->pcdata.s));
            else
		//  fprintf (fp, "---------level %d--------------%s", level, ep->pcdata.s);
            if (ep->pcdata.s[ep->pcdata.sl-1] != '\n')
                fprintf (fp, "\n");
        }
//        if (ep->nel > 0 || ep->pcdata.sl > 0)
	    //fprintf (fp, "%*s</%s>\n", indent, "", ep->tag.s);
//        else
	    //fprintf (fp, "/>\n");
}

/* print elements in root speced in sp (known to be of type defone).
 * print just value if onematch and justvalue else fully qualified name.
 */
static void
findEle (XMLEle *root, const char *dev, const char *nam, char *defone, SearchDef *sp)
{
    char *iele = sp->e;
	XMLEle *ep;
	int i;
    
	/* check for attr keyword */
	for (i = 0; i < NKWA; i++) {
	    if (strcmp (iele, kwattr[i].keyword) == 0) {
		/* just print the property state, not the element values */

		if( verbose > -1)
		{
		    const char *s = findXMLAttValu (root, kwattr[i].indiattr);
		    if (onematch && justvalue)
		    {
			printf ("JJ-------------------%s\n", s);
		    }
		    else
		    {
			strcpy( sp->pstate, s) ; 
			if( verbose)
			    printf ("II-------------------%s.%s.%s=%s\n", dev, nam, kwattr[i].keyword, sp->pstate);
		    }
		}
		sp->ok = 1;   			/* progress */
		return;
	    }
	}

	
	const char *s = findXMLAttValu (root, kwattr[2].indiattr); // watchout for state
	if (onematch && justvalue)
	{
	    if( verbose)
	    printf ("PSTATE 1-------------------%s\n", s);
	}
	else
	{
	    strcpy( sp->pstate, s) ; 
	    if( verbose)
		printf ("PSTATE 2-------------------%s.%s.%s=%s\n", dev, nam, kwattr[i].keyword, sp->pstate);
	}


	/* no special attr, look for specific element name */
	for (ep = nextXMLEle(root,1); ep; ep = nextXMLEle(root,0)) {
	    if (!strcmp (tagXMLEle (ep), defone)) {
		/* legal defXXX, check deeper */
		const char *enam = findXMLAttValu (ep, "name");
		if (iele[0]==WILDCARD || !strcmp(enam,iele)) {
		    /* found it! */
		    char *p = pcdataXMLEle(ep);
		    sp->ok = 1;   		/* progress */
		    if (!strcmp (defone, "oneBLOB"))
			oneBLOB (ep, dev, nam, enam, p, pcdatalenXMLEle(ep));
		    else if (onematch && justvalue)
		    {


			//printf ("GG-----------------------%s\n", p);
		    }
		    else
		    {
			sp->gev = (GetEV *) realloc (sp->gev, (sp->ngev+1)*sizeof(GetEV));
			sp->gev[sp->ngev].ge = strcpy (malloc(strlen(enam)+1), enam);
			sp->gev[sp->ngev].gv = strcpy (malloc(strlen(p)+1), p);
			sp->ngev++ ;

			 
			if( verbose > 1 )
			{
			    printf ("HHX-----------------------%s.%s.%s=%s, OK: %d Ele: %s, Val: %s\n",  dev, nam, enam, p, sp->ok, sp->gev[sp->ngev-1].ge, sp->gev[sp->ngev-1].gv);
			}
		    }
		    if (onematch)
			return;      	/* only one can match*/
		}
	    }
	}
}

/* send server command to svrwfp that enables blobs for the given dev nam
 */
static void
enableBLOBs(const char *dev, const char *nam, FILE *svrwfp, FILE *svrrfp)
{
	if (verbose)
	    fprintf (stderr, "sending enableBLOB %s.%s\n", dev, nam);
	fprintf (svrwfp,"<enableBLOB device='%s' name='%s'>Also</enableBLOB>\n",
								    dev, nam);
	fflush (svrwfp);
}

/* given a oneBLOB, save
 */
static void
oneBLOB (XMLEle *root, const char *dev, const char *nam, const char *enam, char *p, int plen)
{
	const char *format;
	FILE *fp;
	int bloblen;
	unsigned char *blob;
	int ucs;
	int isz;
	char fn[128];
	int i;

	/* get uncompressed size */
	ucs = atoi(findXMLAttValu (root, "size"));
	if (verbose)
	    fprintf (stderr, "%s.%s.%s reports uncompressed size as %d\n",
							dev, nam, enam, ucs);

	/* get format and length */
	format = findXMLAttValu (root, "format");
	isz = !strcmp (&format[strlen(format)-2], ".z");

	/* decode blob from base64 in p */
	blob = malloc (3*plen/4);
	bloblen = from64tobits ((char *)blob, p);
	if (bloblen < 0) {
	    fprintf (stderr, "%s.%s.%s bad base64\n", dev, nam, enam);
	    exit(2);
	}

	/* uncompress effectively in place if z */
	if (isz) {
	    uLong nuncomp = ucs;
	    unsigned char *uncomp = malloc (ucs);
	    int ok = uncompress (uncomp, &nuncomp, blob, bloblen);
	    if (ok != Z_OK) {
		fprintf (stderr, "%s.%s.%s uncompress error %d\n", dev, nam,
								    enam, ok);
		exit(2);
	    }
	    free (blob);
	    blob = uncomp;
	    bloblen = nuncomp;
	}

	/* rig up a file name from property name */
	i = sprintf (fn, "%s.%s.%s%s", dev, nam, enam, format);
	if (isz)
	    fn[i-2] = '\0'; 	/* chop off .z */

	/* save */
	fp = fopen (fn, "w");
	if (fp) {
	    if (verbose)
		fprintf (stderr, "Wrote %s\n", fn);
	    fwrite (blob, bloblen, 1, fp);
	    fclose (fp);
	} else {
	    fprintf (stderr, "%s: %s\n", fn, strerror(errno));
	}

	/* clean up */
	free (blob);
}

/* send the given set specification of the given INDI type to channel on fp */
static void
sendNew (FILE *fp, set_INDIDef *dp, SetPars *sp)
{
	int i;

	fprintf (fp, "<%s device='%s' name='%s'>\n", dp->newType, sp->d, sp->p);
	for (i = 0; i < sp->nev; i++) {
	    if (verbose)
	    {
			fprintf (stderr, "  %s.%s.%s <- %s\n", sp->d, sp->p,
						    sp->ev[i].e, sp->ev[i].v);
	    }
	    fprintf (fp, "  <%s name='%s'>%s</%s>\n", dp->oneType, sp->ev[i].e,
						    sp->ev[i].v, dp->oneType);
	}
	fprintf (fp, "</%s>\n", dp->newType);
	fflush (fp);
	if (feof(fp) || ferror(fp)) {
	    fprintf (stderr, "Send error\n");
	    exit(2);
	}
}


/* send each SetPars, all of which have a known type, to wfp
 */
static void
sendSpecs(FILE *wfp, SetPars *fsets, int fnsets)
{
	int i;

	for (i = 0; i < fnsets; i++)
	{
//	    fprintf( stderr, "Dev %s\n", sets[i].d);
//	    fprintf( stderr, "Pro %s\n", sets[i].p);
//	    fprintf( stderr, "Eln %s\n", sets[i].ev->e);
//	    fprintf( stderr, "Elv %s\n", sets[i].ev->v);
//	    if( sets[i].dp !=NULL)
//	    {
//		fprintf( stderr, "Elt %s\n", sets[i].dp->defType);
//	    }
	    sendNew (wfp, fsets[i].dp, &fsets[i]);
	}

}
void
free_getINDIproperty( SearchDef *csrchs, int cnsrchs)
{
    int i= 0 ;
    int j= 0 ;
    //fprintf( stderr, "free_getINDIproperty: %d\n", cnsrchs) ;
    for(i=0; i < cnsrchs ; i++) //wildi
    { 
	//fprintf( stderr, "FREEXXXXX %s, %s\n", csrchs[i].d,  csrchs[i].p) ;
	free( csrchs[i].d) ;
	free( csrchs[i].p) ;
	free( csrchs[i].e) ;

	for( j=0 ; j < csrchs[i].ngev ; j++ )
	{
	    //fprintf( stderr, "FREEEEEEEEEEEEEEEEEEEE %s, %s\n", csrchs[i].gev[j].ge,  csrchs[i].gev[j].gv) ;
	    free(csrchs[i].gev[j].ge) ;
	    free(csrchs[i].gev[j].gv) ;     
	}
	free ( srchs[i].gev) ; // wildi wrong ?
	free ( srchs[i].pstate) ;
    }
    free( csrchs) ; // wildi wrong ?
}
SearchDef *malloc_getINDIproperty()
{ 
    if (!(srchs))
    {
	(srchs) = (SearchDef *) malloc (1);		/* realloc seed */
	nsrchs= 0 ;
    } 
    srchs = (SearchDef *) realloc ( (srchs), ((nsrchs) + 1)*sizeof(SearchDef));
    //fprintf( stderr, "malloc_getINDIproperty REALLOC %p\n",csrchs) ;

    return srchs ;
}

int fill_getINDIproperty( char *dev, char *type, char *prop, char *ele)
{
    //fprintf( stderr, "fill_getINDIproperty REALLOC %p\n",asrchs) ;
    srchs[nsrchs].d = strcpy (malloc(strlen(dev)+1), dev);
    srchs[nsrchs].p = strcpy (malloc(strlen(prop)+1), prop);
    srchs[nsrchs].e = strcpy (malloc(strlen(ele)+1), ele);
    //srchs[nsrchs].v= NULL ;
    srchs[nsrchs].gev = (GetEV *) malloc (1);		/* seed realloc */
    srchs[nsrchs].ngev = 0;
    srchs[nsrchs].pstate= malloc( 16) ; //wildi, pstate is one of Ok, Idle, Alert, Busy
    srchs[nsrchs].pstate[0]= '\0' ;

    srchs[nsrchs].wc = *dev==WILDCARD || *prop==WILDCARD || *ele==WILDCARD;
    srchs[nsrchs].ok = 0;
    nsrchs++ ;

    return 0 ; // ToDo check return values malloc
}

void free_setINDIproperty( SetPars *csets, int ncsets)
{
    int i ;

    free( csets->d) ;
    free( csets->p) ;
    for(i=0 ; i < csets->nev ; i++) // done 
    {
	free( csets->ev[i].e) ;
	free( csets->ev[i].v) ;

    }
    free( csets->ev) ;
    free( csets) ;

    return ;
}

SetPars *malloc_setINDIproperty( SetPars *csets, int *ncsets)
{ 
    //fprintf( stderr, "malloc_setINDIproperty  SEED %p\n", csets) ;
    if (!(csets))
    {
	(csets) = (SetPars *) malloc (1);		/* realloc seed */
	*ncsets= 0 ;
	//fprintf( stderr, "malloc_setINDIproperty SEED %p\n", csets) ;

    } 
    csets = (SetPars *) realloc ( (csets), ((*ncsets)+1)*sizeof(SetPars));
    //fprintf( stderr, "malloc_setINDIproperty REALLOC %p\n",csets) ;

    return csets ;
}

int fill_setINDIproperty( char *dev, char *type, char *prop, char *ele, char *val, SetPars *dsets, int *dnsets)
{
    int t ;

    dsets[*dnsets].d = strcpy (malloc(strlen(dev)+1), dev);
    dsets[*dnsets].p = strcpy (malloc(strlen(prop)+1), prop);

    for (t = 0; t < NDEFS; t++)
	if (strcmp (type, set_defs[t].defType) == 0)
	    break;
    if (t == NDEFS)
    {
	fprintf( stderr, "NO type found %d//%d\n", (int)t, (int)NDEFS) ;
	return -1; /* ToDo do that better */
    }

    dsets[*dnsets].dp = &set_defs[t];
    dsets[*dnsets].ev = (SetEV *) malloc (1);		/* seed realloc */
    dsets[*dnsets].nev = 0;


// wildi was   scanEV (&sets[nsets++], ele, val);

    static char sep[] = ";";
    char *ep, *vp;

    if (verbose > 1)
	fprintf (stderr, "11Scanning %s = %s, sep %s\n", ele, val, sep);
    char ee[1024] ;
    char vv[1024] ;

    strcpy( ee, ele) ;
    strcpy( vv, val) ;

    char *e0 = strtok_r (ee, sep, &ep); 
    char *v0 = strtok_r (vv, sep, &vp); 

    while (1) 
    {
	if (!e0 && !v0)
	    break;
	if (!e0) {
	    fprintf (stderr, "More values than elements for %s.%s\n",
		     sets[*dnsets].d, sets[*dnsets].p);
	    exit(2);
	}
	if (!v0) {
	    fprintf (stderr, "More elements than values for %s.%s\n",
								sets[*dnsets].d, sets[*dnsets].p);
	    exit(2);
	}
	if( verbose > 1)
	    fprintf (stderr, "22Scanning %s = %s\n", ele, val);
	dsets[*dnsets].ev = (SetEV *) realloc (dsets[*dnsets].ev, (dsets[*dnsets].nev+1)*sizeof(SetEV));

	dsets[*dnsets].ev[dsets[*dnsets].nev].e = strcpy (malloc(strlen(e0)+1), e0);
	dsets[*dnsets].ev[dsets[*dnsets].nev].v = strcpy (malloc(strlen(v0)+1), v0);

	dsets[*dnsets].nev++;

	ele = NULL;
	val = NULL;
	e0 = strtok_r (NULL, sep, &ep); 
	v0 = strtok_r (NULL, sep, &vp); 
    }
    ++*dnsets ;

    return 0 ;
}
/* scan e1[;e2...] v1[;v2,...] from e[] and v[] and add to spec sp.
 * exit if trouble.
 */
