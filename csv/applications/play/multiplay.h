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


/// @author cedric wohlleber

#ifndef COMMA_CSV_MULTIPLAY_H
#define COMMA_CSV_MULTIPLAY_H

#include <vector>
#include <boost/thread/thread_time.hpp>
#include "../../../csv/options.h"
#include "../../../csv/stream.h"
#include "../../../io/publisher.h"
#include "play.h"

namespace comma {

/// gets data from multiple input files, and output in a real time manner to output files,  using timestamps
class Multiplay
{
    public:
        struct time
        {
            time() {}
            time( boost::posix_time::ptime t ) : timestamp( t ) {}
            boost::posix_time::ptime timestamp;
        };

        struct SourceConfig
        {
            std::string outputFileName;
            std::size_t minNumberOfClients;
            csv::options options;
            boost::posix_time::time_duration offset;
            SourceConfig( const std::string& output, const csv::options& csv ) :
                outputFileName( output ), minNumberOfClients( 0 ), options( csv ) {}
            SourceConfig( const std::string& output, std::size_t n, const csv::options& csv ) :
                outputFileName( output ), minNumberOfClients( n ), options( csv ) {}
            SourceConfig() {};
        };

        Multiplay( const std::vector< SourceConfig >& configs
                , double speed = 1.0
                , bool quiet = false
                , const boost::posix_time::time_duration& resolution = boost::posix_time::milliseconds( 1 )
                , boost::posix_time::ptime from = boost::posix_time::not_a_date_time
                , boost::posix_time::ptime to = boost::posix_time::not_a_date_time
                , bool flush = true
                 );

        void close();

        bool read();
        
        boost::posix_time::ptime now() const { return now_; }

        void paused_for( const boost::posix_time::time_duration& pause_duration ) { m_play.paused_for( pause_duration ); }

    private:
        std::vector<SourceConfig> m_configs;
        std::vector< boost::shared_ptr< comma::io::istream > > istreams_;
        std::vector< boost::shared_ptr< csv::input_stream< time > > > m_inputStreams;
        std::vector< boost::shared_ptr< comma::io::publisher > > m_publishers;
        csv::impl::play m_play;
        std::vector< boost::posix_time::ptime > m_timestamps;
        boost::posix_time::ptime now_;
        bool m_started;
        boost::posix_time::ptime m_from;
        boost::posix_time::ptime m_to;
        std::vector< boost::shared_ptr< csv::ascii< time > > > ascii_;
        std::vector< boost::shared_ptr< csv::binary< time > > > binary_;
        std::vector< char > buf_fer;
        bool ready();
};

} // namespace comma {

namespace comma { namespace visiting {

template <> struct traits< comma::Multiplay::time >
{
    template < typename Key, class Visitor >
    static void visit( Key, comma::Multiplay::time& t, Visitor& v )
    {
        v.apply( "t", t.timestamp );
    }

    template < typename Key, class Visitor >
    static void visit( Key, const comma::Multiplay::time& t, Visitor& v )
    {
        v.apply( "t", t.timestamp );
    }
};

template <> struct traits< comma::Multiplay::SourceConfig >
{
    template < typename Key, class Visitor >
    static void visit( Key, comma::Multiplay::SourceConfig& c, Visitor& v )
    {
        v.apply( "options", c.options );
        v.apply( "output", c.outputFileName );
        v.apply( "clients", c.minNumberOfClients );
        double duration = 0;
        v.apply( "offset", duration );
		c.offset = boost::posix_time::microseconds( static_cast< boost::int64_t >( duration * 1e6 ) );
    }

    template < typename Key, class Visitor >
    static void visit( Key, const comma::Multiplay::SourceConfig& c, Visitor& v )
    {
        v.apply( "options", c.options );
        v.apply( "output", c.outputFileName );
        v.apply( "clients", c.minNumberOfClients );
        double duration = c.offset.total_microseconds();
        duration /= 1e6;
        v.apply( "offset", duration );
    }
};

} } // namespace comma { namespace visiting {

#endif // COMMA_CSV_MULTIPLAY_H
