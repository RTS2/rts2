// CameraIO.cpp: implementation of the CCameraIO class.
//
// Copyright (c) 2000 Apogee Instruments Inc.
//////////////////////////////////////////////////////////////////////

#define LINUX

#ifndef LINUX
#include "stdafx.h"
#else
#include <assert.h>
#include <sys/io.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#define HANDLE int
#define FALSE 0
#define DWORD long
#define _ASSERT assert
#define REALTIME_PRIORITY_CLASS 1
#define GetCurrentProcess getpid
#define LOBYTE(x) ((x) & 0xff)
#define HIBYTE(x) ((x >> 8) & 0xff)
#endif

#define MIRQ1 0x21
#define MIRQ2 0xA1

#include <time.h>
#include <stdio.h>
#include "CameraIO_Linux.h"
#include "ApogeeLinux.h"

const int NUM_POSITIONS = 6;
const int NUM_STEPS_PER_FILTER = 48;
const int STEP_DELAY = 10;

const unsigned char Steps[] =
{ 0x10, 0x30, 0x20, 0x60, 0x40, 0xc0, 0x80, 0x90 };
const int NUM_STEPS = sizeof (Steps);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCameraIO::CCameraIO ()
{
	InitDefaults ();

	m_TDI = false;

	m_Shutter = false;
	m_FilterPosition = 0;
	m_FilterStepPos = 0;

	m_WaitingforImage = false;
	m_WaitingforLine = false;

	m_WaitingforTrigger = false;
	m_Status = Camera_Status_Idle;
	m_CoolerStatus = Camera_CoolerStatus_Off;

	m_ExposureBinX = 0;
	m_ExposureBinY = 0;
	m_ExposureStartX = 0;
	m_ExposureStartY = 0;
	m_ExposureNumX = 0;
	m_ExposureNumY = 0;
	m_ExposureColumns = 0;
	m_ExposureRows = 0;
	m_ExposureSkipC = 0;
	m_ExposureSkipR = 0;
	m_ExposureHFlush = 0;
	m_ExposureVFlush = 0;
	m_ExposureBIC = 0;
	m_ExposureBIR = 0;
	m_ExposureAIC = 0;
	m_ExposureRemainingLines = 0;
	m_ExposureAIR = 0;

	m_RegShadow[Reg_Command] = 0;
	m_RegShadow[Reg_Timer] = 0;
	m_RegShadow[Reg_VBinning] = 0;
	m_RegShadow[Reg_AICCounter] = 0;
	m_RegShadow[Reg_TempSetPoint] = 0;
	m_RegShadow[Reg_PixelCounter] = 0;
	m_RegShadow[Reg_LineCounter] = 0;
	m_RegShadow[Reg_BICCounter] = 0;

	m_FastShutterBits_Mode = 0;
	m_FastShutterBits_Test = 0;
	m_IRQMask = 0;
	saveIRQS = 0;

}


CCameraIO::~CCameraIO ()
{
	::close (fileHandle);
}


////////////////////////////////////////////////////////////
// System methods

int
GetPriorityClass (__attribute__ ((unused)) HANDLE hProcess)
{
	int i;
	i = sched_getscheduler (0);
	return (i);
}


int
SetPriorityClass (__attribute__ ((unused)) HANDLE hProcess, int hPriority)
{
	sched_param p;

	if (hPriority)
	{
		sched_setscheduler (0, SCHED_RR, &p);
	}
	else
	{
		sched_setscheduler (0, SCHED_OTHER, &p);
	}
	return 1;
}


void
Sleep (__attribute__ ((unused)) int hTime)
{
	//timespec t;
	//t.tv_sec = 0;
	//t.tv_nsec = hTime * 1000000;
	//    nanosleep(&t);
}


void
ATLTRACE (__attribute__ ((unused)) const char *msg)
{
}


void
CCameraIO::Reset ()
{
	unsigned short val = 0;
								 // Take snapshot of currrent status
	Read (Reg_CommandReadback, val);
								 // remember it in our write shadow
	m_RegShadow[Reg_Command] = val;

	// In case these were left on, turn them off
								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_FIFOCache;
								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_TDIMode;

								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_ResetSystem;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_ResetSystem;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

	m_WaitingforImage = false;
	m_WaitingforLine = false;
	m_WaitingforTrigger = false;
}


void
CCameraIO::AuxOutput (unsigned char val)
{
	// clear bits to 0
	m_RegShadow[Reg_TempSetPoint] &=
		~(RegBitMask_PortControl << RegBitShift_PortControl);

	// set our new bits
	m_RegShadow[Reg_TempSetPoint] |= val << RegBitShift_PortControl;

	Write (Reg_TempSetPoint, m_RegShadow[Reg_TempSetPoint]);
}


// Input reg is from 0 to 7, val is any 16 bit number
void
CCameraIO::RegWrite (short reg, unsigned short val)
{
	Write (reg, val);

	// Update our shadow register
	switch (reg)
	{
		case Reg_Command:
			m_RegShadow[Reg_Command] = val;
			break;
		case Reg_Timer:
			m_RegShadow[Reg_Timer] = val;
			break;
		case Reg_VBinning:
			m_RegShadow[Reg_VBinning] = val;
			break;
		case Reg_AICCounter:
			m_RegShadow[Reg_AICCounter] = val;
			break;
		case Reg_TempSetPoint:
			m_RegShadow[Reg_TempSetPoint] = val;
			break;
		case Reg_PixelCounter:
			m_RegShadow[Reg_PixelCounter] = val;
			break;
		case Reg_LineCounter:
			m_RegShadow[Reg_LineCounter] = val;
			break;
		case Reg_BICCounter:
			m_RegShadow[Reg_BICCounter] = val;
			break;
		default:
			_ASSERT (FALSE);	 // application program bug
	}
}


// Input reg is from 8 to 12, returned val is any 16 bit number
void
CCameraIO::RegRead (short reg, unsigned short &val)
{
	Read (reg, val);
}


