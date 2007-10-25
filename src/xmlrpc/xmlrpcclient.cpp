// HelloClient.cpp : A simple xmlrpc client. Usage: HelloClient serverHost serverPort
// Link against xmlrpc lib and whatever socket libs your system needs (ws2_32.lib
// on windows)
#include "xmlrpc++/XmlRpc.h"
#include <iostream>
using namespace XmlRpc;

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: HelloClient serverHost serverPort\n";
		return -1;
	}
	int port = atoi(argv[2]);
	//XmlRpc::setVerbosity(5);

	// Use introspection API to look up the supported methods
	XmlRpcClient c(argv[1], port);
	XmlRpcValue noArgs, result;
	if (c.execute("system.listMethods", noArgs, result))
		std::cout << "\nMethods:\n " << result << "\n\n";
	else
		std::cout << "Error calling 'listMethods'\n\n";

	// Use introspection API to get the help string for the Hello method
	XmlRpcValue oneArg;
	oneArg[0] = "DeviceCount";
	if (c.execute("system.methodHelp", oneArg, result))
		std::cout << "Help for 'DeviceCount' method: " << result << "\n\n";
	else
		std::cout << "Error calling 'methodHelp'\n\n";

	// Call the DeviceCount method
	if (c.execute("DeviceCount", noArgs, result))
		std::cout << result << "\n\n";
	else
		std::cout << "Error calling 'DeviceCount'\n\n";

	// Call system.listDevices method
	if (c.execute("system.listDevices", noArgs, result))
	{
		std::cout << "Devices: " << std::endl;
		for (int i = 0; i < result.size(); i++)
		{
			std::cout << " " << result[i] << std::endl;
		}
	}
	else
	{
		std::cout << "Error calling 'system.listDevices'\n\n";
	}

	// Call system.listVariables method
	if (c.execute("system.listValues", noArgs, result))
	{
		std::cout << "Devices + values: " << std::endl;
		for (int i = 0; i < result.size(); i++)
		{
			std::cout << " " << result[i] << std::endl;
		}
	}
	else
	{
		std::cout << "Error calling 'system.listValues'\n\n";
	}

	oneArg[0] = "C0";

	// Call system.listVariables method with one device
	if (c.execute("system.listValuesDevice", oneArg, result))
	{
		std::cout << "Devices + values for C0: " << std::endl;
		for (int i = 0; i < result.size(); i++)
		{
			std::cout << " " << result[i] << std::endl;
		}
	}
	else
	{
		std::cout << "Error calling 'system.listValuesDevice'\n\n";
	}

	oneArg[0] = "C0";
	oneArg[1] = "C1";

	// Call system.listVariables method with one device
	if (c.execute("system.listValuesDevice", oneArg, result))
	{
		std::cout << "Devices + values for C0 and C1: " << std::endl;
		for (int i = 0; i < result.size(); i++)
		{
			std::cout << " " << result[i] << std::endl;
		}
	}
	else
	{
		std::cout << "Error calling 'system.listValuesDevice'\n\n";
	}

	XmlRpcValue notDevice;
	notDevice[0] = "CXXX";

	// Call system.listVariables method with one device
	if (c.execute("system.listValuesDevice", notDevice, result))
	{
		std::cout << "Devices + values for CXX: " << std::endl;
		for (int i = 0; i < result.size(); i++)
		{
			std::cout << " " << result[i] << std::endl;
		}
	}
	else
	{
		std::cout << "Error calling 'system.listValuesDevice'\n\n";
	}

	return 0;
}
