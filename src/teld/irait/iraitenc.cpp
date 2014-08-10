#include "iraitenc.h"
#include "teld.h"

using namespace rts2teld;

IraitEnc::IraitEnc (rts2core::Block *_master, const char *_hostname, int _port):rts2core::ConnTCP (_master, _hostname, _port)
{

}

long IraitEnc::getXAxisEncPos()
{
	std::string strRX;
	clientTxRx( "getposx", strRX);
	return atol(strRX.c_str ());	
}
long IraitEnc::getYAxisEncPos()
{
	std::string strRX;
	clientTxRx( "getposy", strRX);
	return atol(strRX.c_str ());	
}

long IraitEnc::getXAxisEncPosArcsec()
{
	return 0; // countsToArcsecs(getXAxisEncPos());	
}

long IraitEnc::getYAxisEncPosArcsec()
{
	return 0; // countsToArcsecs(getYAxisEncPos());	
}


void IraitEnc::clientTx (const char *tx)
{
	sendData (tx);
	
	sendData ("\r\n");
}

void IraitEnc::clientRx (std::string &rx)
{
	std::stringstream _is;
	receiveData (&_is, 5, '\n');
	rx = _is.str ();
}

void IraitEnc::clientTxRx (const char *tx, std::string &rx)
{
	clientTx (tx);
	clientRx (rx);
}