bool
CCameraIO::FilterHome ()
{
	HANDLE hProcess;
	DWORD Class;

	if (m_HighPriority)
	{							 // Store current process class and priority
		hProcess = GetCurrentProcess ();
		Class = GetPriorityClass (hProcess);
		SetPriorityClass (hProcess, REALTIME_PRIORITY_CLASS);
	}

	// Find the home position
	m_FilterPosition = 0;
	int Safety = 0;
	for (int I = 0; I < NUM_POSITIONS * NUM_STEPS_PER_FILTER * 2; I++)
	{
		// Advance the filter one step
		m_FilterStepPos += 1;
		if (m_FilterStepPos >= NUM_STEPS)
			m_FilterStepPos = 0;
		unsigned char Step = Steps[m_FilterStepPos];

		AuxOutput (Step);
		Sleep (STEP_DELAY);

		// Check for strobe
		unsigned short val = 0;
		Read (Reg_Status, val);
		if (val & RegBit_GotTrigger)
		{
			// Cycle all the way around if it's on the first time
			if (I < NUM_STEPS_PER_FILTER)
			{
				if (++Safety > NUM_STEPS_PER_FILTER * 2)
				{
					// Restore normal priority
					if (m_HighPriority)
						SetPriorityClass (hProcess, Class);
					return false;
				}
				I = 0;
				continue;
			}

			// Continue cycling until we get clear of the opto mirror
			for (int J = 0; J < NUM_STEPS_PER_FILTER; J++)
			{
				// Advance the filter one step
				m_FilterStepPos += 1;
				if (m_FilterStepPos >= NUM_STEPS)
					m_FilterStepPos = 0;
				unsigned char Step2 = Steps[m_FilterStepPos];

				AuxOutput (Step2);
				Sleep (STEP_DELAY);

				val = 0;
				Read (Reg_Status, val);
				if (val & RegBit_GotTrigger)
				{
					Sleep (10);

					val = 0;
					Read (Reg_Status, val);
					if (val & RegBit_GotTrigger)
					{
						// Restore normal priority
						if (m_HighPriority)
							SetPriorityClass (hProcess, Class);
						return true;
					}
				}
			}

			// Restore normal priority
			if (m_HighPriority)
				SetPriorityClass (hProcess, Class);
			return true;
		}
	}

	// Restore normal priority
	if (m_HighPriority)
		SetPriorityClass (hProcess, Class);
	return false;
}


void
CCameraIO::FilterSet (short Slot)
{
	// Determine how far we have to move
	int Pos = Slot - m_FilterPosition;
	if (Pos < 0)
		Pos += NUM_POSITIONS;

	HANDLE hProcess;
	DWORD Class;

	if (m_HighPriority)
	{							 // Store current process class and priority
		hProcess = GetCurrentProcess ();
		Class = GetPriorityClass (hProcess);
		SetPriorityClass (hProcess, REALTIME_PRIORITY_CLASS);
	}

	for (int I = 0; I < Pos; I++)
	{
		// Advance one position
		for (int J = 0; J < NUM_STEPS_PER_FILTER; J++)
		{
			m_FilterStepPos += 1;
			if (m_FilterStepPos >= NUM_STEPS)
				m_FilterStepPos = 0;
			unsigned char Step = Steps[m_FilterStepPos];

			AuxOutput (Step);
			Sleep (STEP_DELAY);
		}
	}

	if (m_HighPriority)
		SetPriorityClass (hProcess, Class);

	m_FilterPosition = Slot;
}


////////////////////////////////////////////////////////////
// Normal exposure methods

bool
CCameraIO::Expose (double Duration, bool Light)
{
	if (!m_TDI && (Duration < m_MinExposure || Duration > m_MaxExposure))
		return false;

	// Validate all input variables
	if (m_Columns < 1 || m_Columns > MAXCOLUMNS)
		return false;
	m_ExposureColumns = m_Columns;

	if (m_Rows < 1 || m_Rows > MAXROWS)
		return false;
	m_ExposureRows = m_Rows;

	if (m_SkipC < 0)
		return false;
	m_ExposureSkipC = m_SkipC;

	if (m_SkipR < 0)
		return false;
	m_ExposureSkipR = m_SkipR;

	if (m_HFlush < 1 || m_HFlush > MAXHBIN)
		return false;
	m_ExposureHFlush = m_HFlush;

	if (m_VFlush < 1 || m_VFlush > MAXVBIN)
		return false;
	m_ExposureVFlush = m_VFlush;

	if (m_BIC < 1 || m_BIC > MAXCOLUMNS)
		return false;
	m_ExposureBIC = m_BIC;

	if (m_BIR < 1 || m_BIR > MAXROWS)
		return false;
	m_ExposureBIR = m_BIR;

	// Validate all input variables
	if (m_BinX < 1 || m_BinX > MAXHBIN)
		return false;
	m_ExposureBinX = m_BinX;

	if (m_StartX < 0 || m_StartX >= MAXCOLUMNS)
		return false;
	m_ExposureStartX = m_StartX;

	if (m_NumX < 1 || m_NumX * m_BinX > m_ImgColumns)
		return false;
	m_ExposureNumX = m_NumX;

	// Calculate BIC, RawPixelCount, AIC
								 // unbinned columns
	unsigned short BIC = m_ExposureBIC + m_ExposureStartX;
	unsigned short RawPixelCount = m_ExposureNumX * m_ExposureBinX;
								 // unbinned columns
	m_ExposureAIC = m_ExposureColumns - BIC - RawPixelCount;

	if (m_BinY < 1 || m_BinY > MAXVBIN)
		return false;
	m_ExposureBinY = m_BinY;

	unsigned short VBin, row_offset;

	if (m_TDI)
	{							 // row_offset is the drift time in milliseconds when in TDI mode
		row_offset = (unsigned short) (Duration * 1000 + 0.5);
		Duration = 0.0;
	}
	else
	{
		if (m_StartY < 0 || m_StartX >= MAXROWS)
			return false;
		m_ExposureStartY = m_StartY;

		if (m_NumY < 1 || m_NumY * m_BinY > m_ImgRows)
			return false;
		m_ExposureNumY = m_NumY;

								 // unbinned rows
		unsigned short BIR = m_ExposureBIR + m_ExposureStartY;
		if (BIR >= MAXROWS)
			return false;
								 // unbinned rows
		m_ExposureAIR = m_ExposureRows - BIR - m_ExposureNumY * m_ExposureBinY;

		if (m_VFlush > BIR)
		{
			VBin = BIR;
			m_ExposureRemainingLines = 0;
		}
		else
		{
			VBin = m_VFlush;
								 // unbinned rows
			m_ExposureRemainingLines = BIR % VBin;
		}
								 // unbinned rows
		row_offset = BIR - m_ExposureRemainingLines;
	}

	StopFlushing ();
	Reset ();

	LoadColumnLayout (m_ExposureAIC, BIC,
		(unsigned short) m_ExposureNumX + m_ExposureSkipC);
	LoadTimerAndBinning (Duration, (unsigned short) m_ExposureHFlush, VBin);
	LoadLineCounter (row_offset);

	if (m_TDI)
	{
		// Turn on TDI
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_TDIMode;

		// Disable FIFO cache
								 // set bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_FIFOCache;

		// Set shutter override
		if (Light)
								 // set bit to 1
			m_RegShadow[Reg_Command] |= RegBit_ShutterOverride;
		else
								 // set bit to 0
			m_RegShadow[Reg_Command] &= ~RegBit_ShutterOverride;

		Write (Reg_Command, m_RegShadow[Reg_Command]);

		// Update our status
		m_Shutter = Light;
		m_WaitingforTrigger = false;
		m_WaitingforLine = false;
	}
	else
	{
		// Set shutter
		if (Light)
								 // set bit to 1
			m_RegShadow[Reg_Command] |= RegBit_ShutterEnable;
		else
								 // set bit to 0
			m_RegShadow[Reg_Command] &= ~RegBit_ShutterEnable;

		Write (Reg_Command, m_RegShadow[Reg_Command]);

		// Update our status
		unsigned short val = 0;
		Read (Reg_CommandReadback, val);
		if (val & RegBit_ShutterOverride)
			m_Shutter = true;
		else
			m_Shutter = Light;

		if ((val & RegBit_TriggerEnable))
			m_WaitingforTrigger = true;
		else
			m_WaitingforTrigger = false;

		// Start the exposure
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_StartTimer;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_StartTimer;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

		m_WaitingforImage = true;
	}

	return true;
}


