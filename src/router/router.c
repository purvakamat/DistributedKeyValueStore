
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

static int server_count = 0;
static char** server_ports;
static int* server_size;
static char* localhost = "localhost";


static char* get_server_with_memory(){
  int min_size = 99999;
  int j = 0;
  
  for(int i=0; i<server_count; i++) {
    if(server_size[i] < min_size){
	min_size = server_size[i];
 	j = i;
    }
  }

  char* server = NULL;

  if(min_size != 99999){
    server = server_ports[j];
    server_size[j]++;
  }
  
  return server;
}


static void example__get_server (Dkvs__Router_Service    *service,
                  	      const Dkvs__RouterRequest  *router_request,
                              Dkvs__RouterResponse_Closure  closure,
                  	      void                       *closure_data)
{
  (void) service;
  if (router_request == NULL)
    closure (NULL, closure_data);
  else
    {
      fprintf (stderr,"which server has this key? %s\n",(char*)router_request->key.data);
      char* addr = ht_get((char*)router_request->key.data);
      
      if(addr == NULL){
        char* new_port =  get_server_with_memory();
	if(new_port != NULL){
	  addr = malloc(strlen(localhost) + strlen(new_port) + 1);
	  strcpy(addr, localhost);
	  strcat(addr, ":");
	  strcat(addr, new_port);
	  strcat(addr,"\0");
	
	  ht_set((char*)router_request->key.data, addr);
	  fprintf (stderr,"new addr: %s\n",addr);
	}
      }
	
      fprintf (stderr,"addr: %s\n",addr);
      
      Dkvs__RouterResponse result = DKVS__ROUTER_RESPONSE__INIT;
      result.address = addr;
      closure (&result, closure_data);
    }
}


static Dkvs__Router_Service the_router_service = DKVS__ROUTER__INIT(example__);


int main(int args, char** argv) {

  // initialize
  ht_create(500);
  const char *name = "4747";
  server_ports = malloc((args - 1) * sizeof(char*));

  int j=0;
  for(int i=1; i<args; i++){
    server_ports[j] = malloc(strlen(argv[i] + 1));
    strcpy(server_ports[j], argv[i]);
    j++;
  }
  
  server_count = j;
  server_size = malloc(server_count * sizeof(int));

  for(int i=0; i<server_count; i++){
    server_size[i] = 0;
  }
  
  // initialize server
  ProtobufC_RPC_Server *server;
  ProtobufC_RPC_AddressType address_type = PROTOBUF_C_RPC_ADDRESS_TCP;
  
  server = protobuf_c_rpc_server_new (address_type, name, (ProtobufCService *) &the_router_service, NULL);

  for (;;)
    protobuf_c_rpc_dispatch_run (protobuf_c_rpc_dispatch_default ());
  
  return 0;
}
