/* @file Driver file for LX200 telescope
 * 
 * Based on original c library for xephem writen by Ken Shouse
 * <ken@kshouse.engine.swri.edu> and modified by Carlos Guirao
 * <cguirao@eso.org>
 *
 * @author petr 
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/time.h>
#include <strings.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

char *port_dev;	// device name
int port;		// port descriptor
double park_dec;	// parking declination

#define PORT_TIME_OUT 5;	// port timeout

pthread_mutex_t &tel_mutex;	// lock for port access
pthread_mutex_t &mov_mutex;	// lock for moving

/* init to given port
 * 
 * @param devptr Pointer to device name
 * 
 * @return 0 on succes, -1 & set errno otherwise
 */
int init(const char *devptr)
{
  struct termios tel_termios;
  openlog( "LX200", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1 );	// open syslog
  port_dev = malloc( strlen( devptr ) + 1 );
  strcpy( port_dev, devptr );
  port = open( port_dev, O_RDWR );
  if ( open == -1 ) 
	  return -1;
  pthread_mutex_init( tel_mutex, NULL ); // those calls shall never fall
  pthread_mutex_init( mov_mutex, NULL );

  if ( tcgetattr (port, &tel_termios ) == -1 )
	 return -1;
  
  if ( cfsetospeed( &tel_termios, B9600) == -1 ||
		  cfsetispeed( &tel_termios, B9600) == -1 )
	  return -1;
  tel_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
  tel_termios.c_oflag = 0;
  tel_termios.c_cflag = ( tel_termios.c_cflag & ~(TERMIOS.CSIZE) | TERMIOS.CS8 ) & ~(TERMIOS.PARENB | TERMIOS.PARODD);
  tel_termiso.c_lflag = 0;
  tel_termios.c_cc[VMIN] = 1;
  tel_termios.c_cc[VTIME] = 5;

  if ( tcsetattr (port, TCSANOW, &tel_termios) == -1 )
    return -1;

  syslog (LOG_DEBUG, "Initialization complete" );

  return tcflush( port, TCIOFLUSH);
}

/* Reads some data directly from port
 * 
 * Log all flow as LOG_DEBUG to syslog
 * 
 * @param buf buffer to read in data
 * @param count how much data will read
 * @return -1 on failure, otherwise number of read data 
 */
