#ifndef _CCD_MACROS_H
#define _CCD_MACROS_H

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
	}\
}

#define _CAMERA_PULSE(a);	\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0xb8 | a);\
	outportb(baseAddress, 0x38 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0x30 | a);\
	outportb(baseAddress, 0xb0 | a);\
	outportb(baseAddress, 0x30 | a);

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
