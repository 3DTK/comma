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

#include <set>
#include <vector>
#include <boost/lexical_cast.hpp>
#include "../../application/command_line_options.h"
#include "../../string/string.h"

static void usage( bool verbose )
{
    std::cerr << "take csv string, quote anything that is not a number" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat lines.csv | csv-quote [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --delimiter,-d=<delimiter>; default: ," << std::endl;
    std::cerr << "    --do-not-quote-empty-fields,--do-not-quote-empty; do not quote empty fields, unless they are quoted in the input, i.e: a,,b -> \"a\",,\"b\"" << std::endl;
    std::cerr << "    --fields=<fields>: quote given fields, if their values are not numbers, additionally use --force to override" << std::endl;
    std::cerr << "                       if --unquote, unquote only given fields" << std::endl;
    std::cerr << "    --force=<fields>: quote given fields, if their values are numbers" << std::endl;
    std::cerr << "    --escaped: escape quotes with backslash" << std::endl;
    std::cerr << "    --mangle-delimiter=<what>: mangle delimiter inside of the quotes (to unmangle later e.g. with sed)" << std::endl;
    std::cerr << "    --quote=<quote sign>; default: double quote" << std::endl;
    std::cerr << "    --unquote; remove quotes" << std::endl;
    std::cerr << std::endl;
    std::cerr << "example" << std::endl;
    std::cerr << "    echo -e \"x=hi there\\ny=34\" | csv-quote --delimiter = --fields ,x" << std::endl;
    std::cerr << "  output:" << std::endl;
    std::cerr << "    x=\"hi there\"" << std::endl;
    std::cerr << "    y=34" << std::endl;
    std::cerr << std::endl;
    exit( 1 );
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        char delimiter = options.value( "--delimiter,-d", ',' );
        if( options.exists( "--mangle-delimiter" ) )
        {
            std::string mangled = options.value< std::string >( "--mangle-delimiter" );
            while( std::cin.good() )
            {
                std::string line;
                std::getline( std::cin, line );
                if( line.empty() ) { continue; }
                bool quoted = false;
                bool escaped = false;
                for( char c: line ) // quick and dirty; performance should be OK, since std::cout gets flushed only on std::endl
                {
                    if( quoted && c == delimiter ) { std::cout << mangled; continue; }
                    switch( c )
                    {
                        case '\\': escaped = !escaped; break;
                        case '"': if( !escaped ) { quoted = !quoted; escaped = false; } break;
                        default: if( escaped ) { escaped = false; } break;
                    }
                    std::cout << c;
                }
                std::cout << std::endl;
            }
            return 0;
        }
        std::set< std::size_t > fields;
        {
            const std::vector< std::string >& v = comma::split( options.value< std::string >( "--fields", "" ), ',' );
            for( unsigned int i = 0; i < v.size(); ++i ) { if( !v[i].empty() ) { fields.insert( i ); } }
        }
        std::set< std::size_t > forced;
        {
            const std::vector< std::string >& v = comma::split( options.value< std::string >( "--force", "" ), ',' );
            for( unsigned int i = 0; i < v.size(); ++i ) { if( !v[i].empty() ) { forced.insert( i ); } }
        }
        bool do_not_quote_empty_fields = options.exists( "--do-not-quote-empty-fields,--do-not-quote-empty" );
        char quote = options.value( "--quote", '\"' );
        bool unquote = options.exists( "--unquote" );
        std::string backslash;
        std::vector< std::string > format;
        if (options.exists("--format") ) { format = comma::split(options.value<std::string>("--format"), ','); }
        if( options.exists( "--escape" ) ) { backslash = "\\"; }
        while( std::cin.good() )
        {
            std::string line;
            std::getline( std::cin, line );
            if( line.empty() ) { continue; }
            const std::vector< std::string >& v = comma::split( line, delimiter );
            if (!format.empty() && format.size() != v.size()) { COMMA_THROW(comma::exception, "--format \"" << options.value<std::string>("--format") << "\" has " << format.size() << " fields but input line \"" << line << "\" has " << v.size() << " fields"); }
            std::string comma;
            for( std::size_t i = 0; i < v.size(); ++i )
            {
                bool has_field = fields.empty() || fields.find( i ) != fields.end();
                std::cout << comma;
                comma = delimiter;
                if( unquote )
                {
                    std::cout << ( has_field ? comma::strip( v[i], quote ) : v[i] );
                }
                else
                {
                    bool do_quote = false;
                    const std::string& value = comma::strip( v[i], quote );
                    if( has_field )
                    {
                        if( format.empty() )
                        {
                            if( !( do_not_quote_empty_fields && v[i].empty() ) )
                            {
                                do_quote = true;
                                if( forced.find( i ) == forced.end() ) { try { boost::lexical_cast< double >( value ); do_quote = false; } catch( ... ) {} }
                            }
                        }
                        else
                        {
                            do_quote = ( format[i][0] == 's' ); // quick and dirty
                        }
                    }
                    if( do_quote ) { std::cout << backslash << quote << value << backslash << quote; } else { std::cout << value; }
                }
            }
            std::cout << std::endl;
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-quote: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-quote: unknown exception" << std::endl; }
    return 1;
}
