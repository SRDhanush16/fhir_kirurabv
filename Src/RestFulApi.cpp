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

#include "RestFulApi.h"

/*
 * ****************************************************************************
 * Namespaces
 * ****************************************************************************
 */

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
static int fdFhirServerSocket = -1; // file descriptior for socket 
static size_t socketTimeoutMilliSeconds = DEFAULT_SOCKET_TIMEOUT_MILLISECONDS;
static int fhirPort = DEFAULT_HOST_PORT_NUMBER;
static struct sockaddr_in socketAddress;
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
    {"/endpoint1", std::bind(&RestFulApi::endpoint1Handler, this, std::placeholders::_1, std::placeholders::_2)},
    {"/endpoint2", std::bind(&RestFulApi::endpoint2Handler, this, std::placeholders::_1, std::placeholders::_2)},
    {"/endpoint3", std::bind(&RestFulApi::endpoint3Handler, this, std::placeholders::_1, std::placeholders::_2)}
  };
}

// ////////////////////////////////////////////////////////////////////////////
RestFulApi::~RestFulApi(void)
{
 //   if (0 != atexit(Deinit))
 //   {
 //     CPRINTF_(INIT, TEXTCOLOR_RED, "Error! 0 != atexit(Deinit)");
 //   }

  // Destructor

 }

// ////////////////////////////////////////////////////////////////////////////
void RestFulApi::InitRestFulApi(void)
{
  std::cout << "111111111111111111111111111!" << "\n";

  fdFhirServerSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (fdFhirServerSocket < 0)
  {
    std::cout << "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE function socket() failure!" << "\n";
    return;
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

  socketAddress.sin_port = htons(fhirPort);
  snprintf(ipAddressString, sizeof(ipAddressString) - 1, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
  socketAddress.sin_addr.s_addr = inet_addr(ipAddressString);

  socketAddress.sin_family = AF_INET;

  if (bind(fdFhirServerSocket, (const struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0)
  {
    perror("Failed to bind server socket");
    return;
  }
  std::cout << "333333333333333333333333333333333333333333333333333333333 end of InitWebServer()!";

}

// ////////////////////////////////////////////////////////////////////////////
void RestFulApi::listenSocketFhir(void)
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
    // requests.\n";
    char requestBuffer[DEAFAULT_SOCKET_READ_BUFSIZE];
    int bytesRead = recv(fdWebClient, requestBuffer, sizeof(requestBuffer), 0);
    if (bytesRead > 0)
    {
      std::cout << "RRRRRRRRRRRRRreadbuf = " << requestBuffer << "\n";
      //SendHttpResponse(fdWebClient, "response", "response_format", "HTTP/1.1 200 OK\n");
      ParseInMessage(fdWebClient, requestBuffer);
    }
  }

}

// ////////////////////////////////////////////////////////////////////////////

void RestFulApi::ParseInMessage(int fdWebClient, char buffer[]){
  std::string request(buffer); 

  // Parse the request to find the HTTP method and the endpoint (e.g., GET /simpleGET)
  size_t methodEnd = request.find(" ");  // Find the space after the method (GET, POST, etc.)
  std::string method = request.substr(0, methodEnd);  // Extract HTTP method (GET, POST, etc.)

  size_t endpointStart = methodEnd + 1;  // Start of the endpoint (after the space)
  size_t endpointEnd = request.find(" ", endpointStart);  // End of the endpoint
  std::string endpointCalled = request.substr(endpointStart, endpointEnd - endpointStart);  // Extract endpoint

  // Log the method and endpoint for debugging
  std::cout << "Method: " << method << std::endl;
  std::cout << "Endpoint: " << endpointCalled << std::endl;
  bool isFound = false;
  for(const auto& endpoint : restEndpoints){
    if(endpoint.first == endpointCalled){
      isFound = true;
      endpoint.second(fdWebClient,request);
      break;
    }
  }
  if(!isFound){
    SendHttpResponse(fdWebClient,"Not Found","application/json","HTTP/1.1 404 Not Found");
  }
  close(fdWebClient);

}

void RestFulApi::ParseInMessage(void)
{

}

// ////////////////////////////////////////////////////////////////////////////
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

// ////////////////////////////////////////////////////////////////////////////
void RestFulApi::endpoint1Handler(int webClinet_fd,std::string request){
  std::string response = "hello from endpoint1";
  SendHttpResponse(webClinet_fd,response,"application/json","HTTP/1.1 200 OK");
}
void RestFulApi::endpoint2Handler(int webClinet_fd,std::string request){
  std::string response = "hello from endpoint2";
  SendHttpResponse(webClinet_fd,response,"application/json","HTTP/1.1 200 OK");
}
void RestFulApi::endpoint3Handler(int webClinet_fd,std::string request){
  std::string response = "hello from endpoint3";
  SendHttpResponse(webClinet_fd,response,"application/json","HTTP/1.1 200 OK");
}


// code ends

