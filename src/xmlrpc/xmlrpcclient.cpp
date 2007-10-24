// HelloClient.cpp : A simple xmlrpc client. Usage: HelloClient serverHost serverPort
// Link against xmlrpc lib and whatever socket libs your system needs (ws2_32.lib 
// on windows)
#include "xmlrpc++/XmlRpc.h"
#include <iostream>
using namespace XmlRpc;

int
main (int argc, char *argv[])
{
  if (argc != 3)
    {
      std::cerr << "Usage: HelloClient serverHost serverPort\n";
      return -1;
    }
  int port = atoi (argv[2]);
  //XmlRpc::setVerbosity(5);

  // Use introspection API to look up the supported methods
  XmlRpcClient c (argv[1], port);
  XmlRpcValue noArgs, result;
  if (c.execute ("system.listMethods", noArgs, result))
    std::cout << "\nMethods:\n " << result << "\n\n";
  else
    std::cout << "Error calling 'listMethods'\n\n";

  // Use introspection API to get the help string for the Hello method
  XmlRpcValue oneArg;
  oneArg[0] = "DeviceCount";
  if (c.execute ("system.methodHelp", oneArg, result))
    std::cout << "Help for 'DeviceCount' method: " << result << "\n\n";
  else
    std::cout << "Error calling 'methodHelp'\n\n";

  // Call the Hello method
  if (c.execute ("DeviceCount", noArgs, result))
    std::cout << result << "\n\n";
  else
    std::cout << "Error calling 'DeviceCount'\n\n";

  return 0;
}
