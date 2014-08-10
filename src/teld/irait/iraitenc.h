#include "connection/tcp.h"
#include <sstream>

namespace rts2teld
{

class IraitEnc:public rts2core::ConnTCP
{
	public:
		IraitEnc (rts2core::Block *_master, const char *_hostname, int _port);

		// public interface to IRAIT encoders
		long getXAxisEncPos ();
		long getYAxisEncPos ();
		long getXAxisEncPosArcsec ();
		long getYAxisEncPosArcsec ();
		
	private:
		void clientTxRx (const char *tx, std::string &rx);
		void clientRx (std::string &rx);
		void clientTx (const char *tx);
};

};
