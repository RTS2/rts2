#ifndef _CCD_MACROS_H
#define _CCD_MACROS_H

// parport output cycle duration (in us*16)
extern unsigned short pp_ospeed;	// default let be 1.25 us
// parport input cycle duration
//extern unsigned short pp_ispeed; // default let be 1.25 us

#define IODELAYCONST 0xa
//#define IODELAYCONST 0x0

#define WAIT_4_CAMERA(); \
{\
	t0 = 0;\
	outportb(baseAddress, 0);\
	while(1)\
	{\
		if( (inportb(baseAddress+1)&0x80) == 0 ) break;\
			if(t0++ > 500) return 0xd;\
		lost_ticks++;\
	}\
}
//fprintf(stderr, " %d\n",t0);fflush(stderr);

// CCD_ID a;
#if 0
// This idea is excellent, but udelay seems to be callable only from kernel...?
#define _CAMERA_PULSE(a);	\
	outportb(baseAddress, 0x38 | a);\
	udelay((10 - pp_ospeed) / 16);\
	outportb(baseAddress, 0xb8 | a);\
	udelay((10 - pp_ospeed) / 16);\
	outportb(baseAddress, 0x30 | a);\
	udelay((30 - pp_ospeed) / 16);\
	outportb(baseAddress, 0xb0 | a);\
	udelay((10 - pp_ospeed) / 16);
#endif

#if 0
// For ssslllooowww parallel port, as on my notebook
#define _CAMERA_PULSE(a);	\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0xb8 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0xb0 | a);
#endif

#if 0
// For standard parallel port
#define _CAMERA_PULSE(a);	\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0xb8 | a);\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0xb0 | a);\
	outportb(baseAddress, 0x30 | a);
#endif

#if 1
// For our new fast parallel port (non-standard)
#define _CAMERA_PULSE(a);	\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0xb8 | a);\
	outportb(baseAddress, 0xb8 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0xb0 | a);\
	outportb(baseAddress, 0xb0 | a);
#endif

#ifdef _CCD_IMAGING_C
void
CAMERA_PULSE (int a)
{
  _CAMERA_PULSE (a);
}
#else
void CAMERA_PULSE (int a);
#endif

#endif // _CCD_MACROS_H
