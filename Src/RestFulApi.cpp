/*********************************************************************/
/* Copyright (c) 2015  F.Stephan GmbH                                */
/*-------------------------------------------------------------------*/
/* $RCSfile: RestFulApi.cpp,v $                                            */
/*-------------------------------------------------------------------*/
/* Function  : FHIR RESTful API                                      */
/*-------------------------------------------------------------------*/
/* Project  : HL7 FHIR                                               */
/* $Revision: 1 $                                                    */
/* $Date: 2025-02-24 14:37:49 $                                      */
/* $Author: govinda $                                                */
/*-------------------------------------------------------------------*/

/*
***********************************************************************
* Includes
***********************************************************************
*/

#include <unistd.h>
#include <cstddef>
#include <string>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cstring>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <ctime>
#include <net/if.h>
#include <arpa/inet.h>
#include <map>
#include <functional>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#include "RestFulApi.h"
#include "json.hpp"


/*
 * ****************************************************************************
 * Namespaces
 * ****************************************************************************
 */

using json = nlohmann::json;

/*
***********************************************************************
* Defines
***********************************************************************
*/
#define ETHERNET_IF_NAME "enp0s3"
#define DEFAULT_SOCKET_TIMEOUT_MILLISECONDS 1450000
#define DEFAULT_HOST_PORT_NUMBER 8000
#define DEAFAULT_SOCKET_READ_BUFSIZE 8192

/*
***********************************************************************
* External Variables
***********************************************************************
*/

/*
***********************************************************************
* Globals/Statics
***********************************************************************
*/
static int fdFhirServerSocket = -1; // file descriptior for socket , for server
static size_t socketTimeoutMilliSeconds = DEFAULT_SOCKET_TIMEOUT_MILLISECONDS;
static int fhirPort = DEFAULT_HOST_PORT_NUMBER;
static struct sockaddr_in socketAddress; // for server 
static char ipAddressString[64];

/*
***********************************************************************
* Prototypes
***********************************************************************
*/

/*
***********************************************************************
* Implementation
***********************************************************************
*/

// ////////////////////////////////////////////////////////////////////////////
RestFulApi::RestFulApi(void)
{
  // Constructor
  // Bind member functions using std::bind and store them in restEndpoints map
  restEndpoints = {
    {"GET /endpoint1", std::bind(&RestFulApi::endpoint1Handler, this, std::placeholders::_1, std::placeholders::_2)},
    {"POST /endpoint2", std::bind(&RestFulApi::endpoint2Handler, this, std::placeholders::_1, std::placeholders::_2)},
    {"GET /endpoint3", std::bind(&RestFulApi::endpoint3Handler, this, std::placeholders::_1, std::placeholders::_2)}
  };
}

// ////////////////////////////////////////////////////////////////////////////
RestFulApi::~RestFulApi(void)
{
  // Destructor
}

// ////////////////////////////////////////////////////////////////////////////
void RestFulApi::handleErrors() {
  ERR_print_errors_fp(stderr);
  //exit(EXIT_FAILURE);  // exit app and shutdown
}

void RestFulApi::initialize_ssl() {
  // Initialize OpenSSL
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
}

SSL_CTX* RestFulApi::create_context() {
  const SSL_METHOD* method = TLS_server_method(); // Use TLS for server-side communication
  SSL_CTX* ctx = SSL_CTX_new(method);
  if (!ctx) {
      std::cerr << "Error creating SSL context" << std::endl;
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
  }
  return ctx;
}

void RestFulApi::configure_ssl_context(SSL_CTX* ctx) {
  // Load the certificate and key
  if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
      std::cerr << "Error loading certificate" << std::endl;
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
      std::cerr << "Error loading private key" << std::endl;
      ERR_print_errors_fp(stderr);
      exit(EXIT_FAILURE);
  }

  if (!SSL_CTX_check_private_key(ctx)) {
      std::cerr << "Private key does not match the certificate public key" << std::endl;
      exit(EXIT_FAILURE);
  }
}

