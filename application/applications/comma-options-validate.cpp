// This file is part of comma, a generic and flexible library
// Copyright (c) 2011 The University of Sydney
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Sydney nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include "../command_line_options.h"

void usage( bool )
{
    std::cerr << std::endl;
    std::cerr << "validate command line options against option description" << std::endl;
    std::cerr << std::endl;
    std::cerr << "returns 0, if options are valid, 1 otherwise" << std::endl;
    std::cerr << std::endl;
    std::cerr << "a convenience for option parsing, e.g. in bash scripts" << std::endl;
    std::cerr << std::endl;
    std::cerr << "cat description.txt | comma-options-to-name-value <options>" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::command_line_options::description::usage() << std::endl;
    std::cerr << "lines starting with spaces, tabs, or comment signs '#' are ignored" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples (try them):" << std::endl;
    std::cerr << "    valid options:" << std::endl;
    std::cerr << "        echo '--verbose,-v' | comma-options-validate -v hello world" << std::endl;
    std::cerr << "    invalid options:" << std::endl;
    std::cerr << "        echo '--verbose,-v' | comma-options-validate -v hello world --blah" << std::endl;
    std::cerr << std::endl;
    exit( 1 );
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        std::vector< comma::command_line_options::description > descriptions;
        while( std::cin.good() )
        {
            std::string line;
            std::getline( std::cin, line );
            line = comma::strip( line, '\r' ); // windows... sigh...
            if( line.empty() || line[0] == ' '  || line[0] == '\t'  || line[0] == '#' ) { continue; } // quick and dirty
            descriptions.push_back( comma::command_line_options::description::from_string( line ) );
        }
        options.assert_valid( descriptions, true );
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "comma-options-validate: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "comma-options-validate: unknown exception" << std::endl; }
    return 1;
}
