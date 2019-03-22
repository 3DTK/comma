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

#ifdef WIN32
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#endif

#include <iostream>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../base/exception.h"
#include "../../base/types.h"
#include "../../io/file_descriptor.h"
#include "../../math/compare.h"
#include "../../csv/options.h"
#include "../../csv/stream.h"
#include "../../visiting/traits.h"

using namespace comma;

static void usage(bool detail=false)
{
    std::cerr << std::endl;
    std::cerr << "Read input data and thin them down by the given percentage;" << std::endl;
    std::cerr << "buffer handling optimized for a high-output producer" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: cat full.csv | csv-thin [<rate>] [<options>] > thinned.csv" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << "    --binary,-b=<format>: data is packets of fixed size given by <format>." << std::endl;
    std::cerr << "                          Alternatively use --size" << std::endl;
    std::cerr << "    --deterministic,-d: input is downsampled by a factor of int( 1 / <rate> )." << std::endl;
    std::cerr << "                That is, if <rate> is 0.33, output every third packet." << std::endl;
    std::cerr << "                Default is to output each packet with a probability of <rate>." << std::endl;
    std::cerr << "    --fields=<fields>: use timestamp in fields to determine time for --period" << std::endl;
    std::cerr << "    --fps,--frames-per-second=<d>: deprecated and removed" << std::endl;
    std::cerr << "    --period=<n>: output once every <n> seconds, ignores <rate>" << std::endl;
    std::cerr << "    --size,-s=<size>: data is packets of fixed size, otherwise data is expected" << std::endl;
    std::cerr << "                      line-wise. Alternatively use --binary" << std::endl;
    std::cerr << std::endl;
    if( detail )
    {
        std::cerr << "csv options:" << std::endl;
        std::cerr<< comma::csv::options::usage() << std::endl;
        std::cerr << std::endl;
    }
    else
    {
        std::cerr << "use -v or --verbose to see more detail" << std::endl;
        std::cerr << std::endl;
    }
    std::cerr << "examples:" << std::endl;
    std::cerr << "    output 70% of data:          cat full.csv | csv-thin 0.7" << std::endl;
    std::cerr << "    output once every 2 seconds: cat full.csv | csv-thin --period 2" << std::endl;
    std::cerr << "    using timestamp from input:  cat full.csv | csv-thin --period 2 --fields t" << std::endl;
    std::cerr << "    binary data:                 cat full.bin | csv-thin 0.1 --binary 3d" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 1 );
}

static double rate;
static bool deterministic;
static boost::optional< boost::posix_time::microseconds > period;

struct timestamped
{
    boost::posix_time::ptime timestamp;
    timestamped() {}
    timestamped( const boost::posix_time::ptime& timestamp ) : timestamp( timestamp ) {}
};

namespace comma { namespace visiting {

template <> struct traits< timestamped >
{
    template < typename K, typename V > static void visit( const K&, const timestamped& p, V& v ) { v.apply( "t", p.timestamp ); }
    template < typename K, typename V > static void visit( const K&, timestamped& p, V& v ) { v.apply( "t", p.timestamp ); }
};

} } // namespace comma { namespace visiting {

static bool ignore()
{
    if( period )
    {
        static boost::posix_time::ptime next_time = boost::posix_time::microsec_clock::universal_time();
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
        if( now > next_time )
        {
            next_time += *period;
            return false;
        }
        return true;
    }
    if( deterministic )
    {
        /*
        static unsigned int count = 1;
        static unsigned int kept = 1;
        if( double( kept ) / count > rate ) { ++count; return false; }
        if( kept == rate * count ) { count = 0; kept = 0; }
        ++kept;
        ++count;
        return true;
        */
        
        static const unsigned long long size = 1.0e+9;
        static unsigned long long step = 0;
        static unsigned long long count = 0;
        ++count;
        if( count < ( step + 1 ) / rate ) { return true; }
        ++step;
        if( step == size )
        {
            count = 0;
            step = 0;
        }
        return false;
    }
    static boost::mt19937 rng;
    static boost::uniform_real<> dist( 0, 1 );
    static boost::variate_generator< boost::mt19937&, boost::uniform_real<> > random( rng, dist );
    static bool do_ignore = comma::math::less( rate, 1.0 );
    return do_ignore && random() > rate;
}

