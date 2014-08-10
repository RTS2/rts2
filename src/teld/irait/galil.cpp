#include "galil.h"
#include <sstream>

#define AR_ABS =114212084
#define DEC_ABS =14286199

using namespace rts2teld;

Galil::Galil (rts2core::Block *_master, const char *_hostname, int _port):ConnTCP (_master, _hostname, _port)
{
}

double Galil::getXMotVelArcsS() 
{
	double dArcsecs;	// arcsec/s on sky

	dArcsecs = countsToArcsecs( (long)getXMotVel() );

	return dArcsecs;
}

double Galil::getYMotVelArcsS() 
{
	double dArcsecs;	// arcsec/s on sky

	dArcsecs = countsToArcsecs( (long)getYMotVel() );

	return dArcsecs;
}

void Galil::sendLong (const char *cmd, long value, std::string &Rx)
{
        char Tx[20];

	snprintf (Tx, 20, "%s%li", cmd, value);
	clientTxRx (Tx, Rx);   
}

void Galil::sendCmd (const char *cmd, std::string &Rx)
{
        char Tx[20];
	snprintf (Tx, 20, "%s", cmd);
	clientTxRx (Tx, Rx);
}

void Galil::send2Long (const char *cmd, long value1, long value2, std::string &Rx)
{
        char Tx[20];

	snprintf (Tx, 20, "%s%li,%li", cmd, value1, value2);
	clientTxRx (Tx, Rx); 
}

void Galil::setXMotVel (long lVel) 
{
        std::string Rx;

        sendLong ("SP", lVel, Rx);
}

void Galil::setYMotVel (long lVel) 
{
        std::string Rx;

        sendLong ("SP,", lVel, Rx);	
}

void Galil::setXYMotVel( long lVelX, long lVelY ) 
{
        std::string Rx;

	send2Long ("SP", lVelX, lVelY, Rx);
}

void Galil::setXMotVelArcsS( double dVel ) 
{
	
	long lVel;	//	motor encoder p.p.s.

	lVel = arcsecsToCounts(dVel);

	setXMotVel(lVel);

        
}

void Galil::setYMotVelArcsS( double dVel ) 
{
	long lVel;	//	motor encoder p.p.s.

	lVel = arcsecsToCounts(dVel);

	setYMotVel(lVel);
}

void Galil::setXYMotVelArcsS( double dVelX, double dVelY ) 
{
	long lVelX, lVelY;	//	motor encoder p.p.s.

	lVelX = arcsecsToCounts(dVelX);
	lVelY = arcsecsToCounts(dVelY);

        setXYMotVel(lVelX,lVelY);
}

void Galil::startXMove() 
{
        std::string Rx;
	sendCmd ("BGA",Rx);
	
}

void Galil::startYMove() 
{
        std::string Rx;
        sendCmd ("BGB", Rx);
}

void Galil::startXYMove() 
{
	std::string Rx;
        sendCmd ("BGAB", Rx);
	
}

void Galil::stopXMove() 
{
        std::string Rx;
	sendCmd ("STA", Rx);
	
}

void Galil::stopYMove() 
{
	std::string Rx;
	sendCmd ("STB", Rx);
}

void Galil::stopXYMove() 
{
        std::string Rx;
	sendCmd ("STAB", Rx);
}

void Galil::moveXToAbs( long lCounts )


{
	// spostamento alla posizione assoluta lCounts
	std::string Rx;

	//	correzione offset X
	/*
	lCounts += m_lXEncOffset;
	*/
	sendLong("PR%i",lCounts, Rx);
}

void Galil::moveYToAbs( long lCounts )
{
        std::string Rx;
	//	correzione offset Y
	/*
	lCounts += m_lYEncOffset;
	*/

	sendLong ("PR,%i",lCounts,Rx);
}



void Galil::moveXRel( long lCounts )
{
	// spostamento relativo (lCounts da posizione corrente)
        std::string Rx;
	sendLong("PR%i",lCounts,Rx);
}

void Galil::moveYRel( long lCounts )
{
	std::string Rx;
	sendLong ("PR,%i",lCounts,Rx);
}

void Galil::moveXRelArcs( double dArcsecs )
{
	long lPos;
	lPos = arcsecsToCounts(dArcsecs);
	moveXRel(lPos);
}

void Galil::moveYRelArcs( double dArcsecs )
{
	long lPos;
	lPos = arcsecsToCounts(dArcsecs);
	moveYRel(lPos);
}

void Galil::setAbsXCurrPos( long lCounts )
{
	std::string Rx;
	sendLong ("DP%i",lCounts,Rx);
}

void Galil::setAbsYCurrPos( long lCounts )
{
        std::string Rx;
        sendLong ("DP,%i",lCounts,Rx);
}

void Galil::setAbsXCurrPosArcs( double dArcsecs )
{
	long lPos;
	lPos = arcsecsToCounts(dArcsecs);
	setAbsXCurrPos(lPos);
}

void Galil::setAbsYCurrPosArcs( double dArcsecs )
{
	long lPos;
	lPos = arcsecsToCounts(dArcsecs);
	setAbsYCurrPos(lPos);
}

