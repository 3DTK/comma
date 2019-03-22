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


/// @author vsevolod vlaskine

#include <iostream>
#include <string>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../name_value/parser.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

namespace {

void bash_completion( unsigned const ac, char const * const * av )
{
    static char const * const arguments[] = {
        " --binary -b --delimiter -d --fields -f --flush --format --full-xpath --help -h --quote --precision --verbose -v",
        " --input-fields --output-fields --output-format"
    };
    std::cout << arguments << std::endl;
    exit( 0 );
}


static void usage( bool )
{
    std::cerr << std::endl;
    std::cerr << "generate active inactive state with respect to duration." << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat a.csv | csv-time-pulse [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "<options>" << std::endl;
    std::cerr << comma::csv::options::usage( "t,duration,active" ) << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples" << std::endl;
    std::cerr << "    > 20181111T000000,2,1 | csv-time-pulse" << std::endl;
    std::cerr << "    20181111T000000,2,1" << std::endl;
    std::cerr << "    20181111T000000,2,0" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    > 20181111T000000,-10,0 | csv-time-pulse" << std::endl;
    std::cerr << "    20181111T235950,-10,1" << std::endl;
    std::cerr << "    20181111T000000,-10,0" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( -1 );
}

struct point_t
{
    boost::posix_time::ptime timestamp;
    double duration;
    bool   active;
    point_t() {}
    point_t( const boost::posix_time::ptime& timestamp ) : timestamp( timestamp ) {}
};

void handle_info_options( comma::command_line_options const& options )
{
    if( options.exists( "--input-fields" ) ) { std::cout << comma::join( comma::csv::names< point_t >( false ), ',' ) << std::endl; exit( 0 ); }
}

boost::posix_time::time_duration get_delay( double duration, int sign )
{
    auto minutes = int( std::floor( std::abs( duration ) / 60 ) );
    auto seconds = int( std::floor( std::abs( duration ) - double( minutes ) * 60 ) );
    auto microseconds = int( ( std::abs( duration ) - ( double( minutes ) * 60 + seconds ) ) * 1000000 );
    minutes *= sign;
    seconds *= sign;
    microseconds *= sign;
    return boost::posix_time::minutes( minutes ) + boost::posix_time::seconds( seconds ) + boost::posix_time::microseconds( microseconds );
}

std::string get_input_line( comma::csv::input_stream< point_t > const& istrm, comma::csv::options const& csv )
{
    if( csv.binary() ) { return std::string( istrm.binary().last(), csv.format().size() ); }
    else { return comma::join( istrm.ascii().last(), csv.delimiter ); }
}

}


namespace comma { namespace visiting {

template <> struct traits< point_t >
{
    template < typename K, typename V > static void visit( const K&, const point_t& p, V& v )
    {
        v.apply( "t", p.timestamp );
        v.apply( "duration", p.duration );
        v.apply( "active", p.active );
    }
    template < typename K, typename V > static void visit( const K&, point_t& p, V& v )
    {
        v.apply( "t", p.timestamp );
        v.apply( "duration", p.duration );
        v.apply( "active", p.active );
    }
};
    
} } // namespace comma { namespace visiting {

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        if( options.exists( "--bash-completion" ) ) bash_completion( ac, av );
        handle_info_options( options );

        comma::csv::options csv( options );
        comma::csv::input_stream< point_t > istrm( std::cin, csv );
        comma::csv::output_stream< point_t > ostrm( std::cout, csv );

        while( std::cin.good() && !std::cin.eof() )
        {
            auto record = istrm.read(); if( !record ) break;
            auto sign = record->duration < 0 ? -1 : 1;
            auto delay = get_delay( record->duration, sign );
            auto line = get_input_line( istrm, csv );

            point_t delayed_record = *record;
            delayed_record.timestamp = record->timestamp + delay;
            delayed_record.active = !record->active;

            if( sign > 0 )
            {
                ostrm.write( *record, line );
                ostrm.write( delayed_record, line );
            }
            else
            {
                ostrm.write( delayed_record, line );
                ostrm.write( *record, line );
            }
        }
        return 0;     
    }
    catch( std::exception& ex ) { std::cerr << "csv-time-delay: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-time-delay: unknown exception" << std::endl; }
    return 1;
}