static bool ignore_with_timestamp( boost::posix_time::ptime timestamp )
{
    static boost::posix_time::ptime next_time = timestamp;
    if( timestamp > next_time )
    {
        next_time += *period;
        return false;
    }
    return true;
}

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        bool binary = options.exists( "--size,-s,--binary,-b" );
        deterministic = options.exists( "--deterministic,-d" );
        if( options.exists( "--period" )) { period = boost::posix_time::microseconds( static_cast<unsigned int> (options.value< double >( "--period" ) * 1000000 )); }
        if(options.exists("--fps,--frames-per-second")) { COMMA_THROW( comma::exception, "ERROR: --fps option is deprecated and removed! Please talk to software team if you are using it"); }
        #ifdef WIN32
        if( binary ) { _setmode( _fileno( stdin ), _O_BINARY ); _setmode( _fileno( stdout ), _O_BINARY ); }
        #endif

        if( options.exists( "--fields" ))
        {
            if( !period ) { COMMA_THROW( comma::exception, "--fields requires --period option" ); }
            comma::csv::options csv( options );
            comma::csv::input_stream< timestamped > istream( std::cin, csv );
            while( std::cin.good() && !std::cin.eof() )
            {
                const timestamped* p = istream.read();
                if( !p ) { break; }
                if( !ignore_with_timestamp( p->timestamp ))
                {
                    if( istream.is_binary()) { std::cout.write( istream.binary().last(), istream.binary().size() ); }
                    else { std::cout << comma::join( istream.ascii().last(), istream.ascii().ascii().delimiter() )<< std::endl; }
                }
            }
            return 0;
        }
        
        std::vector< std::string > v;

        if( !period )
        {
            v = options.unnamed( "--deterministic,-d", "-.*" );
            if( v.empty() ) { std::cerr << "csv-thin: please specify rate" << std::endl; usage(); }
            rate = boost::lexical_cast< double >( v[0] );
            if( comma::math::less( rate, 0 ) || comma::math::less( 1, rate ) ) { std::cerr << "csv-thin: expected rate between 0 and 1, got " << rate << std::endl; usage(); }
        }

        if( binary ) // quick and dirty, improve performance by reading larger buffer
        {
            std::size_t size = options.value( "--size,-s", 0u );
            std::string format_string = options.value< std::string >( "--binary,-b", "" );
            boost::optional< comma::csv::format > f;
            if( !format_string.empty() ) { f.reset( comma::csv::format( format_string ) ); }
            if( !size ) { size = f->size(); }
            if( f && f->size() != size ) { std::cerr << "csv-thin: expected consistent size, got --size " << size << " and --binary of size " << f->size() << std::endl; return 1; }
            unsigned int factor = 65536 / size; // arbitrary
            if( factor == 0 ) { factor = 1; }
            std::vector< char > buf( size * factor );
            #ifdef WIN32
            while( std::cin.good() && !std::cin.eof() )
            {
                // it all does not seem to work: in_avail() always returns 0
                //std::streamsize available = std::cin.rdbuf()->in_avail();
                //if( available < 0 ) { continue; }
                //if( available > 0 ) { std::cerr << "available = " << available << std::endl; }
                //std::size_t e = available < int( size ) ? size : available - available % size;
                std::cin.read( &buf[0], size ); // quick and dirty
                if( std::cin.gcount() <= 0 ) { break; }
                if( std::cin.gcount() < int( size ) ) { std::cerr << "csv-thin: expected " << size << " bytes; got only " << std::cin.gcount() << std::endl; exit( 1 ); }
                { if( !ignore() ) { std::cout.write( &buf[0], size ); std::cout.flush(); } }
            }
            #else
            char* cur = &buf[0];
            unsigned int offset = 0;
            unsigned int capacity = buf.size();
            while( std::cin.good() && !std::cin.eof() )
            {
                int count = ::read( comma::io::stdin_fd, cur + offset, capacity );
                if( count <= 0 )
                {
                    if( offset != 0 ) { std::cerr << "csv-thin: expected at least " << size << " bytes, got only " << offset << std::endl; return 1; }
                    break;
                }
                offset += count;
                capacity -= count;
                for( ; offset >= size; cur += size, offset -= size )
                {
                    if( !ignore() ) { std::cout.write( cur, size ); }
                }
                if( capacity == 0 ) { cur = &buf[0]; offset = 0; capacity = buf.size(); }
                std::cout.flush();
            }
            #endif
        }
        else
        {
            std::string line;
            while( std::cin.good() && !std::cin.eof() )
            {
                std::getline( std::cin, line );
                if( !line.empty() && !ignore() ) { std::cout << line << std::endl; }
            }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "csv-size: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-size: unknown exception" << std::endl; }
    usage();
}