// ////////////////////////////////////////////////////////////////////////////
SSL_CTX* RestFulApi::InitRestFulApi(void)
{
  std::cout << "111111111111111111111111111!" << "\n";

  // Initialize OpenSSL
  initialize_ssl();
  SSL_CTX* ctx = create_context();  // Create SSL context
  configure_ssl_context(ctx);  // Configure the context with certs

  fdFhirServerSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (fdFhirServerSocket < 0)
  {
    std::cout << "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE function socket() failure!" << "\n";
    //return ;
    exit(-1);
  }
  std::cout << "2222222222222222222222222222222222222!" << "\n";

  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, ETHERNET_IF_NAME, IFNAMSIZ - 1);
  ioctl(fdFhirServerSocket, SIOCGIFADDR, &ifr);

  int socketOptionsVal = 1;
  if (setsockopt(fdFhirServerSocket, SOL_SOCKET, SO_REUSEADDR, &socketOptionsVal, sizeof(int)) == -1)
  {
    perror("setsockopt");
    exit(-1);
  }

  socketAddress.sin_family = AF_INET;
  socketAddress.sin_port = htons(fhirPort);
  snprintf(ipAddressString, sizeof(ipAddressString) - 1, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
  socketAddress.sin_addr.s_addr = inet_addr(ipAddressString);

  if (bind(fdFhirServerSocket, (const struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0)
  {
    perror("Failed to bind server socket");
    //return;
    exit(-1);
  }
  std::cout << "333333333333333333333333333333333333333333333333333333333 end of InitWebServer()!";
  return ctx;
}

// ////////////////////////////////////////////////////////////////////////////
void RestFulApi::listenSocketFhir(SSL_CTX* ctx)
{
  int fdWebClient;
  struct sockaddr_in client_addr;

  if (fdFhirServerSocket < 0)
  {
    perror("fdSocket Not initialized.");
    return;
  }
  if (listen(fdFhirServerSocket, SOMAXCONN) < 0)
  {
    perror("fdSocket Listen failed...Aborting.\n");
    return;
  }

  struct timeval timeoutWebServer;
  timeoutWebServer.tv_sec = socketTimeoutMilliSeconds / 1000;
  timeoutWebServer.tv_usec = (socketTimeoutMilliSeconds % 1000) * 1000;
  setsockopt(fdFhirServerSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeoutWebServer, sizeof(timeoutWebServer));

  time_t server_start_time = time(NULL);

  std::cout << "\n" << ipAddressString << "  open for connection...[PORT:" << fhirPort << "]\n";
  
  while (true)
  {
    time_t current_time = time(NULL);
    if (difftime(current_time, server_start_time) >= socketTimeoutMilliSeconds / 1000)
    {
      std::cerr << "Server Timed out! (ET : " << socketTimeoutMilliSeconds << " ms) \n";
      break;
    }
    socklen_t client_addr_len = sizeof(client_addr);
    fdWebClient = accept(fdFhirServerSocket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (fdWebClient < 0)
    {
      std::cerr << "Couldn't establish connection with the client. \n";
    }

    // Create SSL structure for the new connection
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fdWebClient);

    // requests.\n";
    char requestBuffer[DEAFAULT_SOCKET_READ_BUFSIZE];
    int bytesRead;
    if(SSL_accept(ssl) == -1){
      std::cout << "Error accepting the SSL connection" << std::endl;
      ERR_print_errors_fp(stderr);
    }else{
      bytesRead = SSL_read(ssl,requestBuffer,sizeof(requestBuffer));
      //int bytesRead = recv(fdWebClient, requestBuffer, sizeof(requestBuffer), 0);
      if (bytesRead > 0)
      {
        std::cout << "RRRRRRRRRRRRRreadbuf = " << requestBuffer << "\n";
        ParseInMessage(ssl, fdWebClient,requestBuffer);
      }
    }
  }
  close(fdFhirServerSocket);
  SSL_CTX_free(ctx);

}

// ////////////////////////////////////////////////////////////////////////////

void RestFulApi::ParseInMessage(SSL* ssl, int fdWebClient,char buffer[]){
  std::string request(buffer); 

  // Parse the request to find the HTTP method and the endpoint (e.g., GET /simpleGET)
  size_t methodEnd = request.find(" ");  // Find the space after the method (GET, POST, etc.)
  std::string method = request.substr(0, methodEnd);  // Extract HTTP method (GET, POST, etc.)
  size_t endpointStart = methodEnd + 1;  // Start of the endpoint (after the space)
  size_t endpointEnd = request.find(" ", endpointStart);  // End of the endpoint
  std::string endpointCalled = request.substr(0,endpointEnd);

  std::cout << "Method: " << method << std::endl;
  std::cout << "Endpoint: " << endpointCalled << std::endl;

  auto it = restEndpoints.find(endpointCalled);
  if( it != restEndpoints.end()){
    it->second(ssl, request);
  }else{
    SendHttpsResponse(ssl,"Not Found","application/json","HTTP/1.1 404 Not Found");
  }
  close(fdWebClient);
}



// //////////////////////////////////////////////////////////////////////////// - NP
std::string RestFulApi::getCurDate()
{
  const time_t now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = gmtime(&now);
  char buffer[80];
  strftime(buffer, sizeof(buffer), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", timeInfo);
  std::string dateToday = buffer;
  return dateToday;
}

// ////////////////////////////////////////////////////////////////////////////
int RestFulApi::SendHttpResponse(u_int client, std::string response, std::string responseContent, std::string httpResponseCode)
{
  std::string httpResponse = "";
  httpResponse += httpResponseCode;
  httpResponse += getCurDate();
  httpResponse += "Content-Type: " + responseContent + "\n";
  httpResponse += "Content-Length:" + std::to_string(response.size()) + "\n";
  httpResponse += "Accept-Ranges: bytes\n";
  httpResponse += "Connection: close\n";
  httpResponse += "\n";
  httpResponse += response;

  const char *httpResponseCStr = httpResponse.c_str();
  if (send(client, httpResponseCStr, httpResponse.size(), 0) < 0)
  {
    std::cout<<"Response Failed"<<std::endl;
    return 1;
  }
  std::cout <<"Response Sent"<<std::endl;
  return 0;
}

int RestFulApi::SendHttpsResponse(SSL* ssl, std::string response, std::string responseContent, std::string httpResponseCode){
  std::string httpResponse = "";
  httpResponse += httpResponseCode;
  httpResponse += getCurDate();
  httpResponse += "Content-Type: " + responseContent + "\n";
  httpResponse += "Content-Length:" + std::to_string(response.size()) + "\n";
  httpResponse += "Accept-Ranges: bytes\n";
  httpResponse += "Connection: close\n";
  httpResponse += "\n";
  httpResponse += response;
  
  int httpReponse_Length = httpResponse.length();
  int write_result = SSL_write(ssl,httpResponse.c_str(),httpReponse_Length);
  if (write_result <= 0) {
    int ssl_error = SSL_get_error(ssl, write_result);
    std::cerr << "Error writing to client, SSL error code: " << ssl_error << std::endl;
    ERR_print_errors_fp(stderr);
  } else {
      std::cout << "Successfully sent response to client, bytes written: " << write_result << std::endl;
  }
  return 0;
}
// ////////////////////////////////////////////////////////////////////////////
void RestFulApi::endpoint1Handler(SSL* ssl,std::string request){
  std::string response = "hello from endpoint1";
  SendHttpsResponse(ssl,response,"application/json","HTTP/1.1 200 OK");
}
void RestFulApi::endpoint2Handler(SSL* ssl,std::string request){

  std::string response;
  size_t bodyStart = request.find("\r\n\r\n");
  if(bodyStart != std::string::npos){
    std::string jsonBody = request.substr(bodyStart + 4);
    try{
      json jsonData = json::parse(jsonBody);
      std::cout << "Parsed JSON : " << jsonData.dump(4) << std::endl;
      //std::string str1 = jsonData["key1"], str2 = jsonData["key2"]; Way of Accessing Json Data
      response = "Received JSON: " + jsonData.dump();
    }catch(const json::parse_error e){
      std::cout<< "Something went wrong while parsing JSON"<<std::endl;
    }
  }else{
    response = "hello from endpoint2";
  }
  
  SendHttpsResponse(ssl,response,"application/json","HTTP/1.1 200 OK");

}
void RestFulApi::endpoint3Handler(SSL* ssl,std::string request){
  std::string response = "hello from endpoint3";
  SendHttpsResponse(ssl,response,"application/json","HTTP/1.1 200 OK");
}




