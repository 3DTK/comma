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

#include <string.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <boost/array.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/static_assert.hpp>
#include <boost/unordered_map.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../application/signal_flag.h"
#include "../../base/exception.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../csv/traits.h"
#include "../../io/stream.h"
#include "../../math/compare.h"
#include "../../name_value/parser.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

static void usage( bool more )
{
    std::cerr << std::endl;
    std::cerr << "join two csv files or streams by one or several key fields" << std::endl;
    std::cerr << std::endl;
    std::cerr << "if no key fields specified, join each line of stdin input with each line of the second stream" << std::endl;
    std::cerr << std::endl;
    std::cerr << "if block field present, read and join block by block" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat something.csv | csv-join \"something_else.csv[,options]\" [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --help,-h: help; --help --verbose: more help" << std::endl;
    std::cerr << "    --first-matching: output only the first matching record (a bit of hack for now, but we needed it)" << std::endl;
    std::cerr << "    --flag-matching: output all records, with 1 appended to matching records and 0 appended to not-matching records" << std::endl;
    std::cerr << "    --matching: output only matching records from stdin" << std::endl;
    std::cerr << "    --nearest: if --radius specified, output only nearest record" << std::endl;
    std::cerr << "    --not-matching: not matching records as read from stdin, no join performed" << std::endl;
    std::cerr << "    --strict: fail, if id on stdin is not found, or there are multiple filter keys on --unique, etc" << std::endl;
    std::cerr << "    --radius,--epsilon=<value>; compare keys in given radius; the keys will be interpreted as floating point numbers" << std::endl;
    std::cerr << "    --swap-output,--swap; output filter records first with the stdin record appended, a convenience option" << std::endl;
    std::cerr << "    --unique,--unique-matches: expect only unique matches, exit with error otherwise" << std::endl;
    std::cerr << "    --verbose,-v: more output to stderr" << std::endl;
    std::cerr << std::endl;
    std::cerr << "key field options" << std::endl;
    std::cerr << "    --string,-s: keys are strings; a quick and dirty option to support strings" << std::endl;
    std::cerr << "    --double: keys are doubles (no epsilon applied)" << std::endl;
    std::cerr << "    --time: keys are timestamps" << std::endl;
    std::cerr << "    default key type is integer" << std::endl;
    std::cerr << std::endl;
    std::cerr << "finite state machine" << std::endl;
    std::cerr << "    given a state-transition table as a file/stream containing both the fields 'state' and 'next_state'" << std::endl;
    std::cerr << "    csv-join will perform joins on the given keys (e.g. event) and an internal state with" << std::endl;
    std::cerr << "    the 'state' in the transition table" << std::endl;
    std::cerr << "    upon a match csv-join will output the match and update its internal state to 'next_state'" << std::endl;
    std::cerr << "    note:" << std::endl;
    std::cerr << "        * this mode expects unique matches" << std::endl;
    std::cerr << "        * this mode is only activated when the file/stream fields contain both 'state' and 'next_state'" << std::endl;
    std::cerr << "    --initial-state,--state: initial internal state (default: 0)" << std::endl;
    if( more )
    {
        std::cerr << std::endl;
        std::cerr << "csv options:" << std::endl;
        std::cerr << comma::csv::options::usage();
        std::cerr << std::endl;
        std::cerr << "    field names:" << std::endl;
        std::cerr << "        block: block number" << std::endl;
        std::cerr << "        any other field name: key" << std::endl;
        std::cerr << std::endl;
        std::cerr << "        block acts as a key but stream processing occurs at the end of each" << std::endl;
        std::cerr << "        block. If no block field is given the entire input is considered to be" << std::endl;
        std::cerr << "        one block. Blocks are required to be contiguous in the input stream." << std::endl;
    }
    std::cerr << std::endl;
    std::cerr << "Examples (try them):" << std::endl;
    std::cerr << "    on the following data file:" << std::endl;
    std::cerr << "        echo 1,1,2,hello > data.csv" << std::endl;
    std::cerr << "        echo 1,2,3,hello >> data.csv" << std::endl;
    std::cerr << "        echo 3,3,4,world >> data.csv" << std::endl;
    std::cerr << "        echo 3,4,3,world >> data.csv" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    join with a matching record" << std::endl;
    std::cerr << "        echo 1,blah | csv-join --fields=id \"data.csv;fields=id\"" << std::endl;
    std::cerr << "        echo 3,blah | csv-join --fields=id \"data.csv;fields=,,id\"" << std::endl;
    std::cerr << "        echo 5,blah | csv-join --fields=id \"data.csv;fields=,,id\"" << std::endl;
    std::cerr << "        echo 5,blah | csv-join --fields=id \"data.csv;fields=,,id\" --not-matching" << std::endl;
    std::cerr << "        echo 5,blah | csv-join --fields=id \"data.csv;fields=,,id\" --strict" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    join by key which is a string" << std::endl;
    std::cerr << "        echo 1,hello | csv-join --fields=,id \"data.csv;fields=,,,id\" --string" << std::endl;
    std::cerr << "        echo 1,world | csv-join --fields=,id \"data.csv;fields=,,,id\" --string" << std::endl;
    std::cerr << "        echo 1,blah | csv-join --fields=,id \"data.csv;fields=,,,id\" --string" << std::endl;
    std::cerr << "        echo 1,blah | csv-join --fields=,id \"data.csv;fields=,,,id\" --string --not-matching" << std::endl;
    std::cerr << "        echo 1,blah | csv-join --fields=,id \"data.csv;fields=,,,id\" --string --strict" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    finite state machine" << std::endl;
    std::cerr << "        csv-join --fields=event \"data.csv;fields=event,state,next_state\" --initial-state 1" << std::endl;
    std::cerr << "        <input:1>" << std::endl;
    std::cerr << "        <input:1>" << std::endl;
    std::cerr << "        <input:3>" << std::endl;
    std::cerr << "        <input:3>" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( 0 );
}

