// This file is part of comma, a generic and flexible library
// Copyright (c) 2011 The University of Sydney
// Copyright (c) 2019 Vsevolod Vlaskine
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

/// @author vsevolod vlaskine

#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <boost/optional.hpp>
#include "../../application/command_line_options.h"
#include "../../string.h"
#include "../../xpath/xpath.h"

static void usage( bool )
{
    std::cerr << std::endl;
    std::cerr << "take path-value data on stdin, output as csv to stdout" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat data.path-value | name-value-convert [<options>] > data.csv" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --delimiter,-d=<delimiter>; default=','; csv delimiter" << std::endl;
    std::cerr << "    --equal-sign,-e=<equal_sign>; default='='; equal sign" << std::endl;
    std::cerr << "    --fields,-f=<fields>; fields to output" << std::endl;
    std::cerr << "    --prefix,--path,-p=[<prefix>]; optional prefix" << std::endl;
    std::cerr << "    --unindexed-fields=<fields>; if no --fields specified, output unindexed fields once, if --fields specified, append given unindexed fields to all records" << std::endl;
    std::cerr << "    --unsorted; the input data is not sorted by index" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples" << std::endl;
    std::cerr << "    indexed data" << std::endl;
    std::cerr << "        cat <<EOF | $scriptname --fields=name,value,status --prefix=my/test" << std::endl;
    std::cerr << "        my/test[0]/name=a" << std::endl;
    std::cerr << "        my/test[0]/value=10" << std::endl;
    std::cerr << "        my/test[0]/status=0" << std::endl;
    std::cerr << "        my/test[1]/name=b" << std::endl;
    std::cerr << "        my/test[1]/value=20" << std::endl;
    std::cerr << "        my/test[1]/status=1" << std::endl;
    std::cerr << "        EOF" << std::endl;
    std::cerr << std::endl;
    std::cerr << "        yields:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "        a,10,0" << std::endl;
    std::cerr << "        b,20,1" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    unindexed fields" << std::endl;
    std::cerr << "        todo" << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

typedef std::unordered_map< std::string, std::string > values_t;

static std::string join( const std::vector< std::string >& fields, values_t& values, char delimiter )
{
    std::ostringstream oss;
    std::string comma;
    for( const auto& f: fields ) { oss << comma << values[f]; comma = delimiter; } // quick and dirty as everything else
    values.clear();
    return oss.str();
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        std::string fs = options.value< std::string >( "--fields,-f", "" );
        std::vector< std::string > fields = comma::split( fs, ',' );
        std::vector< std::string > unindexed_fields;
        std::string ufs = options.value< std::string >( "--unindexed-fields", "" );
        std::unordered_set< std::string > unindexed_fields_set;
        if( !ufs.empty() )
        { 
            unindexed_fields = comma::split( ufs, ',' );
            for( const auto& f: unindexed_fields ) { unindexed_fields_set.insert( f ); }
        }
        if( fields[0].empty() && unindexed_fields.empty() ) { std::cerr << "name-value-to-csv: please specify --fields or --unindexed-fields" << std::endl; return 1; }
        bool unindexed = fields[0].empty();
        values_t unindexed_values;
        options.assert_mutually_exclusive( "--unsorted", "--unindexed,--no-index" );
        options.assert_mutually_exclusive( "--unindexed,--no-index", "--unindexed-fields" );
        bool unsorted = options.exists( "--unsorted" );
        char delimiter = options.value( "--delimiter,-d", ',' );
        char equal_sign = options.value( "--equal-sign,-e", '=' );
        std::string prefix = options.value< std::string >( "--prefix,--path,-p", "" );
        values_t values; // quick and dirty; watch performance?
        std::map< unsigned int, values_t > map;
        boost::optional< unsigned int > index;
        while( std::cin.good() && !std::cin.eof() )
        {
            std::string s;
            std::getline( std::cin, s );
            if( comma::strip( s, " \t" ).empty() || comma::strip( s, " \t" )[0] == '#' ) { continue; }
            auto e = s.find_first_of( equal_sign ); // todo: use boost::spirit
            if( e == std::string::npos ) { std::cerr << "name-value-to-csv: expected path-value pair; got: '" << s << "'" << std::endl; return 1; }
            std::string name = s.substr( 0, e );
            if( unindexed_fields_set.find( name ) != unindexed_fields_set.end() ) { unindexed_values[name] = s.substr( e + 1 ); continue; }
            if( name.substr( 0, prefix.size() ) != prefix ) { continue; }
            if( unindexed )
            {
                if( prefix.empty() ) { values[name] = s.substr( e + 1 ); }
                else if( name[ prefix.size() ] == '/' ) { values[ name.substr( prefix.size() + 1 ) ] = s.substr( e + 1 ); }
                continue;
            }
            if( name[prefix.size()] != '[' ) { continue; }
            auto b = s.find_first_of( ']', prefix.size() );
            if( b == std::string::npos ) { std::cerr << "name-value-to-csv: expected path-value pair with valid indices; got: '" << s << "'" << std::endl; return 1; }
            if( s[ b + 1 ] != '/' ) { continue; }
            unsigned int current_index = boost::lexical_cast< unsigned int >( name.substr( prefix.size() + 1, b - prefix.size() - 1 ) );
            if( unsorted || !unindexed_fields.empty() ) { map[current_index][name.substr( b + 2 )] = s.substr( e + 1 ); continue; }
            if( index && current_index < *index ) { std::cerr << "name-value-to-csv: expected sorted index, got index " << current_index << " after " << *index << " in line: '" << comma::strip( s ) << "'" << std::endl; return 1; }
            if( index && current_index > *index ) { std::cout << join( fields, values, delimiter ) << std::endl; }
            values[name.substr( b + 2 )] = s.substr( e + 1 );
            index = current_index;
        }
        if( unindexed )
        { 
            std::cout << join( unindexed_fields, unindexed_values, delimiter ) << std::endl;
        }
        else if( unsorted || !unindexed_fields.empty() )
        { 
            std::string u;
            if( !unindexed_fields.empty() ) { u = join( unindexed_fields, unindexed_values, delimiter ); }
            for( auto v: map )
            {
                std::cout << join( fields, v.second, delimiter );
                if( !unindexed_fields.empty() ) { std::cout << delimiter << u; }
                std::cout << std::endl;
            }
        }
        else if( index )
        {
            std::cout << join( fields, values, delimiter ) << std::endl;
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "name-value-to-csv: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "name-value-to-csv: unknown exception" << std::endl; }
    return 1;
}