bool
CCameraIO::BufferImage (__attribute__ ((unused)) char *bufferName)
{
	unsigned short *pImageData;
	bool status;
	short xSize, ySize;
// WTF???
	pImageData = NULL;			 //(unsigned short *)CCD_locate_buffer(bufferName, 2 , cols, rows, hbin, vbin );
	if (pImageData == NULL)
	{
		return 0;
	}

	status = GetImage (pImageData, xSize, ySize);
	return status;
}


bool
CCameraIO::GetImage (unsigned short *pImageData, short &xSize, short &ySize)
{
	int i;
	unsigned short BIC = m_ExposureBIC + m_ExposureStartX;

	// Update internal variables in case application did not poll read_Status
	m_WaitingforTrigger = false;
	m_WaitingforLine = false;

	if (m_WaitingforImage)
	{							 // In case application did not poll read_Status
		m_WaitingforImage = false;

		/////////////////////////////////////
		// Wait until camera is done flushing
								 // wait at most m_Timeout seconds
		clock_t StopTime = clock () + long (m_Timeout * CLOCKS_PER_SEC);
		while (true)
		{
			unsigned short val = 0;
			Read (Reg_Status, val);
			if ((val & RegBit_FrameDone) != 0)
				break;

			if (clock () > StopTime)
				return false;	 // Timed out
		}
	}

	//        MaskIrqs();

	/////////////////////////////////////
	// Update our internal status
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);
	if (!(val & RegBit_ShutterOverride))
		m_Shutter = false;

	StopFlushing ();
	LoadColumnLayout (m_ExposureAIC, BIC,
		(unsigned short) m_ExposureNumX + m_ExposureSkipC);

	if (m_ExposureRemainingLines > 0)
	{
		LoadTimerAndBinning (0.0, m_ExposureHFlush, m_ExposureRemainingLines);

		/////////////////////////////////////
		// Clock out the remaining lines
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_StartNextLine;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_StartNextLine;
		Write (Reg_Command, m_RegShadow[Reg_Command]);
		/////////////////////////////////////

		/////////////////////////////////////
		// Wait until camera is done clocking
								 // wait at most one second
		clock_t StopTime = clock () + CLOCKS_PER_SEC;
		while (true)
		{
			unsigned short val2 = 0;
			Read (Reg_Status, val2);
			if ((val2 & RegBit_LineDone) != 0)
				break;			 // Line done

			if (clock () > StopTime)
			{
				Flush ();
				return false;	 // Timed out, no image available
			}
		}
	}

	LoadTimerAndBinning (0.0, m_ExposureBinX, m_ExposureBinY);

	bool ret = false;			 // assume failure

	// NB Application must have allocated enough memory or else !!!
	if (pImageData != NULL)
	{
		HANDLE hProcess;
		DWORD Class;

		if (m_HighPriority)
		{						 // Store current process class and priority
			hProcess = GetCurrentProcess ();
			Class = GetPriorityClass (hProcess);
			SetPriorityClass (hProcess, REALTIME_PRIORITY_CLASS);
		}

								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_FIFOCache;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

		long XPixels = long (m_ExposureNumX);
		long SkipPixels = long (m_ExposureSkipC);
		for (i = 0; i < m_ExposureSkipR; i++)
		{
			if (ReadLine (SkipPixels, XPixels, pImageData))
				break;
		}

		if (i == m_ExposureSkipR)
		{						 // We have skipped all the lines
			long YPixels = long (m_ExposureNumY);
			unsigned short *pLineBuffer = pImageData;
			for (i = 0; i < YPixels; i++)
			{
				if (ReadLine (SkipPixels, XPixels, pLineBuffer))
					break;
				pLineBuffer += XPixels;
			}

			if (i == YPixels)
				ret = true;		 // We have read all the lines
		}

								 // set bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_FIFOCache;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

		//Restore priority
		if (m_HighPriority)
			SetPriorityClass (hProcess, Class);
	}

	//        UnmaskIrqs();

	if (ret)
	{							 // We were successfull
		Flush (m_ExposureAIR);	 // flush after imaging rows

		xSize = m_ExposureNumX;
		ySize = m_ExposureNumY;

		if (m_DataBits == 16)
		{						 // Take care of two's complement converters
			unsigned short *Ptr = pImageData;
			short *Ptr2 = (short *) pImageData;
			long Size = m_ExposureNumX * m_ExposureNumY;
			for (i = 0; i < Size; i++)
			{
				*Ptr++ = (unsigned short) *Ptr2++ + 32768;
			}
		}

	}
	else
	{							 // Something went wrong
		xSize = 0;
		ySize = 0;
	}

	Flush ();					 // start normal flushing

	return ret;
}


////////////////////////////////////////////////////////////
// Drift scan methods

bool
CCameraIO::DigitizeLine ()
{
	/////////////////////////////////////
	// All of these are done just in case
	// since they are called in Expose()
	StopFlushing ();

	unsigned short BIC = m_ExposureBIC + m_ExposureStartX;
	LoadColumnLayout (m_ExposureAIC, BIC,
		(unsigned short) m_ExposureNumX + m_ExposureSkipC);
	LoadTimerAndBinning (0.0, m_ExposureBinX, m_ExposureBinY);

	// Disable FIFO cache
								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_FIFOCache;
	/////////////////////////////////////

	/////////////////////////////////////
	// Clock out the line
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_StartNextLine;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_StartNextLine;
	Write (Reg_Command, m_RegShadow[Reg_Command]);
	/////////////////////////////////////

	m_WaitingforLine = true;
	return true;
}


