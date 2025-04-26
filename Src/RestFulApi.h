/*********************************************************************/
/* Copyright (c) 2015  F.Stephan GmbH                                */
/*-------------------------------------------------------------------*/
/* $RCSfile: WebServer.h,v $                                         */
/*-------------------------------------------------------------------*/
/* Function  : FHIR RESTful API                                      */
/*-------------------------------------------------------------------*/
/* Project  : HL7 FHIR                                               */
/* $Revision: 1 $                                                    */
/* $Date: 2025-02-24 14:37:49 $                                      */
/* $Author: govinda $                                                */
/*-------------------------------------------------------------------*/

#ifndef SOURCE_BUSINESS_FHIR_RESTFULAPI_H_
#define SOURCE_BUSINESS_FHIR_RESTFULAPI_H_
#define DEFAULT_SOCKET_READ_BUFSIZE 8192

#ifdef __cplusplus

#include <map>
#include <functional>
#include <openssl/ssl.h>
#include <openssl/rand.h>

/*
***********************************************************************
* Includes
***********************************************************************
*/

/*
***********************************************************************
* Defines
***********************************************************************
*/

/*
 * ****************************************************************************
 * Typedefs
 * ****************************************************************************
 */

/*
***********************************************************************
* Prototypes
***********************************************************************
*/
class RestFulApi
{
   // ////////////////////////////////////////////////////////////////////////////
   // Public
   // ////////////////////////////////////////////////////////////////////////////
public:
   /*_--------------------------------------------------------------------------*/
   /*
    * \brief the class that handles the RestFulApi interface
    * @param none
    */
   RestFulApi(void);

   /*_--------------------------------------------------------------------------*/
   /** virtual destructor
    */
   ~RestFulApi(void);

   /*_--------------------------------------------------------------------------*/
   /** RestFulApi init functions
    */
   SSL_CTX* InitRestFulApi();

   
   /*_--------------------------------------------------------------------------*/
   /** listen for socket messages
    */
   void listenSocketFhir(SSL_CTX* ctx);

   /*_--------------------------------------------------------------------------*/
   /** RestEnpoints
    */
   void endpoint1Handler(SSL* ssl, std::string request);
   void endpoint2Handler(SSL* ssl, std::string request);
   void endpoint3Handler(SSL* ssl, std::string request);


   // ////////////////////////////////////////////////////////////////////////////
   // Public inline implementation
   // ////////////////////////////////////////////////////////////////////////////

   // ////////////////////////////////////////////////////////////////////////////
   // Protected
   // ////////////////////////////////////////////////////////////////////////////
protected:
   // ////////////////////////////////////////////////////////////////////////////
   // Protected inline implementation
   // ////////////////////////////////////////////////////////////////////////////

   // ////////////////////////////////////////////////////////////////////////////
   // Private
   // ////////////////////////////////////////////////////////////////////////////
private:

   char requestBuffer[DEFAULT_SOCKET_READ_BUFSIZE];
   std::map<std::string, std::function<void(SSL*, std::string)>> restEndpoints;

   /*_--------------------------------------------------------------------------*/
   /** Parse incoming message
    */
   void ParseInMessage(SSL* ssl, int fdWebClient,char buffer[]);

   /*_--------------------------------------------------------------------------*/
   /** getCurDate
    */
   std::string getCurDate();

   /*_--------------------------------------------------------------------------*/
   /** SendHttpResponse
    */
   int SendHttpResponse(u_int client, std::string response, std::string responseContent, std::string httpResponseCode);
   int SendHttpsResponse(SSL* ssl, std::string response, std::string responseContent, std::string httpResponseCode);
   /*_--------------------------------------------------------------------------*/
   /** SSL
    */
   void handleErrors();
   void initialize_ssl();
   SSL_CTX* create_context();
   void configure_ssl_context(SSL_CTX* ctx);

};

#endif /* __cplusplus */
#endif /* SOURCE_BUSINESS_FHIR_RESTFULAPI_H_ */
