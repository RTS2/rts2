/* -------------------------------------------------------------------------- */
/* -              Header for Planewave Hedric Focuser Programs             - */
/* -------------------------------------------------------------------------- */
/*                                                                            */
/* Copyright 2010 John Kielkopf                                               */
/*                                                                            */
/* Distributed under the terms of the General Public License (see LICENSE)    */
/*                                                                            */
/* John Kielkopf (kielkopf@louisville.edu)                                    */
/*                                                                            */
/* Date: November 10, 2010                                                    */
/* Version: 1.1                                                               */
/*                                                                            */
/* History:                                                                   */
/*                                                                            */
/* October 25, 2010                                                           */
/*   Version 1.0                                                              */
/*     Released                                                               */
/*                                                                            */
/* November 10, 2010                                                          */
/*   Version 1.1                                                              */
/*     Modified for CDK125                                                    */

/* Focus commands are defined in the program file */

/* Focus speed values   */
/*                      */
/* Fast     4           */
/* Medium   3           */
/* Slow     2           */
/* Precise  1           */

/* Focus command values */
/*                      */
/* Out     +1           */
/* In      -1           */
/* Off      0           */
/* Precise  1           */
    
/* Use these for CDK125 */

#define FOCUSSCALE       11.513442 
#define FOCUSDIR            1  

/* Use these for CDK20N */

/* #define FOCUSSCALE       7447. */ 
/* #define FOCUSDIR            1  */ 

/* Use these for CDK20S */

/* #define FOCUSSCALE       1179. */  
/* #define FOCUSDIR           -1  */ 

#define FOCUSERPORT "/dev/ttyS1" 

/* #define FOCUSERPORT "/dev/ttyUSB0" */