bool
CCameraIO::GetLine (unsigned short *pLineData, short &xSize)
{
	int i;

	if (m_WaitingforLine)
	{							 // In case application did not poll read_Status
		m_WaitingforLine = false;

		/////////////////////////////////////
		// Wait until camera is done clocking
								 // wait at most one second
		clock_t StopTime = clock () + CLOCKS_PER_SEC;
		while (true)
		{
			unsigned short val = 0;
			Read (Reg_Status, val);
			if ((val & RegBit_LineDone) != 0)
				break;			 // Line done

			if (clock () > StopTime)
			{
				Flush ();
				return false;	 // Timed out, no line available
			}
		}
	}

	bool ret = false;			 // assume failure

	//        MaskIrqs();

	// NB Application must have allocated enough memory or else !!!
	if (pLineData != NULL)
	{
		HANDLE hProcess;
		DWORD Class;

		if (m_HighPriority)
		{						 // Store current process class and priority
			hProcess = GetCurrentProcess ();
			Class = GetPriorityClass (hProcess);
			SetPriorityClass (hProcess, REALTIME_PRIORITY_CLASS);
		}

		long XPixels = long (m_ExposureNumX);
		long SkipPixels = long (m_ExposureSkipC);

		if (ReadLine (SkipPixels, XPixels, pLineData))
		{						 // Something went wrong
			xSize = 0;
			ret = false;
		}
		else
		{
			xSize = m_ExposureNumX;

			if (m_DataBits == 16)
			{					 // Take care of two's complement converters
				unsigned short *Ptr = pLineData;
				short *Ptr2 = (short *) pLineData;
				long Size = m_ExposureNumX;
				for (i = 0; i < Size; i++)
				{
					*Ptr++ = (unsigned short) *Ptr2++ + 32768;
				}
			}

			ret = true;
		}

		//Restore priority
		if (m_HighPriority)
			SetPriorityClass (hProcess, Class);
	}

	//        UnmaskIrqs();
	return ret;
}


bool
CCameraIO::BufferDriftScan (__attribute__ ((unused)) char *bufferName, int delay, int rowCount,
int nblock)
{
	unsigned short *pImageData, *ptr;
	int irow;
	//short cols, rows, hbin, vbin;
	struct timespec tdrift;
	struct timeval now1, now2;
	struct timezone zone;
	struct sched_param s;
	long waitfor, diff;
	int iblock;

	//cols = m_NumX * m_BinX;
	//rows = rowCount;
	//hbin = m_BinX;
	//vbin = 1;

	pImageData = NULL;			 //(unsigned short *)CCD_locate_buffer(bufferName, 2 , cols, rows, hbin, vbin );
	if (pImageData == NULL)
	{
		return 0;
	}
	ptr = pImageData;
	irow = 0;
	waitfor = delay * 1000;
	sched_setscheduler (0, SCHED_RR, &s);
	while (irow < rowCount + m_NumY)
	{
		iblock = 0;
		while (iblock < nblock && irow < rowCount + m_NumY)
		{
			gettimeofday (&now1, &zone);
			DigitizeLine ();
			GetLine (ptr, m_NumX);
			if (irow >= m_NumY)
			{
				ptr = ptr + m_NumX;
			}
			irow++;
			iblock++;
		}
		tdrift.tv_sec = 0;
		if (waitfor > 20000000)
		{
			tdrift.tv_nsec = waitfor - 20000000;
			nanosleep (&tdrift, NULL);
		}
		gettimeofday (&now2, &zone);
		diff =
			((now2.tv_sec - now1.tv_sec) * 1000000 + now2.tv_usec -
			now1.tv_usec) / 1000;
		tdrift.tv_sec = 0;
		tdrift.tv_nsec = 100000;
		while (diff < waitfor / 1000000)
		{
			nanosleep (&tdrift, NULL);
			gettimeofday (&now2, &zone);
			diff =
				((now2.tv_sec - now1.tv_sec) * 1000000 + now2.tv_usec -
				now1.tv_usec) / 1000;
		}
	}
	sched_setscheduler (0, SCHED_OTHER, &s);

	return 1;
}


////////////////////////////////////////////////////////////
// Easy to use methods

bool
CCameraIO::Snap (double Duration, bool Light, unsigned short *pImageData,
short &xSize, short &ySize)
{
	// NB This also demonstrates how an application might use the
	// Expose and GetImage routines.

	bool ret = Expose (Duration, Light);
	if (!ret)
		return false;

	if (m_WaitingforTrigger)
	{
		Camera_Status stat;
		while (true)
		{						 // This will wait forever if no trigger happens
			stat = read_Status ();
			if (stat == Camera_Status_Exposing)
				break;
			Sleep (220);		 // dont bog down the CPU while polling
		}
		m_WaitingforTrigger = false;
	}

	// Only wait a time slightly greater than the duration of the exposure
	// but enough for the BIR to flush out
	clock_t StopTime =
		clock () + long ((1.2 * Duration + m_Timeout) * CLOCKS_PER_SEC);
	while (true)
	{
		Camera_Status stat = read_Status ();
		if (stat == Camera_Status_ImageReady)
			break;

		if (clock () > StopTime)
			return false;		 // Timed out, no image available
		Sleep (220);			 // dont bog down the CPU while polling
	}

	return GetImage (pImageData, xSize, ySize);
}


////////////////////////////////////////////////////////////
// Camera Settings

Camera_Status
CCameraIO::read_Status ()
{
	unsigned short val = 0;
	Read (Reg_Status, val);

	if (val & RegBit_Exposing)	 //11.0
	{
		ATLTRACE ("Exposing\r\n");
		m_WaitingforTrigger = false;
		m_Status = Camera_Status_Exposing;
	}

	else if (m_WaitingforTrigger)
		m_Status = Camera_Status_Waiting;

								 //11.11
	else if (m_WaitingforImage && (val & RegBit_FrameDone))
	{
		ATLTRACE ("ImageReady\r\n");
		m_WaitingforImage = false;
		m_Status = Camera_Status_ImageReady;
	}

								 //11.1
	else if (m_WaitingforLine && (val & RegBit_LineDone))
	{
		ATLTRACE ("LineReady\r\n");
		m_WaitingforLine = false;
		m_Status = Camera_Status_LineReady;
	}
	else if (m_WaitingforImage || m_WaitingforLine)
	{
		ATLTRACE ("Flushing\r\n");
		m_Status = Camera_Status_Flushing;
	}
	else
		m_Status = Camera_Status_Idle;

	return m_Status;
}


