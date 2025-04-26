/*********************************************************************/
/* Copyright (c) 2015  F.Stephan GmbH                                */
/*-------------------------------------------------------------------*/
/* $RCSfile: Fhir.cpp,v $                                            */
/*-------------------------------------------------------------------*/
/* Function  : FHIR  protocl interface                               */
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


#include <cstddef>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <map>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "RestFulApi.h"
#include "Fhir.h"


static RestFulApi* fhirServer = new RestFulApi();

// ////////////////////////////////////////////////////////////////////////////
int main(void)
{
  SSL_CTX* ctx = fhirServer->InitRestFulApi();
  fhirServer->listenSocketFhir(ctx);
}
