
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "../hashtable/hashtable.c"

// protobuf-c headers
#include <protobuf-c/protobuf-c.h>
#include <protobuf-c-rpc/protobuf-c-rpc.h>
// Definitions for our protocol (under build/)
#include <protobuf/protocol.pb-c.h>


static hashtable_t* hashtable;


static void example__set (Dkvs__Server_Service    *service,
                  	      const Dkvs__Row  *row,
                              Dkvs__Status_Closure  closure,
                  	      void                       *closure_data)
{
  (void) service;
  if (row == NULL)
    closure (NULL, closure_data);
  else
    {
      ht_set((const char*)row->key.data, (const char*)row->value.data);
      fprintf(stderr, "Key value saved successfully\n");
      Dkvs__Status result = DKVS__STATUS__INIT;
      result.status = 0;
      closure (&result, closure_data);
    }
}


static void example__get (Dkvs__Server_Service    *service,
                  	  const Dkvs__RouterRequest  *router_request,
                          Dkvs__Row_Closure  closure,
                  	  void                       *closure_data)
{
  (void) service;
  if (router_request == NULL)
    closure (NULL, closure_data);
  else
    {
      ProtobufCBinaryData val;
      char* value = ht_get(router_request->key.data);
      
      if(value == NULL){
	fprintf(stderr, "No match found\n");
	val.data = malloc(1);
        val.len  = 1;
        val.data = NULL;
      }
      else{
	fprintf(stderr, "Found\n");
	val.data = malloc(strlen(value)+1);
        val.len  = strlen(value)+1;
        val.data = value;
      }

      Dkvs__Row result = DKVS__ROW__INIT;
      result.key = router_request->key;
      result.value = val;
      closure (&result, closure_data);
    }
}


static Dkvs__Server_Service the_server_service = DKVS__SERVER__INIT(example__);


int main(int argc, char** argv) {

  // initializing hashtable
  ht_create(500);

  // initializing server
  ProtobufC_RPC_Server *server;
  ProtobufC_RPC_AddressType address_type = PROTOBUF_C_RPC_ADDRESS_TCP;
  const char *name = argv[1];
  
  server = protobuf_c_rpc_server_new (address_type, name, (ProtobufCService *) &the_server_service, NULL);

  for (;;)
    protobuf_c_rpc_dispatch_run (protobuf_c_rpc_dispatch_default ());
  
  return 0;
}