bool
CCameraIO::read_Present ()
{
	// This does not work on all cameras
	/*
	  m_RegShadow[ Reg_BICCounter ] |= RegBit_LoopbackTest;	// set bit to 1
	  Write( Reg_BICCounter, m_RegShadow[ Reg_BICCounter ] );

	  bool FailedLoopback = false;
	  unsigned short val = 0;
	  Read( Reg_Status, val );
	  if ( !( val & RegBit_LoopbackTest ) ) FailedLoopback = true;

	  m_RegShadow[ Reg_BICCounter ] &= ~RegBit_LoopbackTest;	// clear bit to 0
	  Write( Reg_BICCounter, m_RegShadow[ Reg_BICCounter ] );

	  Read( Reg_Status, val );
	  if ( val & RegBit_LoopbackTest ) FailedLoopback = true;
	*/

	unsigned short val = 0;
								 // Take snapshot of currrent status
	Read (Reg_CommandReadback, val);
								 // remember it in our write shadow
	m_RegShadow[Reg_Command] = val;

	bool TriggerEnabled = (val & RegBit_TriggerEnable) != 0;

								 // clear bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_TriggerEnable;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // get currrent status
	Read (Reg_CommandReadback, val);
	if (val & RegBit_TriggerEnable)
		return false;

								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_TriggerEnable;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // get currrent status
	Read (Reg_CommandReadback, val);
	if (!(val & RegBit_TriggerEnable))
		return false;

								 // clear bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_TriggerEnable;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // get currrent status
	Read (Reg_CommandReadback, val);
	if (val & RegBit_TriggerEnable)
		return false;

	if (TriggerEnabled)
	{							 // Set it back the way it was
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_TriggerEnable;
		Write (Reg_Command, m_RegShadow[Reg_Command]);
	}
	return true;
}


bool
CCameraIO::read_Shutter ()
{
	unsigned short regval = 0;
	Read (Reg_Status, regval);
	if (!(regval & RegBit_Exposing))
	{							 // We are not exposing, but might have finnshed an exposure
		// and have not called GetImage yet, so update our internal variable
		regval = 0;
		Read (Reg_CommandReadback, regval);
		if (!(regval & RegBit_ShutterOverride))
			// The shutter override is not on, so the shutter must be closed
			m_Shutter = false;
	}

	return m_Shutter;
}


bool
CCameraIO::read_ForceShutterOpen ()
{
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);
	return ((val & RegBit_ShutterOverride) != 0);
}


void
CCameraIO::write_ForceShutterOpen (bool val)
{
	if (val)
	{
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_ShutterOverride;
		m_Shutter = true;		 // shutter will open immediately now matter what is going on
	}
	else
	{
								 // clear bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_ShutterOverride;

		unsigned short regval = 0;
		Read (Reg_Status, regval);
		if ((regval & RegBit_Exposing))
		{
			// Shutter will remain open if a Light frame is being taken
			// however if a dark frame was being exposed while the
			// override was on or the override is turned on during the exposure
			// and now is turned off (dumb idea but some app might do it!)
			// we must update our variable since the shutter will close
			// when override gets turned off below
			regval = 0;
			Read (Reg_CommandReadback, regval);
			if (!(regval & RegBit_ShutterEnable))
				m_Shutter = false;
		}
		else
		{						 // Not currently exposing so shutter will close
			// once override is turned off, update our variable
			m_Shutter = false;
		}
	}
	Write (Reg_Command, m_RegShadow[Reg_Command]);
}


bool
CCameraIO::read_LongCable ()
{
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);
	return ((val & RegBit_CableLength) != 0);
}


void
CCameraIO::write_Shutter (bool val)
{
	if (val)
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_ShutterEnable;
	else
								 // clear bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_ShutterEnable;

	Write (Reg_Command, m_RegShadow[Reg_Command]);
}


void
CCameraIO::write_LongCable (bool val)
{
	if (val)
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_CableLength;
	else
								 // clear bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_CableLength;

	Write (Reg_Command, m_RegShadow[Reg_Command]);
}


short
CCameraIO::read_Mode ()
{
	return ((m_RegShadow[Reg_LineCounter] >> RegBitShift_Mode) &
		RegBitMask_Mode);
}


void
CCameraIO::write_Mode (short val)
{
	// clear bits to 0
	m_RegShadow[Reg_LineCounter] &= ~(RegBitMask_Mode << RegBitShift_Mode);

	// set our new bits
	m_RegShadow[Reg_LineCounter] |=
		((unsigned short) val & RegBitMask_Mode) << RegBitShift_Mode;

	Write (Reg_LineCounter, m_RegShadow[Reg_LineCounter]);
}


short
CCameraIO::read_TestBits ()
{
	return ((m_RegShadow[Reg_BICCounter] >> RegBitShift_Test) &
		RegBitMask_Test);
}


void
CCameraIO::write_TestBits (short val)
{
	// clear bits to 0
	m_RegShadow[Reg_BICCounter] &= ~(RegBitMask_Test << RegBitShift_Test);

	// set our new bits
	m_RegShadow[Reg_BICCounter] |=
		((unsigned short) val & RegBitMask_Test) << RegBitShift_Test;

	Write (Reg_BICCounter, m_RegShadow[Reg_BICCounter]);
}


short
CCameraIO::read_Test2Bits ()
{
	return ((m_RegShadow[Reg_AICCounter] >> RegBitShift_Test2) &
		RegBitMask_Test2);
}


void
CCameraIO::write_Test2Bits (short val)
{
	// clear bits to 0
	m_RegShadow[Reg_AICCounter] &= ~(RegBitMask_Test2 << RegBitShift_Test2);

	// set our new bits
	m_RegShadow[Reg_AICCounter] |=
		((unsigned short) val & RegBitMask_Test2) << RegBitShift_Test2;

	Write (Reg_AICCounter, m_RegShadow[Reg_AICCounter]);
}


bool
CCameraIO::read_FastReadout ()
{
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);
	return ((val & RegBit_Focus) != 0);
}


void
CCameraIO::write_FastReadout (bool val)
{
	if (val)
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_Focus;
	else
								 // clear bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_Focus;

	Write (Reg_Command, m_RegShadow[Reg_Command]);
}


bool
CCameraIO::read_UseTrigger ()
{
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);
	return ((val & RegBit_TriggerEnable) != 0);
}


void
CCameraIO::write_UseTrigger (bool val)
{
	if (val)
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_TriggerEnable;
	else
								 // clear bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_TriggerEnable;

	Write (Reg_Command, m_RegShadow[Reg_Command]);
}


////////////////////////////////////////////////////////////
// Cooler Settings

double
CCameraIO::read_CoolerSetPoint ()
{
	// Get the setting from the shadow registers
	short DACunits =
		short ((m_RegShadow[Reg_TempSetPoint] >> RegBitShift_TempSetPoint) &
		RegBitMask_TempSetPoint);
	return (DACunits - m_TempCalibration) / m_TempScale;
}


void
CCameraIO::write_CoolerSetPoint (double val)
{
	// clear bits to 0
	m_RegShadow[Reg_TempSetPoint] &=
		~(RegBitMask_TempSetPoint << RegBitShift_TempSetPoint);

	// Calculate DAC units from degrees Celcius
	unsigned short DACunits =
		(unsigned) (m_TempScale * val) + m_TempCalibration;

	// set our new bits
	m_RegShadow[Reg_TempSetPoint] |=
		(DACunits & RegBitMask_TempSetPoint) << RegBitShift_TempSetPoint;

	Write (Reg_TempSetPoint, m_RegShadow[Reg_TempSetPoint]);
}


