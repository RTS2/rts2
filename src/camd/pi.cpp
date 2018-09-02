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
#include <PvDevice.h>

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

		int listCameras ();

		PvDevice* connect (const PvString &aConnectionID);
		int listParameters (const PvString &aConnectionID);
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
		return listCameras();
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

int PI::listCameras ()
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
				lLastDeviceInfo = lDeviceInfo;
			}

			listParameters(lLastDeviceInfo->GetConnectionID());
		}
	}
	return 0;
}

PvDevice* PI::connect (const PvString &aConnectionID)
{
	PvResult res;
	PvDevice *dev = PvDevice::CreateAndConnect(aConnectionID, &res);
	if (!res.IsOK())
	{
		PvDevice::Free(dev);
		return NULL;
	}
	return dev;
}

int PI::listParameters (const PvString &aConnectionID)
{
	PvDevice *pv = connect(aConnectionID);
	if (pv == NULL)
	{
		std::cerr << "Unable to connect to " << aConnectionID.GetAscii() << std::endl;
		return -1;
	}

	PvGenParameterArray *params = pv->GetParameters();
	for (uint32_t x = 0; x < params->GetCount(); x++)
	{
		PvGenParameter *p = params->Get(x);
		PvString name, category;
		p->GetCategory(category);
		p->GetName(name);
		std::cerr << category.GetAscii() << ":" << name.GetAscii() << ", ";

		if (!p->IsAvailable())
		{
			std::cerr << "{Not Available}" << std::endl;
			continue;
		}

		if (!p->IsReadable())
		{
			std::cerr << "{Not readable}" << std::endl;
			continue;
		}

		PvGenType type;
		p->GetType(type);
		switch (type)
		{
			case PvGenTypeInteger:
			{
				int64_t val;
				static_cast<PvGenInteger *>(p)->GetValue(val);
				std::cerr << "Integer: " << val;
				break;
			}

			case PvGenTypeEnum:
			{
				PvString val;
				static_cast<PvGenEnum *>(p)->GetValue(val);
				std::cerr << "Enum: " << val.GetAscii();
				break;
			}

			case PvGenTypeBoolean:
			{
				bool val;
				static_cast<PvGenBoolean *>(p)->GetValue(val);
				std::cerr << "Boolean: " << (val ? "TRUE" : "FALSE");	
				break;
			}

			case PvGenTypeString:
			{
				PvString val;
				static_cast<PvGenString *>(p)->GetValue(val);
				std::cerr << "String: " << val.GetAscii();
				break;
			}

			case PvGenTypeCommand:
				std::cerr << "Command";
				break;

			case PvGenTypeFloat:
			{
				double val;
				static_cast<PvGenFloat *>(p)->GetValue(val);
				std::cerr << "Float: " << val;
			}

			default:
				std::cerr << "Unknow type";
		}
		std::cerr << std::endl;
	}

	PvDevice::Free(pv);
	return 0;
}

int main (int argc, char **argv)
{
	PI device (argc, argv);
	return device.run ();
}