static bool verbose;
static bool first_matching;
static bool nearest;
static bool unique;
static bool strict;
static bool not_matching;
static bool matching;
static bool flag_matching;
static bool swap_output;
static comma::csv::options stdin_csv;
static comma::csv::options filter_csv;
boost::scoped_ptr< comma::io::istream > filter_transport;
static comma::uint32 block = 0;
static boost::optional< double > radius;

static void hash_combine_( std::size_t& seed, boost::posix_time::ptime key )
{
    BOOST_STATIC_ASSERT( sizeof( boost::posix_time::ptime ) == 8 );
    boost::hash_combine( seed, *reinterpret_cast< const long long* >( &key ) );
}

template < typename K > static void hash_combine_( std::size_t& seed, K key ) { boost::hash_combine( seed, key ); }

template < typename K >
struct input
{
    std::vector< K > keys;

    comma::uint32 block;
    K next_state;

    input() : block( 0 ) {}
    input( const input& i ) : keys( i.keys ), block( i.block ), next_state( i.next_state ) {}

    bool operator==( const input& rhs ) const
    {
        for( std::size_t i = 0; i < keys.size(); ++i ) { if( keys[i] != rhs.keys[i] ) { return false; } }
        return true;
    }

    bool operator<( const input& rhs ) const
    {
        if( keys.empty() ) { COMMA_THROW( comma::exception, "if --radius given, expected exactly one key, got none" ); }
        if( keys.size() > 1 ) { COMMA_THROW( comma::exception, "if --radius given, expected one key, got: " << keys.size() ); }
        return comma::math::less( keys[0], rhs.keys[0] ); //, *radius );
    }

    struct hash : public std::unary_function< input, std::size_t >
    {
        std::size_t operator()( input const& p ) const
        {
            std::size_t seed = 0;
            for( std::size_t i = 0; i < p.keys.size(); ++i ) { hash_combine_( seed, p.keys[i] ); }
            return seed;
        }
    };

    typedef boost::unordered_map< input, std::vector< std::string >, hash > unordered_map;
    typedef std::map< input, std::vector< std::string > > map;
};

