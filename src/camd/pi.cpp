/* 
 * Dummy camera for testing purposes.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "camd.h"

#include <sstream>

#include <PvSystem.h>
#include <PvInterface.h>

namespace rts2camd
{

/**
 * Class for a Pleora/Pi camera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class PI:public Camera
{
	public:
		PI (int in_argc, char **in_argv);
		virtual ~PI (void);

		virtual int processOption (int in_opt);
		virtual int initHardware ();
		
		virtual int initChips ();
		virtual int info ();
		virtual int startExposure ();

		virtual int doReadout ();

	protected:
		virtual void initBinnings ();

		virtual void initDataTypes ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

	private:
};

};

using namespace rts2camd;

PI::PI (int argc, char **argv):Camera (argc, argv)
{
	addOption ('l', NULL, 0, "list available cameras");
}

PI::~PI ()
{
}

int PI::processOption (int opt)
{
	if (opt == 'l')
	{
		PvResult lResult;
		const PvDeviceInfo* lLastDeviceInfo = NULL;

		// Find all devices on the network.
		PvSystem lSystem;
		lResult = lSystem.Find();
		if ( !lResult.IsOK() )
		{
			std::cerr << "PvSystem::Find Error: " << lResult.GetCodeString().GetAscii() << std::endl;
			return -1;
		}

		// Go through all interfaces 
		uint32_t lInterfaceCount = lSystem.GetInterfaceCount();
		for ( uint32_t x = 0; x < lInterfaceCount; x++ )
		{
			std::cerr << "Interface " << x << std::endl;

			// Get pointer to the interface.
			const PvInterface* lInterface = lSystem.GetInterface( x );

			// Is it a PvNetworkAdapter?
			const PvNetworkAdapter* lNIC = dynamic_cast<const PvNetworkAdapter*>( lInterface );
			if ( lNIC != NULL )
			{
				std::cerr << "  MAC Address: " << lNIC->GetMACAddress().GetAscii() << std::endl;
				std::cerr << "  IP Address: " << lNIC->GetIPAddress().GetAscii() << std::endl;
				std::cerr << "  Subnet Mask: " << lNIC->GetSubnetMask().GetAscii() << std::endl;
			}

			// Is it a PvUSBHostController?
			const PvUSBHostController* lUSB = dynamic_cast<const PvUSBHostController*>( lInterface );
			if ( lUSB != NULL )
			{
				std::cerr << "  Name: " << lUSB->GetName().GetAscii() << std::endl;
			}

			// Go through all the devices attached to the interface
			uint32_t lDeviceCount = lInterface->GetDeviceCount();
			for ( uint32_t y = 0; y < lDeviceCount ; y++ )
			{
				const PvDeviceInfo *lDeviceInfo = lInterface->GetDeviceInfo( y );

				std::cerr << "  Device " << y << std::endl;
				std::cerr << "	Display ID: " << lDeviceInfo->GetDisplayID().GetAscii() << std::endl;
				
				const PvDeviceInfoGEV* lDeviceInfoGEV = dynamic_cast<const PvDeviceInfoGEV*>( lDeviceInfo );
				const PvDeviceInfoU3V *lDeviceInfoU3V = dynamic_cast<const PvDeviceInfoU3V *>( lDeviceInfo );
				const PvDeviceInfoUSB *lDeviceInfoUSB = dynamic_cast<const PvDeviceInfoUSB *>( lDeviceInfo );
				const PvDeviceInfoPleoraProtocol* lDeviceInfoPleora = dynamic_cast<const PvDeviceInfoPleoraProtocol*>( lDeviceInfo );

				if ( lDeviceInfoGEV != NULL ) // Is it a GigE Vision device?
				{
					std::cerr << "	MAC Address: " << lDeviceInfoGEV->GetMACAddress().GetAscii() << std::endl;
					std::cerr << "	IP Address: " << lDeviceInfoGEV->GetIPAddress().GetAscii() << std::endl;
					std::cerr << "	Serial number: " << lDeviceInfoGEV->GetSerialNumber().GetAscii() << std::endl;
					lLastDeviceInfo = lDeviceInfo;
				}
				else if ( lDeviceInfoU3V != NULL ) // Is it a USB3 Vision device?
				{
					std::cerr << "	GUID: " << lDeviceInfoU3V->GetDeviceGUID().GetAscii() << std::endl;
					std::cerr << "	S/N: " << lDeviceInfoU3V->GetSerialNumber().GetAscii() << std::endl;
					std::cerr << "	Speed: " << lUSB->GetSpeed() << std::endl;
					lLastDeviceInfo = lDeviceInfo;
				}
				else if ( lDeviceInfoUSB != NULL ) // Is it an unidentified USB device?
				{
					std::cerr << std::endl;
				}
				else if ( lDeviceInfoPleora != NULL ) // Is it a Pleora Protocol device?
				{
					std::cerr << "	MAC Address: " << lDeviceInfoPleora->GetMACAddress().GetAscii() << std::endl;
					std::cerr << "	IP Address: " << lDeviceInfoPleora->GetIPAddress().GetAscii() << std::endl;
					std::cerr << "	Serial number: " << lDeviceInfoPleora->GetSerialNumber().GetAscii() << std::endl;
				}
			}
		}
		return 0;
	}
	return Camera::processOption (opt);
}

int PI::initHardware ()
{
	return 0;
}

int PI::initChips ()
{
	return 0;
}

int PI::info ()
{
	return Camera::info();
}

int PI::startExposure ()
{
	return 0;
}

int PI::doReadout ()
{
	return 0;
}

void PI::initBinnings ()
{
}

void PI::initDataTypes ()
{
}

int PI::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	return Camera::setValue (old_value, new_value);
}

int main (int argc, char **argv)
{
	PI device (argc, argv);
	return device.run ();
}
