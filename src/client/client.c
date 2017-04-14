#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// protobuf-c headers
#include <protobuf-c/protobuf-c.h>
#include <protobuf-c-rpc/protobuf-c-rpc.h>
// Definitions for our protocol (under build/)
#include <protobuf/protocol.pb-c.h>


struct router_closure_data_s {
	char* server_addr;
	protobuf_c_boolean is_done;
};

typedef struct router_closure_data_s router_closure_data_t;


static void handle_router_response (const Dkvs__RouterResponse *result, void *closure_data)
{
  router_closure_data_t* cd = (router_closure_data_t*) closure_data;

  if (result == NULL)
    printf ("Error processing request.\n");
  else if (result->address == "")
    printf ("Not found.\n");
  else
    {
      if(result->address == NULL)
	fprintf (stderr,"Memory full. Cannot store more data.\n");
      else{
      	cd->server_addr = malloc(strlen(result->address));
  	strcpy(cd->server_addr, result->address);
      }
    }

  cd->is_done = 1;
}


static void handle_server_get_response (const Dkvs__Row *result, void *closure_data)
{
  if (result == NULL)
    printf ("Error processing request.\n");
  else
    {
      if((char*)result->value.data != NULL){
        fprintf (stderr,"key: %s\n", (char*)result->key.data);
        fprintf (stderr,"value: %s\n", (char*)result->value.data);
      }
    }

  * (protobuf_c_boolean *) closure_data = 1;
}


static void handle_server_set_response (const Dkvs__Status *result, void *closure_data)
{
  if (result == NULL)
    printf ("Error processing request.\n");
  else
    {
      if((int)result->status == 0)
        fprintf (stderr, "Key:Value stored successfully.\n");
    }

  * (protobuf_c_boolean *) closure_data = 1;
}


/* Run the main-loop without blocking.  It would be nice
   if there was a simple API for this (protobuf_c_rpc_dispatch_run_with_timeout?),
   but there isn't for now. */
static void
do_nothing (ProtobufCRPCDispatch *dispatch, void *unused)
{
}


static void
run_main_loop_without_blocking (ProtobufCRPCDispatch *dispatch)
{
  protobuf_c_rpc_dispatch_add_idle (dispatch, do_nothing, NULL);
  protobuf_c_rpc_dispatch_run (dispatch);
}


int main(void) {

  // router
  ProtobufCService *router_service;
  ProtobufC_RPC_Client *router_client;
  ProtobufC_RPC_AddressType address_type = PROTOBUF_C_RPC_ADDRESS_TCP;
  const char *router_name = "localhost:4747";
  router_closure_data_t* r_closure_data = malloc(sizeof(router_closure_data_t));
  
  signal (SIGPIPE, SIG_IGN);

  // creating router client
  router_service = protobuf_c_rpc_client_new (address_type, router_name, &dkvs__router__descriptor, NULL);
  if (router_service == NULL)
    printf("error creating service");
  router_client = (ProtobufC_RPC_Client *) router_service;

  // connecting to router
  fprintf (stderr, "Connecting to router... ");
  while (!protobuf_c_rpc_client_is_connected (router_client))
    protobuf_c_rpc_dispatch_run (protobuf_c_rpc_dispatch_default ());
  fprintf (stderr, "done.\n");

  char key[20];
  char value[20];
  int* option;
  char* proceed;

  do{
 
    fprintf (stderr, "1. Get\n2. Set\n");
    fscanf(stdin, "%d", option);
    fprintf(stderr, "Key: ");
    fscanf(stdin, "%s", key);
    if((*option) == 2){
      fprintf(stderr, "Value: ");
      fscanf(stdin, "%s", value);
    }
	    
    // request router for server address
    ProtobufCBinaryData s_req;
    s_req.data = malloc(strlen(key)+1);
    s_req.len  = strlen(key)+1;
    s_req.data = key;  
      
    Dkvs__RouterRequest query = DKVS__ROUTER_REQUEST__INIT;
    query.key = s_req;
    r_closure_data->is_done = 0;

    dkvs__router__get_server (router_service, &query, handle_router_response, r_closure_data);

    // loop until operation completes
    while (!r_closure_data->is_done)
      protobuf_c_rpc_dispatch_run (protobuf_c_rpc_dispatch_default ());
  
    // server address received from router
    fprintf (stderr,"server_address : %s\n", r_closure_data->server_addr);

    if(r_closure_data->server_addr == NULL)
      break;

    // server
    ProtobufCService *server_service;
    ProtobufC_RPC_Client *server_client;
    const char *server_name = r_closure_data->server_addr;
    protobuf_c_boolean is_done = 0;

    // creating server client
    server_service = protobuf_c_rpc_client_new (address_type, server_name, &dkvs__server__descriptor, NULL);
    if (server_service == NULL)
      printf("error creating service");
    server_client = (ProtobufC_RPC_Client *) server_service;

    // connecting to server
    fprintf (stderr, "Connecting to server... ");
    while (!protobuf_c_rpc_client_is_connected (server_client))
      protobuf_c_rpc_dispatch_run (protobuf_c_rpc_dispatch_default ());
    fprintf (stderr, "done.\n");

    // prepare server request
    ProtobufCBinaryData req_k;
    req_k.data = malloc(strlen(key)+1);
    req_k.len  = strlen(key)+1;
    req_k.data = key;

    if((*option) == 1){
      
      Dkvs__RouterRequest s_query = DKVS__ROUTER_REQUEST__INIT;
      s_query.key = req_k;

      dkvs__server__get (server_service, &s_query, handle_server_get_response, &is_done);

    }
    else{
      ProtobufCBinaryData req_v;
      req_v.data = malloc(strlen(value)+1);
      req_v.len  = strlen(value)+1;
      req_v.data = value;

      Dkvs__Row s_query = DKVS__ROW__INIT;
      s_query.key = req_k;
      s_query.value = req_v;
      
      dkvs__server__set (server_service, &s_query, handle_server_set_response, &is_done);
    }

    while (!is_done)
      protobuf_c_rpc_dispatch_run (protobuf_c_rpc_dispatch_default ());

    //close(server_service);

    fprintf(stderr, "Continue? (y/n): ");
    fscanf(stdin, "%s", proceed);

  }while(strcmp(proceed, "y") == 0);

  return 0;
}