template < typename K, bool Strict = true > struct traits
{
    typedef typename input< K >::unordered_map map;
    typedef std::pair< typename map::const_iterator, typename map::const_iterator > pair;
    static pair find( map& m, const input< K >& k, bool )
    {
        typename map::const_iterator it = m.find( k );
        if( it == m.end() ) { return std::make_pair( it, it ); }
        typename map::const_iterator end = it;
        ++end;
        return std::make_pair( it, end );
    }
};

template < typename K > struct type_traits
{
    template < typename Map >
    static typename std::pair< typename Map::iterator, typename Map::iterator > bounds( Map& m, const input< K >& k ) // quick and dirty
    {
        input< K > l = k; // quick and dirty; lame
        input< K > u = k;
        l.keys[0] -= *radius;
        u.keys[0] += *radius;
        return std::make_pair( m.lower_bound( l ), m.upper_bound( u ) );
    }
    
    template < typename Map >
    static typename Map::iterator nearest( Map& m, const input< K >& k ) // quick and dirty
    { 
        auto b = type_traits< K >::bounds( m, k );
        typename Map::iterator min = b.first;
        for( typename Map::iterator it = b.first; it != b.second; ++it )
        {
            if( std::abs( min->first.keys[0] - k.keys[0] ) > std::abs( it->first.keys[0] - k.keys[0] ) ) { min = it; }
        }
        return min;
    }
};

template <> struct type_traits< std::string > // quick and dirty
{
    template < typename Map >
    static std::pair< typename Map::iterator, typename Map::iterator > bounds( Map& m, const input< std::string >& k ) { COMMA_THROW( comma::exception, "never here" ); }    
    template < typename Map > static typename Map::iterator nearest( Map& m, const input< std::string >& ) { COMMA_THROW( comma::exception, "never here" ); }
};

template <> struct type_traits< boost::posix_time::ptime > // quick and dirty
{
    template < typename Map >
    static typename std::pair< typename Map::iterator, typename Map::iterator > bounds( Map& m, const input< boost::posix_time::ptime >& k ) { COMMA_THROW( comma::exception, "never here" ); }
    template < typename Map >
    static typename Map::iterator nearest( Map& m, const input< boost::posix_time::ptime >& ) { COMMA_THROW( comma::exception, "never here" ); }
};

template < typename K > struct traits< K, false >
{
    typedef typename input< K >::map map;
    typedef std::pair< typename map::iterator, typename map::iterator > pair; //typedef std::pair< typename map::const_iterator, typename map::const_iterator > pair;
    static pair find( map& m, const input< K >& k, bool nearest )
    {
        if( !nearest ) { return type_traits< K >::bounds( m, k ); }
        auto n = type_traits< K >::nearest( m, k );
        if( n == m.end() ) { return std::make_pair( n, n ); }
        auto end = n;
        return std::make_pair( n, ++end );
    }
};

namespace comma { namespace visiting {

template < typename T > struct traits< input< T > >
{
    template < typename K, typename V > static void visit( const K&, const input< T >& p, V& v )
    {
        v.apply( "keys", p.keys );
        v.apply( "block", p.block );
        v.apply( "next_state", p.next_state );
    }
    template < typename K, typename V > static void visit( const K&, input< T >& p, V& v )
    {
        v.apply( "keys", p.keys );
        v.apply( "block", p.block );
        v.apply( "next_state", p.next_state );
    }
};

} } // namespace comma { namespace visiting {

template < typename T > static std::string keys_as_string( const input< T >& i ) // quick and dirty
{
    std::ostringstream oss;
    comma::csv::options csv;
    csv.fields = "keys";
    comma::csv::ascii_output_stream< input< T > > os( oss, csv, i );
    os.write( i );
    return oss.str();
}