double Galil::getXMotAcc() 
{
        std::string strRX;
	sendCmd ("AC?", strRX);
        return atof(strRX.c_str ());

}

double Galil::getYMotAcc() 
{
        std::string strRX;
	clientTxRx ("AC,?", strRX);
	return atof(strRX.c_str ());

}

void Galil::setXMotAcc( long lValue ) 
{
        std::string strRX;
	sendLong ("AC%i",lValue,strRX);
}

void Galil::setYMotAcc( long lValue ) 
{
        std::string strRX;
	sendLong("AC,%i",lValue,strRX);
}

void Galil::setXMotDec(long lValue) 
{
        std::string strRX;
	sendLong("DC%i",lValue,strRX);
}

void Galil::setYMotDec(long lValue) 
{
        std::string strRX;
	sendLong ("DC,%i",lValue,strRX);
}


void Galil::getMotorStatus( long &mota , long &motb) 
{
      	std::string strRX;

	sendCmd("MG _BGA", strRX);

	mota = atol(strRX.c_str ());


	sendCmd("MG _BGB" , strRX);

	
	motb = atol(strRX.c_str ());

}

double Galil::getYMotVel() 
{
	std::string strRX;

	sendCmd( "SP,?", strRX );
	return atof(strRX.c_str ());
}

double Galil::getXMotVel() 
{
        std::string strRX;
	
	sendCmd( "SP?", strRX );
	
        return atof(strRX.c_str ());

}


long Galil::arcsecsToCounts( double dArcsecs )
{
	long lCounts;
	int Rid=1440;
	int RidMot=3;
	int AsseRisMotEnc=12800;
	lCounts = (long)( dArcsecs * Rid * RidMot * (double)AsseRisMotEnc / 1296000. ); 

	return lCounts;
}

double Galil::countsToArcsecs( long lCounts )
{
	double dArcsecs;	// arcsec/s on sky
	int Rid=1440;
	int RidMot=3;
	int AsseRisMotEnc=12800;

	dArcsecs = ( (double)lCounts * 1296000. )/( Rid * RidMot * (double)AsseRisMotEnc );
	
	return dArcsecs;
}

void Galil::moveXToAbsArcs( double dArcsecs )
{
	
	long lPos;
	lPos = arcsecsToCounts(dArcsecs);
	moveXToAbs(lPos);
        
}

void Galil::moveYToAbsArcs( double dArcsecs )
{

	long lPos;
	lPos = arcsecsToCounts(dArcsecs);
	moveYToAbs(lPos);
}



double Galil::getXMotEncPosArcs() 
{
	double dArcsecs;
	dArcsecs = countsToArcsecs( getXMotEncPos() );
	return dArcsecs;
}

double Galil::getYMotEncPosArcs() 
{
	double dArcsecs;
	dArcsecs = countsToArcsecs( getYMotEncPos() );
	return dArcsecs;
}


long Galil::getYMotEncPos() 
{
	std::string strRX;
	sendCmd( "RPB", strRX);
	return  atol(strRX.c_str ());
	
}

long Galil::getXMotEncPos ()
{
	std::string strRX;
	clientTxRx("RPA", strRX);
	return atol (strRX.c_str ());
}

void Galil::clientTx (const char *tx)
{
	sendData (tx);
	
	sendData ("\r\n");
	
}

void Galil::clientRx (std::string &rx)
{
        std::stringstream _is;
	receiveData (&_is, 5, '\n');
	rx = _is.str ();
}

void Galil::clientTxRx (const char *tx, std::string &rx)
{
	clientTx (tx);
	clientRx (rx);
}


/*#####################################################################################################################
void Galil::setXMaxMinPos(double maval,double mival) 
{
	m_dXMaxPos = maval;
	m_dXMinPos = mival;
}

void Galil::setYMaxMinPos(double maval,double mival) 
{
	m_dYMaxPos = maval;
	m_dYMinPos = mival;
}

void Galil::setXMaxMinVel(double maval,double mival) 
{
	m_dXMaxVel = maval;
	m_dXMinVel = mival;
}

void Galil::setYMaxMinVel(double maval,double mival) 
{
	m_dYMaxVel = maval;
	m_dYMinVel = mival;
}

void Galil::setXUserUnit(double val)
{
	//UM=ARCSECS;
	//CONVFACTOR[ax]=val*ENCODERRES[ax]/MAXMIS[ARCSECS];

	m_dXENCODERRES = 160000.0;
	m_dXMAXMIS = 1296000.0;

	m_dXCONVFACTOR = val*m_dXENCODERRES/m_dXMAXMIS;

	AsseX.CONVFACTOR[X] = m_dXCONVFACTOR;
}

void Galil::SetYUserUnit(double val)
{
	//UM=ARCSECS;
	//CONVFACTOR[ax]=val*ENCODERRES[ax]/MAXMIS[ARCSECS];

	m_dYENCODERRES = 160000.0;
	m_dYMAXMIS = 1296000.0;

	m_dYCONVFACTOR = val*m_dYENCODERRES/m_dYMAXMIS;

	AsseY.CONVFACTOR[X] = m_dYCONVFACTOR;
}
**********************************************************************************************************************************************************/