// // This file is part of comma, a generic and flexible library
// // Copyright (c) 2011 The University of Sydney
// // All rights reserved.
// //
// // Redistribution and use in source and binary forms, with or without
// // modification, are permitted provided that the following conditions are met:
// // 1. Redistributions of source code must retain the above copyright
// //    notice, this list of conditions and the following disclaimer.
// // 2. Redistributions in binary form must reproduce the above copyright
// //    notice, this list of conditions and the following disclaimer in the
// //    documentation and/or other materials provided with the distribution.
// // 3. All advertising materials mentioning features or use of this software
// //    must display the following acknowledgement:
// //    This product includes software developed by the University of Sydney.
// // 4. Neither the name of the University of Sydney nor the
// //    names of its contributors may be used to endorse or promote products
// //    derived from this software without specific prior written permission.
// //
// // NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// // GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// // HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
// // WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// // MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// // DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// // LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// // CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// // SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// // BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// // WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// // OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// // IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// /// @author vsevolod vlaskine
//
// #ifdef WIN32
// #include <stdio.h>
// #include <fcntl.h>
// #include <io.h>
// #endif
//
// #include <iostream>
// #include <boost/random/mersenne_twister.hpp>
// #include <boost/random/uniform_real.hpp>
// #include <boost/random/variate_generator.hpp>
// #include <comma/application/command_line_options.h>
// #include <comma/application/contact_info.h>
// #include <comma/base/exception.h>
// #include <comma/base/types.h>
// #include <comma/io/file_descriptor.h>
// #include <comma/math/compare.h>
//
// using namespace comma;
//
// static void usage()
// {
//     std::cerr << std::endl;
//     std::cerr << "Read input data and thin them down by the given percentage;" << std::endl;
//     std::cerr << "buffer handling optimized for a high-output producer" << std::endl;
//     std::cerr << std::endl;
//     std::cerr << "Usage: cat full.csv | csv-thin <rate> [<options>] > thinned.csv" << std::endl;
//     std::cerr << std::endl;
//     std::cerr << "e.g. output 70% of data:  cat full.csv | csv-thin 0.7 > thinned.csv" << std::endl;
//     std::cerr << std::endl;
//     std::cerr << "<options>" << std::endl;
//     std::cerr << std::endl;
//     std::cerr << "    --size,-s <size>: if given, data is packets of fixed size" << std::endl;
//     std::cerr << "                      otherwise data is line-based" << std::endl;
//     std::cerr << "    --deterministic,-d: if given, input is downsampled by a factor of int(1 / <rate>)." << std::endl;
//     std::cerr << "                     That is, if <rate> is 0.33, output every third packet." << std::endl;
//     std::cerr << "                     Default is to output each packet with a probability of <rate>." << std::endl;
//     std::cerr << std::endl;
//     std::cerr << comma::contact_info << std::endl;
//     std::cerr << std::endl;
//     exit( 1 );
// }
//
// static double rate;
// static bool deterministic;
// static unsigned long long count_size;
//
// static bool ignore()
// {
//     if( deterministic )
//     {
//         static unsigned long long count = count_size - 1;
//         if( ++count == count_size ) { count = 0; }
//         if(rate<0.5)
//         {
//             return count != 0;
//         }
//         else
//         {
//             return count == 0;
//         }
//     }
//     static boost::mt19937 rng;
//     static boost::uniform_real<> dist( 0, 1 );
//     static boost::variate_generator< boost::mt19937&, boost::uniform_real<> > random( rng, dist );
//     static bool do_ignore = comma::math::less( rate, 1.0 );
//     return do_ignore && random() > rate;
//
// }
//
// int main( int ac, char** av )
// {
//     try
//     {
//         comma::command_line_options options( ac, av );
//         if( options.exists( "--help,-h" ) || ac == 1 ) { usage(); }
//         bool binary = options.exists( "--size,-s" );
//         deterministic = options.exists( "--deterministic,-d" );
//         std::size_t size = options.value( "--size,-s", 0u );
//         #ifdef WIN32
//         if( binary ) { _setmode( _fileno( stdin ), _O_BINARY ); _setmode( _fileno( stdout ), _O_BINARY ); }
//         #endif
//         std::vector< std::string > v = options.unnamed( "--deterministic,-d", "-.*" );
//         if( v.empty() ) { std::cerr << "csv-thin: please specify rate" << std::endl; usage(); }
//         rate = boost::lexical_cast< double >( v[0] );
//         if( comma::math::less( rate, 0 ) || comma::math::less( 1, rate ) ) { std::cerr << "csv-thin: expected rate between 0 and 1, got " << rate << std::endl; usage(); }
//
//         if( deterministic )
//         {
//             if(rate<0.5)
//             {
//                 count_size = static_cast< unsigned long long >( 1.0 / rate );
//             }
//             else
//             {
//                 count_size = static_cast< unsigned long long >( 1.0 / (1-rate) );
//             }
//         }
//
//         if( binary ) // quick and dirty, improve performance by reading larger buffer
//         {
//             unsigned int factor = 65536 / size; // arbitrary
//             if( factor == 0 ) { factor = 1; }
//             std::vector< char > buf( size * factor );
//             #ifdef WIN32
//             while( std::cin.good() && !std::cin.eof() )
//             {
//                 // it all does not seem to work: in_avail() always returns 0
//                 //std::streamsize available = std::cin.rdbuf()->in_avail();
//                 //if( available < 0 ) { continue; }
//                 //if( available > 0 ) { std::cerr << "available = " << available << std::endl; }
//                 //std::size_t e = available < int( size ) ? size : available - available % size;
//                 std::cin.read( &buf[0], size ); // quick and dirty
//                 if( std::cin.gcount() <= 0 ) { break; }
//                 if( std::cin.gcount() < int( size ) ) { std::cerr << "csv-thin: expected " << size << " bytes; got only " << std::cin.gcount() << std::endl; exit( 1 ); }
//                 { if( !ignore() ) { std::cout.write( &buf[0], size ); std::cout.flush(); } }
//             }
//             #else
//             char* cur = &buf[0];
//             unsigned int offset = 0;
//             unsigned int capacity = buf.size();
//             while( std::cin.good() && !std::cin.eof() )
//             {
//                 int count = ::read( comma::io::stdin_fd, cur + offset, capacity );
//                 if( count <= 0 )
//                 {
//                     if( offset != 0 ) { std::cerr << "csv-thin: expected at least " << size << " bytes, got only " << offset << std::endl; return 1; }
//                     break;
//                 }
//                 offset += count;
//                 capacity -= count;
//                 for( ; offset >= size; cur += size, offset -= size )
//                 {
//                     if( !ignore() ) { std::cout.write( cur, size ); }
//                 }
//                 if( capacity == 0 ) { cur = &buf[0]; offset = 0; capacity = buf.size(); }
//                 std::cout.flush();
//             }
//             #endif
//         }
//         else
//         {
//             std::string line;
//             while( std::cin.good() && !std::cin.eof() )
//             {
//                 std::getline( std::cin, line );
//                 if( !line.empty() && !ignore() ) { std::cout << line << std::endl; }
//             }
//         }
//         return 0;
//     }
//     catch( std::exception& ex ) { std::cerr << "csv-size: " << ex.what() << std::endl; }
//     catch( ... ) { std::cerr << "csv-size: unknown exception" << std::endl; }
//     usage();
// }
