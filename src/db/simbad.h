/** @mainpage simbad.h Definitions

@section SesameSoapBinding Service Binding "SesameSoapBinding"

@subsection SesameSoapBinding_operations Operations

  - @ref ns1__sesame
  - @ref ns1__sesame_
  - @ref ns1__sesame__
  - @ref ns1__SesameXML
  - @ref ns1__Sesame

@subsection SesameSoapBinding_ports Endpoint Ports

  - http://cdsws.u-strasbg.fr/axis/services/Sesame
@section SesameSoapBinding Service Binding "SesameSoapBinding"

@subsection SesameSoapBinding_operations Operations

  - @ref ns1__getAvailability

@subsection SesameSoapBinding_ports Endpoint Ports

  - http://cdsws.u-strasbg.fr/axis/services/Sesame

*/

//  Note: modify this file to customize the generated data type declarations

//gsoapopt w
#include <string>

//gsoap ns1  service name:      SesameSoapBinding 
//gsoap ns1  service type:      Sesame 
//gsoap ns1  service port:      http://cdsws.u-strasbg.fr/axis/services/Sesame 
//gsoap ns1  service namespace: urn:Sesame 

/// Operation response struct "ns1__sesameResponse" of service binding "SesameSoapBinding" operation "ns1__sesame"
struct ns1__sesameResponse
{
  std::string _return_;
};

/// Operation "ns1__sesame" of service binding "SesameSoapBinding"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__sesame(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    std::string                         name,
    std::string                         resultType,
    struct ns1__sesameResponse&
  );
@endcode

C++ proxy class (defined in soapSesameSoapBindingProxy.h):
  class SesameSoapBinding;

*/

//gsoap ns1  service method-style:      sesame rpc
//gsoap ns1  service method-encoding:   sesame http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:     sesame ""
int ns1__sesame (std::string _name,
		 std::string _resultType, struct ns1__sesameResponse &);

/// Operation response struct "ns1__sesameResponse_" of service binding "SesameSoapBinding" operation "ns1__sesame_"
struct ns1__sesameResponse_
{
  std::string _return_;
};

/// Operation "ns1__sesame_" of service binding "SesameSoapBinding"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__sesame_(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    std::string                         name,
    std::string                         resultType,
    struct ns1__sesameResponse_&
  );
@endcode

C++ proxy class (defined in soapSesameSoapBindingProxy.h):
  class SesameSoapBinding;

*/

//gsoap ns1  service method-style:      sesame_ rpc
//gsoap ns1  service method-encoding:   sesame_ http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:     sesame_ ""
int ns1__sesame_ (std::string _name,
		  std::string _resultType, struct ns1__sesameResponse_ &);

/// Operation response struct "ns1__sesameResponse__" of service binding "SesameSoapBinding" operation "ns1__sesame__"
struct ns1__sesameResponse__
{
  std::string _return_;
};

/// Operation "ns1__sesame__" of service binding "SesameSoapBinding"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__sesame__(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    std::string                         name,
    std::string                         resultType,
    struct ns1__sesameResponse__&
  );
@endcode

C++ proxy class (defined in soapSesameSoapBindingProxy.h):
  class SesameSoapBinding;

*/

//gsoap ns1  service method-style:      sesame__ rpc
//gsoap ns1  service method-encoding:   sesame__ http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:     sesame__ ""
int ns1__sesame__ (std::string _name,
		   std::string _resultType, struct ns1__sesameResponse__ &);

/// Operation response struct "ns1__SesameXMLResponse" of service binding "SesameSoapBinding" operation "ns1__SesameXML"
struct ns1__SesameXMLResponse
{
  std::string _return_;
};

/// Operation "ns1__SesameXML" of service binding "SesameSoapBinding"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__SesameXML(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    std::string                         name,
    struct ns1__SesameXMLResponse&
  );
@endcode

C++ proxy class (defined in soapSesameSoapBindingProxy.h):
  class SesameSoapBinding;

*/

//gsoap ns1  service method-style:      SesameXML rpc
//gsoap ns1  service method-encoding:   SesameXML http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:     SesameXML ""
int ns1__SesameXML (std::string _name, struct ns1__SesameXMLResponse &);

/// Operation response struct "ns1__SesameResponse" of service binding "SesameSoapBinding" operation "ns1__Sesame"
struct ns1__SesameResponse
{
  std::string _return_;
};

/// Operation "ns1__Sesame" of service binding "SesameSoapBinding"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__Sesame(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    std::string                         name,
    struct ns1__SesameResponse&
  );
@endcode

C++ proxy class (defined in soapSesameSoapBindingProxy.h):
  class SesameSoapBinding;

*/

//gsoap ns1  service method-style:      Sesame rpc
//gsoap ns1  service method-encoding:   Sesame http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:     Sesame ""
int ns1__Sesame (std::string _name, struct ns1__SesameResponse &);

//gsoap ns1  service name:      SesameSoapBinding 
//gsoap ns1  service type:      Sesame 
//gsoap ns1  service port:      http://cdsws.u-strasbg.fr/axis/services/Sesame 
//gsoap ns1  service namespace: http://DefaultNamespace 

/// Operation response struct "ns1__getAvailabilityResponse" of service binding "SesameSoapBinding" operation "ns1__getAvailability"
struct ns1__getAvailabilityResponse
{
  std::string getAvailabilityReturn;
};

/// Operation "ns1__getAvailability" of service binding "SesameSoapBinding"

/**

Operation details:

  - SOAP RPC encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"

C stub function (defined in soapClient.c[pp]):
@code
  int soap_call_ns1__getAvailability(struct soap *soap,
    NULL, // char *endpoint = NULL selects default endpoint for this operation
    NULL, // char *action = NULL selects default action for this operation
    struct ns1__getAvailabilityResponse&
  );
@endcode

C++ proxy class (defined in soapSesameSoapBindingProxy.h):
  class SesameSoapBinding;

*/

//gsoap ns1  service method-style:      getAvailability rpc
//gsoap ns1  service method-encoding:   getAvailability http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns1  service method-action:     getAvailability ""
int ns1__getAvailability (struct ns1__getAvailabilityResponse &);

typedef float xsd__float;

/*  End of simbad.h Definitions */