Camera_CoolerStatus
CCameraIO::read_CoolerStatus ()
{
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);

								 //12.15
	if (val & RegBit_CoolerEnable)
	{
		unsigned short val2 = 0;
		Read (Reg_Status, val2);

								 //12.8
		if (val & RegBit_CoolerShutdown)
		{
								 //11.6
			if (val2 & RegBit_ShutdownComplete)
				m_CoolerStatus = Camera_CoolerStatus_AtAmbient;
			else
				m_CoolerStatus = Camera_CoolerStatus_RampingToAmbient;
		}
		else
		{
								 //11.5
			if (val2 & RegBit_TempAtMax)
				m_CoolerStatus = Camera_CoolerStatus_AtMax;
								 //11.4
			else if (val2 & RegBit_TempAtMin)
				m_CoolerStatus = Camera_CoolerStatus_AtMin;
								 //11.7
			else if (val2 & RegBit_TempAtSetPoint)
				m_CoolerStatus = Camera_CoolerStatus_AtSetPoint;
			// Check against last known cooler status
			else if (m_CoolerStatus == Camera_CoolerStatus_AtSetPoint)
				m_CoolerStatus = Camera_CoolerStatus_Correcting;
			else
				m_CoolerStatus = Camera_CoolerStatus_RampingToSetPoint;
		}
	}
	else
		m_CoolerStatus = Camera_CoolerStatus_Off;

	return m_CoolerStatus;
}


Camera_CoolerMode
CCameraIO::read_CoolerMode ()
{
	unsigned short val = 0;
	Read (Reg_CommandReadback, val);

	if (val & RegBit_CoolerShutdown)
		return Camera_CoolerMode_Shutdown;
	else if (val & RegBit_CoolerEnable)
		return Camera_CoolerMode_On;
	else
		return Camera_CoolerMode_Off;
}


void
CCameraIO::write_CoolerMode (Camera_CoolerMode val)
{
	switch (val)
	{
		case Camera_CoolerMode_Off:
								 // clear bit to 0
			m_RegShadow[Reg_Command] &= ~(RegBit_CoolerEnable);
								 // clear bit to 0
			m_RegShadow[Reg_Command] &= ~(RegBit_CoolerShutdown);
			break;
		case Camera_CoolerMode_On:
								 // set bit to 1
			m_RegShadow[Reg_Command] |= RegBit_CoolerEnable;
			break;
		case Camera_CoolerMode_Shutdown:
								 // set bit to 1
			m_RegShadow[Reg_Command] |= RegBit_CoolerShutdown;
			break;
		default:
			return;
	}

	Write (Reg_Command, m_RegShadow[Reg_Command]);
}


double
CCameraIO::read_Temperature ()
{
	if (m_TempScale == 0.0)
		return 0.0;
	else
	{
		unsigned short val = 0;
		Read (Reg_TempData, val);

		short DACunits =
			short ((val >> RegBitShift_TempData) & RegBitMask_TempData);

		return (DACunits - m_TempCalibration) / m_TempScale;
	}
}


// Load line counter
void
CCameraIO::LoadLineCounter (unsigned short rows)
{
	/////////////////////////////////////
	// Write out Line_Count  - in unbinned rows
	// clear bits to 0
	m_RegShadow[Reg_LineCounter] &=
		~(RegBitMask_LineCounter << RegBitShift_LineCounter);
	// set our new bits
	m_RegShadow[Reg_LineCounter] |=
		(rows & RegBitMask_LineCounter) << RegBitShift_LineCounter;

	Write (Reg_LineCounter, m_RegShadow[Reg_LineCounter]);
	/////////////////////////////////////
}


// Load AIC, BIC and pixel count into registers
void
CCameraIO::LoadColumnLayout (unsigned short aic, unsigned short bic,
unsigned short pixels)
{
	/////////////////////////////////////
	// Write out AIC - in unbinned columns
	// clear bits to 0
	m_RegShadow[Reg_AICCounter] &=
		~(RegBitMask_AICCounter << RegBitShift_AICCounter);
	// set our new bits
	m_RegShadow[Reg_AICCounter] |=
		(aic & RegBitMask_AICCounter) << RegBitShift_AICCounter;

	Write (Reg_AICCounter, m_RegShadow[Reg_AICCounter]);
	/////////////////////////////////////

	/////////////////////////////////////
	// Write out BIC - in unbinned columns
	// clear bits to 0
	m_RegShadow[Reg_BICCounter] &=
		~(RegBitMask_BICCounter << RegBitShift_BICCounter);
	// set our new bits
	m_RegShadow[Reg_BICCounter] |=
		(bic & RegBitMask_BICCounter) << RegBitShift_BICCounter;

	Write (Reg_BICCounter, m_RegShadow[Reg_BICCounter]);
	/////////////////////////////////////

	/////////////////////////////////////
	// Write out pixel count - in binned columns
	// clear bits to 0
	m_RegShadow[Reg_PixelCounter] &=
		~(RegBitMask_PixelCounter << RegBitShift_PixelCounter);
	// set our new bits
	m_RegShadow[Reg_PixelCounter] |=
		(pixels & RegBitMask_PixelCounter) << RegBitShift_PixelCounter;

	Write (Reg_PixelCounter, m_RegShadow[Reg_PixelCounter]);
	/////////////////////////////////////
}


