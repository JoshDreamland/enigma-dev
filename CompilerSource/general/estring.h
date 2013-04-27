/**
  @file  estring.h
  @brief Declares some very basic string convenience functions. The kind you'll find
         standard in most languages nowadays.
  
  @section License
    Copyright (C) 2008-2013 Josh Ventura
    This file is a part of the ENIGMA Development Environment.

    ENIGMA is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, version 3 of the license or any later version.

    This application and its source code is distributed AS-IS, WITHOUT ANY WARRANTY; 
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE. See the GNU General Public License for more details.

    You should have recieved a copy of the GNU General Public License along
    with this code. If not, see <http://www.gnu.org/licenses/>
**/

#ifndef _ESTRING__H
#define _ESTRING__H

#include <string>
using std::string;

string tostring(int val);
string tostringd(double val);
string tostringv(void* val);
int string_count(char c, char* str);
string arraybounds_as_str(string str);
string string_replace_all(string str,string substr,string nstr);
string toUpper(string x);

#endif