template < typename K, bool Strict = true > struct join_impl_ // quick and dirty
{
    static typename traits< K, Strict >::map filter_map;
    static input< K > default_input;

    static void read_filter_block()
    {
        static comma::csv::input_stream< input< K > > filter_stream( **filter_transport, filter_csv, default_input );
        static const input< K >* last = filter_stream.read();
        filter_map.clear();
        if( !last ) { return; }
        block = last->block;
        comma::uint64 count = 0;
        static comma::signal_flag is_shutdown( comma::signal_flag::hard );
        while( last->block == block && !is_shutdown )
        {
            typename traits< K, Strict >::map::mapped_type& d = filter_map[ *last ];
            if( filter_stream.is_binary() )
            {
                typename traits< K, Strict >::map::mapped_type& d = filter_map[ *last ];
                d.push_back( std::string() );
                d.back().resize( filter_csv.format().size() );
                ::memcpy( &d.back()[0], filter_stream.binary().last(), filter_csv.format().size() );
            }
            else
            {
                d.push_back( comma::join( filter_stream.ascii().last(), stdin_csv.delimiter ) );
            }
            if( verbose ) { ++count; if( count % 10000 == 0 ) { std::cerr << "csv-join: reading block " << block << "; loaded " << count << " point" << ( count == 1 ? "" : "s" ) << "; hash map size: " << filter_map.size() << std::endl; } }
            //if( ( *filter_transport )->good() && !( *filter_transport )->eof() ) { break; }
            last = filter_stream.read();
            if( !last ) { break; }
        }
        if( verbose ) { std::cerr << "csv-join: read block " << block << " of " << count << " point" << ( count == 1 ? "" : "s" ) << "; hash map size: " << filter_map.size() << std::endl; }
    }

    static int run( const comma::command_line_options& options )
    {
        std::vector< std::string > v = comma::split( stdin_csv.fields, ',' );
        std::vector< std::string > w = comma::split( filter_csv.fields, ',' );
        bool got_state = false;
        bool got_next_state = false;
        std::size_t filter_state_index;
        for( std::size_t k = 0; k < w.size() && ( !got_state || !got_next_state ); ++k ) 
        {
            if( w[k] == "state" ) { got_state = true; filter_state_index = k; continue; }
            if( w[k] == "next_state" ) { got_next_state = true; continue; }
        }
        bool is_state_machine = got_state && got_next_state;
        std::size_t default_input_keys_count = 0;
        bool no_stdin_key_fields = true;
        bool no_filter_key_fields = true;
        for( std::size_t i = 0; i < v.size(); ++i ) // quick and dirty, wasteful, but who cares
        {
            if( v[i].empty() || v[i] == "block" ) { continue; }
            no_stdin_key_fields = false;
            for( std::size_t k = 0; k < w.size(); ++k )
            {
                if( is_state_machine && ( w[k] == "state" || w[k] == "next_state" ) ) { no_filter_key_fields = false; continue; }
                if( !w[k].empty() && w[k] != "block" ) { no_filter_key_fields = false; }
                if( v[i] != w[k] ) { continue; }
                v[i] = "keys[" + boost::lexical_cast< std::string >( default_input_keys_count ) + "]";
                w[k] = "keys[" + boost::lexical_cast< std::string >( default_input_keys_count ) + "]";
                ++default_input_keys_count;
            }
        }
        bool do_full_join = no_stdin_key_fields && no_filter_key_fields;
        if( default_input_keys_count == 0 && !do_full_join ) { std::cerr << "csv-join: please specify at least one common key; fields: " << stdin_csv.fields << "; filter fields: " << filter_csv.fields << std::endl; return 1; }
        //if( default_input_keys_count == 0 ) { std::cerr << "csv-join: please specify at least one common key; fields: " << stdin_csv.fields << "; filter fields: " << filter_csv.fields << std::endl; return 1; }
        K state = options.value< K >( "--initial-state,--state", K() );
        std::size_t state_index;
        if( is_state_machine )
        {
            state_index = default_input_keys_count;
            w[filter_state_index] = "keys[" + boost::lexical_cast< std::string >( state_index ) + "]";
            ++default_input_keys_count;
        }
        default_input.keys.resize( default_input_keys_count );
        stdin_csv.fields = comma::join( v, ',' );
        filter_csv.fields = comma::join( w, ',' );
        if( do_full_join )
        {
            if( stdin_csv.fields.empty() ) { stdin_csv.fields = "stdin/dummy"; }
            if( filter_csv.fields.empty() ) { filter_csv.fields = "filter/dummy"; }
        }
        comma::csv::input_stream< input< K > > stdin_stream( std::cin, stdin_csv, default_input );
        filter_transport.reset( new comma::io::istream( filter_csv.filename, filter_csv.binary() ? comma::io::mode::binary : comma::io::mode::ascii ) );
        if( filter_transport->fd() == comma::io::invalid_file_descriptor ) { std::cerr << "csv-join: failed to open \"" << filter_csv.filename << "\"" << std::endl; return 1; }
        std::size_t discarded = 0;
        read_filter_block();
        #ifdef WIN32
        if( stdin_stream.is_binary() ) { _setmode( _fileno( stdout ), _O_BINARY ); }
        #endif
        while( stdin_stream.ready() || std::cin.good() )
        {
            const input< K >* p = stdin_stream.read();
            if( !p ) { break; }
            if( block != p->block ) { read_filter_block(); }
            typename traits< K, Strict >::pair pair;
            if( is_state_machine )
            {
                input< K > q( *p );
                q.keys[ state_index ] = state;
                pair = traits< K, Strict >::find( filter_map, q, false );
            }
            else
            {
                pair = traits< K, Strict >::find( filter_map, *p, nearest );
            }
            if( pair.first == filter_map.end() ) // if( it == filter_map.end() || it->second.empty() )
            {
                if( not_matching )
                {
                    if( stdin_stream.is_binary() ) { std::cout.write( stdin_stream.binary().last(), stdin_csv.format().size() ); }
                    else { std::cout << comma::join( stdin_stream.ascii().last(), stdin_csv.delimiter ) << std::endl; }
                    continue;
                }
                if ( flag_matching )
                {
                    if( stdin_stream.is_binary() ) { 
                        std::cout.write( stdin_stream.binary().last(), stdin_csv.format().size() ); 
                        char match = 0; std::cout.write( &match, 1 );
                    }
                    else { std::cout << comma::join( stdin_stream.ascii().last(), stdin_csv.delimiter ) << stdin_csv.delimiter << 0 << std::endl; }
                    continue;
                }
                if( !strict ) { ++discarded; continue; }
                std::string s;
                comma::csv::options c;
                c.fields = "keys";
                std::cerr << "csv-join: match not found for key(s): " << comma::csv::ascii< input< K > >( c, default_input ).put( *p, s ) << ", block: " << block << std::endl;
                return 1;
            }
            if( not_matching ) { continue; }
            for( typename traits< K, Strict >::map::const_iterator it = pair.first; it != pair.second; ++it )
            {
                if( unique && it->second.size() > 1 )
                {
                    if( strict ) { std::cerr << "csv-join: with --unique option, expected unique entries, got more than one filter entry on the key: " << keys_as_string( it->first ) << std::endl; return 1; }
                    if( verbose ) { std::cerr << "csv-join: got --unique option, but more than one filter entry on the key: " << keys_as_string( it->first ) << "; only the first entry will be output; use --strict to make it fatal error" << std::endl; }
                }
                if( is_state_machine && it->second.size() > 1 ) { std::cerr << "csv-join: finite state machine, expected unique entries, got more than one state transition entry on the key: " << keys_as_string( it->first ) << std::endl; return 1; }
                if( stdin_stream.is_binary() )
                {
                    for( std::size_t i = 0; i < ( first_matching || unique ? 1 : it->second.size() ); ++i )
                    {
                        if( !swap_output ) { std::cout.write( stdin_stream.binary().last(), stdin_csv.format().size() ); }
                        if( is_state_machine ) { state = it->first.next_state; }
                        if( flag_matching ) { char match = 1; std::cout.write( &match, 1 ); break; }
                        if( matching ) { break; }
                        std::cout.write( &( it->second[i][0] ), filter_csv.format().size() );
                        if( swap_output ) { std::cout.write( stdin_stream.binary().last(), stdin_csv.format().size() ); }
                        std::cout.flush();
                    }
                    std::cout.flush();
                }
                else
                {
                    for( std::size_t i = 0; i < ( first_matching || unique ? 1 : it->second.size() ); ++i )
                    {
                        if( !swap_output ) { std::cout << comma::join( stdin_stream.ascii().last(), stdin_csv.delimiter ); }
                        if( is_state_machine ) { state = it->first.next_state; }
                        if( flag_matching ) { std::cout << stdin_csv.delimiter << 1 << std::endl; break; }
                        if( matching ) { std::cout << std::endl; break; }
                        if( !swap_output ) { std::cout << stdin_csv.delimiter; }
                        std::cout << ( filter_csv.binary()
                                     ? filter_csv.format().bin_to_csv( &it->second[i][0], stdin_csv.delimiter )
                                     : it->second[i] );
                        if( swap_output ) { std::cout << stdin_csv.delimiter << comma::join( stdin_stream.ascii().last(), stdin_csv.delimiter ); }
                        std::cout << std::endl;
                    }
                }
                if( first_matching ) { break; }
            }
            if( first_matching ) { filter_map.erase( pair.first, pair.second ); }
        }
        if( verbose ) { std::cerr << "csv-join: discarded " << discarded << " " << ( discarded == 1 ? "entry" : "entries" ) << " with no matches" << std::endl; }
        return 0;
    }
};