// Load timer, vertical binning and horizontal binning in to registers
// If Duration parameter is 0 the current timer value will be retained.
// The VBin and HBin parameters are one based, the HBin value
// is converted to zero base inside this routine.
void
CCameraIO::LoadTimerAndBinning (double Duration, unsigned short HBin,
unsigned short VBin)
{
	/////////////////////////////////////
	// Write out HBin for flushing
	// clear bits to 0
	m_RegShadow[Reg_PixelCounter] &=
		~(RegBitMask_HBinning << RegBitShift_HBinning);
	// set our new bits
	m_RegShadow[Reg_PixelCounter] |=
		((HBin - 1) & RegBitMask_HBinning) << RegBitShift_HBinning;

	Write (Reg_PixelCounter, m_RegShadow[Reg_PixelCounter]);
	/////////////////////////////////////

	/////////////////////////////////////
	// Write out VBin for flushing and Timer
	if (Duration > 0.0)
	{
		if (Duration > m_MaxExposure)
			Duration = m_MaxExposure;

		long valTimer;
		if (m_FastShutter && Duration <= 1048.575)
		{						 // Automatically switch to high precision mode
			valTimer = long ((Duration * 1000) + 0.5);
			m_RegShadow[Reg_LineCounter] |=
				(m_FastShutterBits_Mode & RegBitMask_Mode) << RegBitShift_Mode;
			m_RegShadow[Reg_BICCounter] |=
				(m_FastShutterBits_Test & RegBitMask_Test) << RegBitShift_Test;
		}
		else
		{
			valTimer = long ((Duration * 100) + 0.5);
			if (m_FastShutter)
			{					 // Disable high precision mode
				m_RegShadow[Reg_LineCounter] &=
					~(m_FastShutterBits_Mode & RegBitMask_Mode) <<
					RegBitShift_Mode;
				m_RegShadow[Reg_BICCounter] &=
					~(m_FastShutterBits_Test & RegBitMask_Test) <<
					RegBitShift_Test;
			}
		}

		if (m_FastShutter)
		{
			Write (Reg_LineCounter, m_RegShadow[Reg_LineCounter]);
			Write (Reg_BICCounter, m_RegShadow[Reg_BICCounter]);
		}

		if (valTimer <= 0)
			valTimer = 1;		 // Safety since firmware doesnt like zero

		unsigned short valTimerLow = (unsigned short) (valTimer & 0x0000FFFF);
		unsigned short valTimerHigh = (unsigned short) (valTimer >> 16);

		// Enable loading of timer values
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_TimerLoad;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

		// clear bits to 0
		m_RegShadow[Reg_Timer] = 0;
		// set our new bits
		m_RegShadow[Reg_Timer] |=
			(valTimerLow & RegBitMask_Timer) << RegBitShift_Timer;
		Write (Reg_Timer, m_RegShadow[Reg_Timer]);

		// clear bits to 0
		m_RegShadow[Reg_VBinning] = 0;

		// set our new bits
		m_RegShadow[Reg_VBinning] |=
			(VBin & RegBitMask_VBinning) << RegBitShift_VBinning;
		m_RegShadow[Reg_VBinning] |=
			(valTimerHigh & RegBitMask_Timer2) << RegBitShift_Timer2;
		Write (Reg_VBinning, m_RegShadow[Reg_VBinning]);

		// Disable loading of timer values
								 // set bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_TimerLoad;
		Write (Reg_Command, m_RegShadow[Reg_Command]);
		/////////////////////////////////////
	}
	else
	{
		// clear bits to 0
		m_RegShadow[Reg_VBinning] &=
			~(RegBitMask_VBinning << RegBitShift_VBinning);

		// set our new bits
		m_RegShadow[Reg_VBinning] |=
			(VBin & RegBitMask_VBinning) << RegBitShift_VBinning;
		Write (Reg_VBinning, m_RegShadow[Reg_VBinning]);
	}

}


// Start flushing the entire CCD (rows = -1) or a specific number or rows
void
CCameraIO::Flush (short Rows)
{
	if (Rows == 0)
		return;

	unsigned short AIC = (unsigned short) (m_Columns - m_BIC - m_ImgColumns);
	unsigned short Pixels = (unsigned short) (m_ImgColumns / m_HFlush);
	if (m_ImgColumns % m_HFlush > 0)
		Pixels++;				 // round up if necessary
	LoadColumnLayout (AIC, (unsigned short) m_BIC, Pixels);

	LoadTimerAndBinning (0.0, m_HFlush, m_VFlush);

	if (Rows > 0)
	{
		LoadLineCounter ((unsigned short) Rows);
		StartFlushing ();

		/////////////////////////////////////
		// Wait until camera is done flushing
								 // wait at most m_Timeout seconds
		clock_t StopTime = clock () + long (m_Timeout * CLOCKS_PER_SEC);
		while (true)
		{
			unsigned short val = 0;
			Read (Reg_Status, val);
			if ((val & RegBit_FrameDone) != 0)
				break;

			if (clock () > StopTime)
				break;			 // Timed out
		}
	}
	else
	{
		LoadLineCounter ((unsigned short) m_ImgRows);
		StartFlushing ();
	}
}


void
CCameraIO::StartFlushing ()
{
	/////////////////////////////////////
	// Start flushing
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_StartFlushing;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_StartFlushing;
	Write (Reg_Command, m_RegShadow[Reg_Command]);
	/////////////////////////////////////
}


void
CCameraIO::StopFlushing ()
{
	/////////////////////////////////////
	// Stop flushing
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_StopFlushing;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_StopFlushing;
	Write (Reg_Command, m_RegShadow[Reg_Command]);
	/////////////////////////////////////
}


void
CCameraIO::InitDefaults ()
{
	////////////////////////////////////////////////////////////
	// Camera Settings

	m_HighPriority = true;
	m_PPRepeat = 1;
	m_DataBits = 16;
	m_FastShutter = false;
	m_MaxBinX = 8;
	m_MaxBinY = 63;
	m_MaxExposure = 10485.75;
	m_MinExposure = 0.01;
	m_GuiderRelays = false;
	m_Timeout = 2.0;

	////////////////////////////////////////////////////////////
	// Cooler Settings

	m_TempControl = true;
	m_TempCalibration = 160;
	m_TempScale = 2.1;

	////////////////////////////////////////////////////////////
	// Exposure Settings

	m_BinX = 1;
	m_BinY = 1;
	m_StartX = 0;
	m_StartY = 0;
	m_NumX = 1;
	m_NumY = 1;

	////////////////////////////////////////////////////////////
	// Geometry Settings

	m_Columns = 0;
	m_Rows = 0;
	m_SkipC = 0;
	m_SkipR = 0;
	m_HFlush = 1;
	m_VFlush = 1;
	m_BIC = 4;
	m_BIR = 4;
	m_ImgColumns = 0;
	m_ImgRows = 0;

	////////////////////////////////////////////////////////////
	// CCD Settings

	memset (m_Sensor, 0, 256);
	m_Color = false;
	m_Noise = 0.0;
	m_Gain = 0.0;
	m_PixelXSize = 0.0;
	m_PixelYSize = 0.0;

	////////////////////////////////////////////////////////////
	// Internal variables

	fileHandle = 0;
	m_RegisterOffset = 0;
	m_Interface = Camera_Interface_PCI;
	m_SensorType = Camera_SensorType_CCD;
}


bool
CCameraIO::InitDriver (unsigned short camnum)
{
	char deviceName[64];

	sprintf (deviceName, "%s%d", APOGEE_PCI_DEVICE, camnum);
	fileHandle =::open (deviceName, O_RDONLY);
	if (fileHandle == -1)
		return false;
	return true;
}


