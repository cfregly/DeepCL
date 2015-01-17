// Copyright Hugh Perkins 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

class IHasToString {
public:
    virtual std::string toString() = 0;
};

std::string toString( IHasToString *val ); // { // not terribly efficient, but works...
//   std::ostringstream myostringstream;
//   myostringstream << val->toString();
//   return myostringstream.str();
//}

template<typename T>
std::string toString(T val ) { // not terribly efficient, but works...
   std::ostringstream myostringstream;
   myostringstream << val;
   return myostringstream.str();
}

std::vector<std::string> split(const std::string &str, const std::string &separator = " " );
std::string trim( const std::string &target );

inline double atof( std::string stringvalue ) {
   return std::atof(stringvalue.c_str());
}
inline double atoi( std::string stringvalue ) {
   return std::atoi(stringvalue.c_str());
}

// returns empty string if off the end of the number of available tokens
inline std::string getToken( std::string targetstring, int tokenIndexFromZero, std::string separator = " " ) {
   std::vector<std::string> splitstring = split( targetstring, separator );
   if( tokenIndexFromZero < splitstring.size() ) {
      return splitstring[tokenIndexFromZero];
   } else {
      return "";
   }
}

inline std::string toLower(std::string in ) {
   int len = in.size();
   char buffer[len + 1];
   for( int i = 0; i < len; i++ ) {
      char thischar = in[i];
      thischar = tolower(thischar);
      buffer[i] = thischar;
   }
   buffer[len] = 0;
   return std::string(buffer);
}
