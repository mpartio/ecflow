//============================================================================
// Name        : HttpMain
// Author      : partio
// Revision    : $Revision$
//
// Copyright 2009-2020 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
// Description :
//============================================================================

#include "HttpServer.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
   /// By default, error condition will throw exception.
   try {
	  HttpServer server(argc, argv);
	  server.run();
   } catch (std::exception& e) {
	  std::cerr << e.what() << std::endl;
	  return 1;
   }
   return 0;
}
