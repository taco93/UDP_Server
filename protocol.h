
#ifdef __GCC_IEC_559 
#pragma message("GCC ICE 559 defined...")

#else

#error *** do not use this platform

#endif


#include <stdint.h>

/* 
   Used in both directions; if 
   server->client,type should be set to 1, 
   client->server type = 2. 
 */
struct calcProtocol{
  uint16_t type;  // What message is this, 1 = server to client, 2 client to server, 3... reserved 
  uint16_t major_version; // 1
  uint16_t minor_version; // 0
  uint32_t id; // Server side identification with operation. Client must return the same ID as it got from Server.
  uint32_t arith; // What operation to perform, see mapping below. 
  int32_t inValue1; // integer value 1
  int32_t inValue2; // integer value 2
  int32_t inResult; // integer result
  double flValue1;  // float value 1
  double flValue2;  // float value 2
  double flResult;  // float result
};

  
struct calcMessage {
  uint16_t type;    // See below
  uint32_t message; // See below
  
  // Protocol, UDP = 17, TCP = 6, other values are reserved. 
  uint16_t protocol;
  uint16_t major_version; // 1
  uint16_t minor_version; // 0 

};


/* arith mapping in calcProtocol
1 - add
2 - sub
3 - mul
4 - div
5 - fadd
6 - fsub
7 - fmul
8 - fdiv

other numbers are reserved

*/


/* 
   calcMessage.type
   1 - server-to-client, text protocol
   2 - server-to-client, binary protocol
   3 - server-to-client, N/A
   21 - client-to-server, text protocol
   22 - client-to-server, binary protocol
   23 - client-to-serve, N/A
   
   calcMessage.message 

   0 = Not applicable/availible (N/A or NA)
   1 = OK   // Accept 
   2 = NOT OK  // Reject 

*/
