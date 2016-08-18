// This file is part of comma, a generic and flexible library
// Copyright (c) 2016 The University of Sydney
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
#include <numeric>
#include <boost/array.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../application/signal_flag.h"
#include "../../io/select.h"
#include "../../io/stream.h"

static unsigned int default_window = 10;
static unsigned int default_update_interval = 1;
static char default_delimiter = ',';

static void bash_completion( unsigned const ac, char const * const * av )
{
    static const char* completion_options =
        " --help -h"
        " --window -w --update -u"
        " --delimiter -d --output-fields"
        ;
    std::cout << completion_options << std::endl;
    exit( 0 );
}

void usage( bool verbose = false )
{
    std::cerr << std::endl;
    std::cerr << "pass stdin to stdout and echo bandwidth to stderr" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: io-bandwidth [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --window,-w=[<n>]: sliding window; default=" << default_window << "s" << std::endl;
    std::cerr << "    --update,-u=[<n>]: update interval; default=" << default_update_interval << "s" << std::endl;
    std::cerr << "    --output-fields: list output fields and exit" << std::endl;
    std::cerr << "    --delimiter,-d <delimiter>: default ','" << std::endl;
    std::cerr << std::endl;
    std::cerr << "example" << std::endl;
    std::cerr << "    --- pass data to hexdump and publish bandwidth stats on port 8888 ---" << std::endl;
    std::cerr << "    while : ; do" << std::endl;
    std::cerr << "        dd if=/dev/urandom bs=100 count=1 2> /dev/null; sleep 0.1" << std::endl;
    std::cerr << "    done | io-bandwidth 2> >( io-publish tcp:8888 ) | hexdump" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

static const unsigned int wait_interval = 10; // milliseconds
static const unsigned int bucket_width = 1;   // seconds

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        if( options.exists( "--bash-completion" ) ) bash_completion( ac, av );

        if( options.exists( "--output-fields" )) { std::cout << "timestamp,elapsed_time,received_bytes,bandwidth/average,bandwidth/window" << std::endl; return 0; }

        unsigned int update_interval = options.value< unsigned int >( "--update,-u", default_update_interval );
        unsigned int window = options.value< unsigned int >( "--window,-w", default_window );
        char delimiter = options.value( "--delimiter,-d", default_delimiter );

        comma::io::select select;
        select.read().add( comma::io::stdin_fd );
        comma::io::istream is( "-", comma::io::mode::binary );

        unsigned int total_bytes = 0;
        unsigned int bucket_bytes = 0;
        boost::circular_buffer< unsigned int > window_buckets( window );
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();
        boost::posix_time::ptime next_update = start_time + boost::posix_time::seconds( update_interval );
        boost::posix_time::ptime next_bucket = start_time + boost::posix_time::seconds( bucket_width );

        comma::signal_flag is_shutdown;
        bool end_of_stream = false;
        boost::array< char, 4096 > buffer;

        while( !is_shutdown && !end_of_stream )
        {
            select.wait( boost::posix_time::milliseconds( wait_interval ));

            while( select.check() && select.read().ready( is.fd() ) && is->good() )
            {
                std::size_t available = is.available();
                if( available == 0 ) { end_of_stream = true; break; }
                std::size_t size = std::min( available, buffer.size() );
                is->read( &buffer[0], size );
                bucket_bytes += size;
                total_bytes += size;
                std::cout.write( &buffer[0], size );
                std::cout.flush();
            }

            boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();

            if( now >= next_bucket )
            {
                window_buckets.push_back( bucket_bytes );
                bucket_bytes = 0;
                next_bucket += boost::posix_time::seconds( bucket_width );
            }

            if( now >= next_update )
            {
                boost::posix_time::time_duration elapsed = now - start_time;
                double elapsed_seconds = (double)elapsed.total_milliseconds() / 1000.0f;
                boost::posix_time::ptime now_to_nearest_second = boost::posix_time::ptime( now.date(), boost::posix_time::seconds( now.time_of_day().total_seconds() ));

                std::cerr << boost::posix_time::to_iso_string( now_to_nearest_second ) << delimiter
                          << elapsed_seconds << delimiter
                          << total_bytes << delimiter
                          << (double)total_bytes / elapsed_seconds << delimiter
                          << (double)std::accumulate( window_buckets.begin(), window_buckets.end(), 0.0f )
                                 / window_buckets.size() / bucket_width
                          << std::endl;

                next_update += boost::posix_time::seconds( update_interval );
            }
        }
        return 0;
    }
    catch( std::exception& ex ) { std::cerr << "io-bandwidth: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "io-bandwidth: unknown exception" << std::endl; }
    return 1;
}