int tel_read ( char *buf, int count )
{
  int readed;
  fd_set pfds;
  struct timeval tv;

  readed = 0;
  tv.sec = PORT_TIME_OUT;
  tv.usec = 0;
  
  while ( readed < count )
  {
    FD_ZERO( &pfds );
    FD_SET( port, &pfds );
    if ( select(port + 1, &pfds, NULL ,NULL , &tv) == 1)
	    return -1;

    if (FD_ISSET( port, &pfds) )
      buf[readed] = read( port, 1);
    else {
      syslog (LOG_DEBUG, "Cannot retrieve data from port" );
      errno =  
      return -1;
			else:
				self._log('telescope',"Cannot retrive data from telescope")
				raise TelescopeError("Cannot retrive data from telescope")
		# if not(self.silence):
		if not(silence):
			self._log('telescope-debug',"readed: "+str(ret))
		return ret		
	
	def _readhash(self,max=100):
		"Internal, will read from port till it will read # character. Return all previou character (do not return ending #)"
		i=0
		last=""
		ret=""
		while i<max and last!="#":
			ret=ret+last
			last=self._read(1,1)
		self._log('telescope-debug',"hash-readed: "+str(ret))
		return ret	
	
	def _write(self,what):
		"Internal, what = what string to send to LX200"
		# if not(self.silence):
		self._log('telescope-debug',"will write: "+str(what))
		return os.write(self._lx200port,what)
	
	def _writeread(self,what,howmuch):
		"""Internal, should be used instead of _write and _read - flush dev, write, read."""
		self._lock.acquire()
		try:
			termios.tcflush(self._lx200port,TERMIOS.TCIOFLUSH)
			self._write(what)
			return self._read(howmuch)
		finally:
			# we need to clear lock in any case
			# don't worry, acording to python doc we do it even if return fails
			self._lock.release()
	
	def _writereadhash(self,what,max):
		"""Similar to _writeread, but will use _readhash for read"""
		self._lock.acquire()
		try:
			termios.tcflush(self._lx200port,TERMIOS.TCIOFLUSH)
			self._write(what)
			return self._readhash(max)
		finally:
			# we need to clear lock in any case
			# don't worry, acording to python doc we do it even if return fails
			self._lock.release()
	
	
	def setrate(self,newRate):
		"Set slew rate, newRate: SLEW or FIND or CENTER or GUIDE"
		if(newRate == SLEW): 
			self._write("#:RS#",5)
		elif(newRate == FIND): 
			self._write("#:RM#",5)
		elif(newRate == CENTER): 
			self._write("#:RC#",5)
		elif(newRate == GUIDE): 
			self._write("#:RG#",5)
	
	def startslew(self,direction):
		"Start slew, direction: NORTH,EAST,SOUTH,WEST"
		if(direction == NORTH):
			self._write("#:Mn#",5)
		elif(direction == EAST):
			self._write("#:Me#",5)
		elif(direction == SOUTH):
			self._write("#:Ms#",5)
		elif(direction == WEST):
			self._write("#:Mw#",5)
	
	def stopslew(self,direction):
		"Stop slew, see also StartSlew"
		if(direction == NORTH):
			self._write("#:Qn#",5)
		elif(direction == EAST):
			self._write("#:Qe#",5)
		elif(direction == SOUTH):
			self._write("#:Qs#",5)
		elif(direction == WEST):
			self._write("#:Qw#",5)
	
	def disconnectlx200(self):
		"Close coonection to LX200"
		try:
			os.close(self._lx200port)
		except:
			print "Cannot close"
	
	def getrahms(self):
		"Get current RA as string"
		returnStr=self._writereadhash("#:GR#",10)
		raHrs=returnStr[:2]
		raMin=returnStr[3:5]
		raSec=returnStr[6:]
		raHrs=float(raHrs)
		raMin=float(raMin)
		raSec=float(raSec)
		return raHrs,raMin,raSec
	
	def getra(self):
		"Get current RA as float number (return value)" 
		raHrs,raMin,raSec=self.getrahms()
		returnVal = float(raHrs) + float(raMin)/60.0 + float(raSec)/3600.0
		return returnVal
	
	def _getdeg(self,command):
		"Reads degrees from telescope"
		returnStr=self._writereadhash(command,10)
		decDeg=float(returnStr[:3])
		decMin=float(returnStr[4:6])
		try:
			decSec=float(returnStr[7:])
		except ValueError:
			decSec=0
		return decDeg,decMin,decSec
	
	def getdecdms(self):
		"get current Dec as three floats (dd,mm,ss)"
		return self._getdeg("#:GD#")
		
	def getdec(self):
		"Get current Dec as float number (return value)"
		decDeg,decMin,decSec=self.getdecdms()
		if(decDeg >= 0.0):
			returnVal = decDeg + decMin/60.0 + decSec/3600.0
		else:	
			returnVal = decDeg - decMin/60.0 - decSec/3600.0
		return returnVal
	
	def _gettime(self,command):
		"Read time from telescope"
		returnStr=self._writereadhash(command,10)
		# get out last #
		# returnStr=returnStr[:-1]
		HH,MM,SS=string.split(returnStr,":")
		return float(HH),float(MM),float(SS)
	
	def localtime(self):
		"Get telescope local time"
		return self._gettime("#:GL#")

	def sideraltime(self):
		"Get telescope sideral time"
		return self._gettime("#:GS#")
	
	def latitude(self):
		"Get the latitude (-90-+90) of the currently selected site"
		return self._getdeg("#:Gt#")
		
	def longtitude(self):
		"Get the longtitude (0-360) of the currently selected site"
		return self._getdeg("#:Gg#")
		
	def _setra(self,ra):
		"Internall, set object ra"
		if (ra>=0) and (ra<=24): 
			raHrs = int(ra)
			raMin = int((ra-raHrs)*60.0)
			raSec = int((ra-raHrs-raMin/60.0)*3600)
			outputStr='#:Sr%02d:%02d:%02d#' % (raHrs,raMin,raSec)
			inputStr='0'
			count = 0 
			while count < 200:		
				inputStr=self._writeread(outputStr,1)
				if inputStr[0] == '1':
					break
				count = count + 1
				self.log('telescope','Rettring setra for ' + str(count) + \
					'time due to incorrect return:' + repr(inputStr))
				time.sleep(1)

			if count >= 200:
				self._log('telescope','Setra unsucessfull due to incorrect return.')
				raise TelescopeError,"Timeout waiting for 1 after Sd"
		else:
			self._log("telescope","Invalid ra to _setra:"+str(ra))
			raise TelescopeError,"Invalid ra:"+str(ra)
	
	def _setdec(self,dec):
		"Internal, set object dec"
		if (abs(dec)<=90): 
			decDeg = int(dec)
			mind = abs(dec-decDeg)*60
			decMin = int(mind)
			decSec = round((mind-decMin)*60,2)
			outputStr='#:Sd%+02d%c%02d:%02d#' % (decDeg,0xdf,decMin,decSec)
			inputStr='0'
			count = 0
			while count < 200:
				inputStr=self._writeread(outputStr,1)
				if inputStr[0] == '1':
					break
					count = count + 1
	
				self.log('telescope','Rettring setra for ' + str(count) + \
					'time due to incorrect return:'+repr(inputStr))
				time.sleep(1)
	
			if count >= 200:
				self._log('telescope','Setdec unsucessfull due to incorrect return.')
				raise TelescopeError,"Timeout waiting for 1 after Sr"
	
		else:
			sel._log("telescope","Invalid dec:"+str(dec))
			raise TelescopeError, "Invalid dec:"+str(dec)

	def _slew(self):	
		inputStr=self._writeread("#:MS#",1)
		if(inputStr[0] == '0'):
			returnVal = 0
		else:
			returnVal = inputStr[0]
			inputStr = "1"
			while(inputStr[0] != '#'):
				inputStr=self._read(1)
		return returnVal
		
	def slewtocoords(self,newRA,newDec):
		"Move (=Slew) to newRA and newDec, which must be floats\n\
		return: 0 =all ok, 1=telescope below horizont, 2=upper limit" 
		self._setra(newRA)
		self._setdec(newDec)
		return self._slew()		

	def checkcoords(self,desRA,desDec):
		"Check, whatewer telescope match given coordinates"
		raError = self.getra() - desRA
		decError = self.getdec() - desDec
		if(abs(raError) > 0.05 or abs(decError) > 0.5):
			return 0
		else:
			return 1

	def _moveto(self,newRA,newDec):
		"Internal, will moto to new RA and dec"
		self._moving_lock.acquire()
		try:
			i=self.slewtocoords(newRA,newDec)
			if (i):
				rtlog.log('telescope','Invalid movement: RA:'+str(newRA)+' DEC:'+str(newDec))
				raise TelescopeInvalidMovement
				return i
			self.silence=1
			timeout=time.time()+100 # timeout, wait 100 seconds to reach target
			while (time.time()<timeout) and (not(self.checkcoords(newRA,newDec))):
				# wait for a bit, telescope get confused?? when you check for ra and dec often and sometime doesn't move to required location
				# this is just a theory, which has to be proven
				time.sleep(2)
				self.silence=0
			if time.time()>timeout:
				msg="Timeout during moving to ra="+str(newRA)+" dec="+str(newDec)+". Actual ra="+str(self.getra())+" dec="+str(self.getdec())
				self._log("telescope",msg)
				raise TelescopeTimeout,msg
		finally:
			self._moving_lock.release()
			
		# need to sleep a while, waiting for final precise adjustement of the scope, which could be checked by checkcoords - the scope newer gets to precise position, it just get to something about that, so we could program checkcoords to check for precise position.  
		time.sleep(5)
		return 0

	def moveto(self,newRA,newDec):
		"""Will move to and wait for completition
		Returns 0: error
						1: succeded"""
		self._log("telescope","moveto: RA:"+str(newRA)+" Dec:"+str(newDec))
		for i in range(0,5):
			try:
				self._moveto(newRA+i*0.05,newDec)
				self._log("telescope","moveto finished")
				return 1 
			except TelescopeInvalidMovement:
				return 0
			except TelescopeTimeout:
				self._log("telescope",str(i)+". timeout during moveto "+str(newRA)+" "+str(newDec))

		msg="Too much timeouts during moving to ra="+str(newRA)+" dec="+str(newDec)+". Actual ra="+str(self.getra())+" dec="+str(self.getdec()+", exiting")
		self._log("telescope",msg)
		raise TelescopeError,msg	
	
	def setto(self,ra,dec):
		"Will set ra and dec) to given values"
		self._setra(ra)
		self._setdec(dec)
		a=self._writereadhash("#:CM#",100)
		if not(self.checkcoords(ra,dec)):
			raise TelescopeError,"Ra and dec not set!! Ra="+str(ra)+" Dec="+str(dec)
		
	
int init(char *port)
{
		"""Is called after sucessfull detection of telescope to set needed values"""
		# we get 12:34:4# while we're in short mode
		# and 12:34:45 while we're in long mode
		a=self._writeread("#:Gr#",8)
		if a[7]=="#":	# could be used to distinquish between long and
									# short mode
			self._writeread("#:U#",0)
		# now set low precision, e.g. we won't wait for user
		# to find a star
		# a="HIGH"
		# while a=="HIGH":
		# 	a=self._writeread("#:P#",4)

	def is_ready(self):
		"Will query if telescope present. If it's, call init routines"
		try:
			a=self._writeread("#:Gc#",5)
			if a not in ('(12)#','(24)#'):
				return 0 
			self.init()	
		except TelescopeError:
			return 0
		return 1

int park(void)
{
	return moveto(0,park_dec);
}
