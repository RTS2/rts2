#include "connection/tcp.h"
#include <sstream>

namespace rts2teld
{

class Galil:public rts2core::ConnTCP
{
	public:
		Galil (rts2core::Block *_master, const char *_hostname, int _port);
		// public interface to 
		double getXMotVelArcsS();
		double getYMotVelArcsS() ;
		void setXMotVel( long lVel ); 
		void setYMotVel( long lVel );
		void setXYMotVel( long lVelX, long lVelY ); 
		void setXMotVelArcsS( double dVel );
		void setYMotVelArcsS( double dVel );
		void setXYMotVelArcsS( double dVelX, double dVelY );
		void startXMove();
		void startYMove();
		void startXYMove();
		void stopXMove();
		void stopYMove();
		void stopXYMove();
		void moveXToAbs( long lCounts );
		void moveYToAbs( long lCounts );
		void moveXRel( long lCounts );
		void moveYRel( long lCounts );
		void moveXRelArcs( double dArcsecs );
		void moveYRelArcs( double dArcsecs );
		void setAbsXCurrPos( long lCounts );
		void setAbsYCurrPos( long lCounts );
		void setAbsXCurrPosArcs( double dArcsecs );
		void setAbsYCurrPosArcs( double dArcsecs );
		double getXMotAcc();
		double getYMotAcc();
		void setXMotAcc( long lValue );
		void setYMotAcc( long lValue );
		void setXMotDec( long lValue );
		void setYMotDec( long lValue );
		void getMotorStatus(long &mota , long &motb);
		double getYMotVel();
		double getXMotVel();
		long arcsecsToCounts( double dArcsecs );
		double countsToArcsecs( long lCounts );
		void moveXToAbsArcs( double dArcsecs );
	        void moveYToAbsArcs( double dArcsecs );
		double getXMotEncPosArcs();
		double getYMotEncPosArcs();
        	long getYMotEncPos();
		long getXMotEncPos ();     

	private:
		void clientTxRx (const char *tx, std::string &rx);
	        void clientTx (const char *tx);
                void clientRx (std::string &rx);
		
		void sendCmd (const char *cmd, std::string &Rx);
		void send2Long(const char *cmd, long value1, long value2, std::string &Rx);
		void sendLong (const char *cmd, long value, std::string &Rx);
};

};