long
CCameraIO::Write (unsigned short reg, unsigned short val)
{
	unsigned short RegNumber;
	struct apIOparam request;

	switch (reg)
	{
		case Reg_Command:
			RegNumber = RegPCI_Command;
			break;
		case Reg_Timer:
			RegNumber = RegPCI_Timer;
			break;
		case Reg_VBinning:
			RegNumber = RegPCI_VBinning;
			break;
		case Reg_AICCounter:
			RegNumber = RegPCI_AICCounter;
			break;
		case Reg_TempSetPoint:
			RegNumber = RegPCI_TempSetPoint;
			break;
		case Reg_PixelCounter:
			RegNumber = RegPCI_PixelCounter;
			break;
		case Reg_LineCounter:
			RegNumber = RegPCI_LineCounter;
			break;
		case Reg_BICCounter:
			RegNumber = RegPCI_BICCounter;
			break;
		default:
			_ASSERT (false);
			return 0;
	}
	request.reg = RegNumber;
	request.param1 = (int) val;
	ioctl (fileHandle, APPCI_WRITE_USHORT, (unsigned long) &request);
	return 0;
}


long
CCameraIO::Read (unsigned short reg, unsigned short &val)
{
	int retval;
	struct apIOparam request;
	unsigned short RegNumber;

	switch (reg)
	{
		case Reg_Command:
			RegNumber = RegPCI_CommandRead;
			break;
		case Reg_Timer:
			RegNumber = RegPCI_TimerRead;
			break;
		case Reg_VBinning:
			RegNumber = RegPCI_VBinningRead;
			break;
		case Reg_AICCounter:
			RegNumber = RegPCI_AICCounterRead;
			break;
		case Reg_TempSetPoint:
			RegNumber = RegPCI_TempSetPointRead;
			break;
		case Reg_PixelCounter:
			RegNumber = RegPCI_PixelCounterRead;
			break;
		case Reg_LineCounter:
			RegNumber = RegPCI_LineCounterRead;
			break;
		case Reg_BICCounter:
			RegNumber = RegPCI_BICCounterRead;
			break;
		case Reg_ImageData:
			RegNumber = RegPCI_ImageData;
			break;
		case Reg_TempData:
			RegNumber = RegPCI_TempData;
			break;
		case Reg_Status:
			RegNumber = RegPCI_Status;
			break;
		case Reg_CommandReadback:
			RegNumber = RegPCI_CommandReadback;
			break;
		default:
			assert (1);			 // application program bug
			val = 0;
			return 0;
	}

	request.reg = RegNumber;
	request.param1 = (unsigned long) &retval;
	ioctl (fileHandle, APPCI_READ_USHORT, (unsigned long) &request);
	val = (unsigned short) retval;
	return 0;
}


// Returns 0 if successfull, 1 if Line_Done poll timed out.
long
CCameraIO::ReadLine (long SkipPixels, long Pixels,
unsigned short *pLineBuffer)
{
	int j;
	int retval;
	struct apIOparam request;

	if (!m_TDI)
	{
		/////////////////////////////////////
		// Clock out the line
								 // set bit to 1
		m_RegShadow[Reg_Command] |= RegBit_StartNextLine;
		Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
		m_RegShadow[Reg_Command] &= ~RegBit_StartNextLine;
		Write (Reg_Command, m_RegShadow[Reg_Command]);
		/////////////////////////////////////
	}

	request.reg = RegPCI_ImageData;
	request.param1 = (unsigned long) &retval;

	for (j = 0; j < SkipPixels; j++)
	{
		ioctl (fileHandle, APPCI_READ_USHORT, (unsigned long) &request);
	}
	for (j = 0; j < Pixels; j++)
	{
		ioctl (fileHandle, APPCI_READ_USHORT, (unsigned long) &request);
		*pLineBuffer++ = (unsigned short) retval;
	}
	/////////////////////////////////////
	// Assert done reading line
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_DoneReading;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= ~RegBit_DoneReading;
	Write (Reg_Command, m_RegShadow[Reg_Command]);
	/////////////////////////////////////

	if (!m_TDI)
	{
		/////////////////////////////////////
		// Wait until camera is done
								 // wait at most one second
		clock_t StopTime = clock () + CLOCKS_PER_SEC;
		while (true)
		{
			unsigned short val = 0;
			Read (Reg_Status, val);
			if ((val & RegBit_LineDone) != 0)
				break;			 // Line done

			if (clock () > StopTime)
				return 1;		 // Timed out
		}
	}

	return 0;
}


long
CCameraIO::ReadImage (unsigned short *pImageBuffer)
{
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_FIFOCache;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

	long XEnd = long (m_ExposureNumX);
	long SkipC = long (m_ExposureSkipC);
	for (long i = 0; i < m_ExposureSkipR; i++)
	{
		if (InternalReadLine (false, SkipC, XEnd, NULL))
			return 1;
	}

	long YEnd = long (m_ExposureNumY);
	unsigned short *pLineBuffer = pImageBuffer;
	for (long i = 0; i < YEnd; i++)
	{
		if (InternalReadLine (true, SkipC, XEnd, pLineBuffer))
			return 1;
		pLineBuffer += XEnd;
	}

								 // set bit to 0
	m_RegShadow[Reg_Command] &= !RegBit_FIFOCache;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

	return 0;
}


// Returns 0 if successfull, 1 if Line_Done poll timed out.
long
CCameraIO::InternalReadLine (bool KeepData, long SkipC, long XEnd,
unsigned short *pLineBuffer)
{
	struct apIOparam request;
	int retval;

	/////////////////////////////////////
	// Clock out the line
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_StartNextLine;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= !RegBit_StartNextLine;
	Write (Reg_Command, m_RegShadow[Reg_Command]);
	/////////////////////////////////////

	request.reg = RegPCI_ImageData;
	request.param1 = (unsigned long) &retval;

	for (long j = 0; j < SkipC; j++)
		ioctl (fileHandle, APPCI_READ_USHORT, (unsigned long) &request);

	if (KeepData)
	{
		for (long j = 0; j < XEnd; j++)
		{
			ioctl (fileHandle, APPCI_READ_USHORT, (unsigned long) &request);
			*pLineBuffer++ = (unsigned short) retval;
		}
	}
	else
	{
		for (long j = 0; j < XEnd; j++)
			ioctl (fileHandle, APPCI_READ_USHORT, (unsigned long) &request);
	}

	/////////////////////////////////////
	// Assert done reading line
								 // set bit to 1
	m_RegShadow[Reg_Command] |= RegBit_DoneReading;
	Write (Reg_Command, m_RegShadow[Reg_Command]);

								 // set bit to 0
	m_RegShadow[Reg_Command] &= !RegBit_DoneReading;
	Write (Reg_Command, m_RegShadow[Reg_Command]);
	/////////////////////////////////////

	/////////////////////////////////////
	// Wait until camera is done clocking
								 // wait at most one second
	clock_t StopTime = clock () + CLOCKS_PER_SEC;
	while (true)
	{
		unsigned short val = 0;
		Read (Reg_Status, val);
		if ((val & RegBit_LineDone) != 0)
			break;				 // Line done

		clock_t CurrentTime = clock ();
		if (CurrentTime > StopTime)
			return 1;			 // Timed out
	}

	return 0;
}