template < typename K, bool Strict > typename traits< K, Strict >::map join_impl_< K, Strict >::filter_map;
template < typename K, bool Strict > input< K > join_impl_< K, Strict >::default_input;

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        verbose = options.exists( "--verbose,-v" );
        first_matching = options.exists( "--first-matching" );
        strict = options.exists( "--strict" );
        not_matching = options.exists( "--not-matching" );
        unique = options.exists( "--unique,--unique-matches" );
        matching = options.exists( "--matching" );
        flag_matching = options.exists( "--flag-matching" );
        radius = options.optional< double >( "--radius,--epsilon" );
        nearest = options.exists( "--nearest" );
        swap_output = options.exists( "--swap-output,--swap" );
        if( nearest && !radius ) { std::cerr << "csv-join: if using --nearest, please specify --radius" << std::endl; return 1; }
        options.assert_mutually_exclusive( "--matching,--not-matching,--flag-matching,--swap-output,--swap" );
        options.assert_mutually_exclusive( "--radius,--epsilon,--first-matching" );
        options.assert_mutually_exclusive( "--radius,--epsilon,--string,-s,--double,--time" );
        stdin_csv = comma::csv::options( options );
        std::vector< std::string > unnamed = options.unnamed( "--verbose,-v,--first-matching,--matching,--not-matching,--string,-s,--time,--double,--strict,--swap-output,--swap", "-.*" );
        if( unnamed.empty() ) { std::cerr << "csv-join: please specify the second source" << std::endl; return 1; }
        if( unnamed.size() > 1 ) { std::cerr << "csv-join: expected one file or stream to join, got " << comma::join( unnamed, ' ' ) << std::endl; return 1; }
        comma::name_value::parser parser( "filename", ';', '=', false );
        filter_csv = parser.get< comma::csv::options >( unnamed[0] );
        if( !matching && !not_matching && stdin_csv.binary() && !filter_csv.binary() ) { std::cerr << "csv-join: stdin stream binary and filter stream ascii: this combination is not supported" << std::endl; return 1; }
        return   options.exists( "--time" )
               ? join_impl_< boost::posix_time::ptime, true >::run( options )
               : options.exists( "--string,-s" )
               ? join_impl_< std::string, true >::run( options )
               : options.exists( "--double" )
               ? join_impl_< double, true >::run( options )
               : radius
               ? join_impl_< double, false >::run( options )
               : join_impl_< comma::int64, true >::run( options );
    }
    catch( std::exception& ex ) { std::cerr << "csv-join: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-join: unknown exception" << std::endl; }
    std::cerr << "csv-join: called as: " << comma::join( av, ac, ' ' ) << std::endl;
    return 1;
}
