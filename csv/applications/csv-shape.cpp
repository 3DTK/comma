// This file is part of comma, a generic and flexible library
// Copyright (c) 2017 The University of Sydney
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

/// @author dewey nguyen

#include <iostream>
#include <vector>
#include <deque>
#include "../../base/types.h"
#include "../../application/command_line_options.h"
#include "../options.h"
#include "../stream.h"
#include "../format.h"

using namespace comma;

void bash_completion( unsigned const ac, char const * const * av )
{
    static const char* completion_options =
        " concatenate loop flatten unflatten"
        " --help -h --verbose -v"
        " --binary -b --delimeter -d"
        " --size,-n"
        " --step"
        " --expected-records"
        " --bidirectional"
        " --reverse"
        " --sliding-window,-w"
        " --bidirectional"
        " --reverse"
        " --header --output"
        ;
    std::cout << completion_options << std::endl;
    exit( 0 );
}

static void usage( bool verbose=false )
{
    std::cerr << std::endl;
    std::cerr << "Perform reshaping operations on input data" << std::endl;
    std::cerr << std::endl;
    std::cerr << "operations" << std::endl;
    std::cerr << "    concatenate: group input records for concatenation into output records." << std::endl;
    std::cerr << "                 The user can choose non-overlapping or overlapping grouping" << std::endl;
    std::cerr << "                 (sliding window) mode." << std::endl;
    std::cerr << "    loop:        same as concatenate, but additionally the input record is" << std::endl;
    std::cerr << "                 concatenated with the first record (hence, 'loop')." << std::endl;
    std::cerr << "                 Always uses the sliding window for overlapping groups" << std::endl;
    std::cerr << "    flatten:     flatten block-wise data so it's one record per block" << std::endl;
    std::cerr << "    unflatten:   unflattens flattened data" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage: cat data.csv | csv-shape <operation> [<options>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options" << std::endl;
    std::cerr << "    --help,-h:  see this usage message" << std::endl;
    std::cerr << "    --verbose,-v: more output to stderr, shows examples with --help,-h" << std::endl;
    std::cerr << "    --binary,-b=[<format>]: in binary mode: format string of the input data" << std::endl;
    std::cerr << "    --delimiter,-d=[<char>]: default=','; in ascii mode, field separator" << std::endl;
    std::cerr << std::endl;
    if( verbose ) { std::cerr << comma::csv::format::usage() << std::endl; }
    std::cerr << "operations options" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   concatenate and loop options" << std::endl;
    std::cerr << "      --size,-n=<num>: number of input records in each grouping, where num >= 2" << std::endl;
    std::cerr << "      --step=<num>: default=1; relative offset of the records to be concatenated" << std::endl;
    std::cerr << "      --expected-records: output expected records for given --size and --step" << std::endl;
    std::cerr << "      --bidirectional: output records in both directions (e.g. a,b; b,a)" << std::endl;
    std::cerr << "      --reverse: output records in reverse order (e.g. b,a)" << std::endl;
    std::cerr << "      --sliding-window,-w: use a sliding window to group input records" << std::endl;
    std::cerr << "      --bidirectional: output records in both directions (e.g. a,b; b,a)" << std::endl;
    std::cerr << "      --reverse: output records in reverse order (e.g. b,a)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   flatten options" << std::endl;
    std::cerr << "      --header,--header-fields=[<fields>]: header fields to output" << std::endl;
    std::cerr << "      --output,--output-fields,-o=<fields>: recurring fields to output" << std::endl;
    std::cerr << std::endl;
    std::cerr << "      hhoo / hhoo / hhoo => hhoooooo (until a new block)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "   unflatten options" << std::endl;
    std::cerr << "      --header,--header-fields=[<fields>]: header fields in input" << std::endl;
    std::cerr << "      --recurring,--recurring-fields=[<fields>]: recurring fields in input" << std::endl;
    std::cerr << std::endl;
    std::cerr << "      hhrrrrrr => hhrr / hhrr / hhrr" << std::endl;
    std::cerr << std::endl;
    if( verbose )
    {
        std::cerr << "examples" << std::endl;   
        std::cerr << "   concatenate" << std::endl;
        std::cerr << "      non overlaping groups:" << std::endl;
        std::cerr << "         Concatenate each group of 5 input records into one output record." << std::endl;
        std::cerr << "         Input records 1 to 5 create the first output record, input records" << std::endl;
        std::cerr << "         6 to 10 create the second output record, and so forth." << std::endl;
        std::cerr << "            seq 1 15 | csv-shape concatenate -n 5" << std::endl;
        std::cerr << std::endl;
        std::cerr << "      overlapping groups:" << std::endl;
        std::cerr << "         Move a sliding window of size 5 along the input records, every time" << std::endl;
        std::cerr << "         the sliding window moves, make an output record from window." << std::endl;
        std::cerr << "         Input records 1 to 5 create the first output record, input records" << std::endl;
        std::cerr << "         2 to 6 create the second record, and so forth." << std::endl;
        std::cerr << "            seq 1 10 | csv-shape concatenate -n 5 --sliding-window" << std::endl;
        std::cerr << std::endl;
        std::cerr << "   flatten" << std::endl;
        std::cerr << "      echo -e \"a,0,1,2\\nb,0,3,4\\nc,1,5,6\\nd,1,7,8\" \\" << std::endl;
        std::cerr << "          | csv-shape flatten --fields s,block,a,b --output a,b --header s,block" << std::endl;
        std::cerr << std::endl;
        std::cerr << "   unflatten" << std::endl;
        std::cerr << "      echo -e \"a,0,1,2,w,3,4,x\\nc,1,5,6,y,7,8,z\" \\" << std::endl;
        std::cerr << "          | csv-shape unflatten --header s,block --recurring a,b,z" << std::endl;

    }
    else { std::cerr << "run csv-shape --help --verbose for examples of use" << std::endl; }
    std::cerr << std::endl;
    exit( 0 );
}

std::string operation;

class concatenate_op
{
public:
    concatenate_op()
        : use_sliding_window_(false)
        , bidirectional_(false)
        , reverse_(false)
        , looping_(false)
        , size_(0)
        , count_(0)
        , block_(0)
        , step_(1)
        , expected_records_(0)
        {}

    int run( const comma::command_line_options& options, const comma::csv::options& csv)
    {
        is_binary_ = csv.binary();
        looping_ = ( operation == "loop");
        use_sliding_window_ = ( looping_ || options.exists("--sliding-window,-w") );
        reverse_ = options.exists("--reverse");
        bidirectional_ = options.exists("--bidirectional");
        if( !use_sliding_window_ && is_binary_ ) { simple_binary_pass_through(csv.format(), csv.flush); return 0; };
        size_ = looping_ ? options.value("--size,-n", 2) : options.value< comma::uint32 >("--size,-n");
        step_ = options.value( "--step",1 );
        if( size_ < 2 ) { std::cerr <<  comma::verbose.app_name() << ": expected --size,-n= value to be greater than 1" << std::endl; return 1; }
        expected_records_ = step_ * ( size_ - 1 ) + 1;
        if( options.exists("--expected-records") ) { std::cout << expected_records_ << std::endl; return 0; };
        comma::csv::input_stream< input_t > istream(std::cin, csv);
        std::deque< std::string > deque;
        std::deque< std::string > first;
        bool has_block_ = csv.has_field( "block" );
        while( istream.ready() || ( std::cin.good() && !std::cin.eof() ) )
        {
            const input_t* p = istream.read();
            if( !p ) { break; }
            if ( !deque.empty() && has_block_ && (block_ != p->block) )
            {
                if (!verify(deque)) { return 1; }
                count_ = 0;
                if ( looping_ ) { output_loop(deque, first, csv); }
                deque.clear();  
            }
            block_ = p->block;
            deque.push_back( istream.last() );
            if ( looping_ && first.size() + 1 < expected_records_ )  { first.push_back(istream.last()); }
            ++count_;
            if( deque.size() >= step_ * ( size_ - 1 ) + 1 )
            {
                output_records(deque, csv);
                if( !use_sliding_window_ ) { deque.clear(); } else { deque.pop_front(); }
            }
        }
        if ( !verify(deque)) { return 1; }
        if ( looping_ ) { output_loop(deque, first, csv); }
        return 0;
    }

    struct input_t {
        comma::uint32 block = 0;
    };

private:
    bool is_binary_;
    bool use_sliding_window_;
    bool bidirectional_;
    bool reverse_;
    bool looping_;
    comma::uint32 size_;
    comma::uint32 count_;
    comma::uint32 block_;
    comma::uint32 step_;
    comma::uint32 expected_records_;

    // There is nothing to do in this case - binary data
    static void simple_binary_pass_through(const comma::csv::format& f, bool flush=false)
    {
        std::vector< char > buffer( f.size(), '\0' );
        while( std::cin.good() && !std::cin.eof() )
        {
            if( std::cin.read( &buffer[0], buffer.size() ) ) {
                std::cout.write( &buffer[0], buffer.size() );
                if( flush ) { std::cout.flush(); }
            }
        }
    }

    bool verify( const std::deque<std::string>& deque )
    { 
        if( use_sliding_window_ && count_ < step_ * ( size_ - 1 ) + 1 )
        { 
            std::cerr << comma::verbose.app_name() << ": --size,-n=" << size_ << ", --step=" << step_ << ", expected records count (" << step_ * ( size_ - 1 ) + 1
                      << ") is bigger than total number of input records: " << count_ << std::endl; 
            return false; 
        }
        if ( !use_sliding_window_ && !deque.empty() ) 
        { 
            std::cerr << comma::verbose.app_name() << ": error, leftover tail input record found: " << deque.size() << " lines." << std::endl; 
            return false; 
        }
        return true;
    }

    void output_records(const std::deque<std::string>& deque, const comma::csv::options& csv)
    {
        auto delimiter = std::string();
        if (!reverse_)
        {
            std::cout.write( &( deque.front()[ 0 ] ), deque.front().size() );
            auto is = deque.cbegin(); while( ( is + 1 ) != deque.cend() ) { if(!is_binary_){ std::cout << csv.delimiter; } is += step_; std::cout.write( &(*is)[0], is->size() ); }
            if(!is_binary_){ std::cout << std::endl; }
            if (csv.flush) { std::cout.flush(); }
        }
        if (bidirectional_ || reverse_ ) 
        {
            std::cout.write( &( deque.back()[0] ), deque.back().size() );
            auto is = deque.crbegin(); while( ( is + 1 ) != deque.crend() ) { if(!is_binary_){ std::cout << csv.delimiter; } is += step_; std::cout.write( &(*is)[0], is->size() ); }
            if(!is_binary_){ std::cout << std::endl; }
            if (csv.flush) { std::cout.flush(); }
        }
    }

    void output_loop( std::deque<std::string>& deque, std::deque<std::string>& first, const comma::csv::options& csv)
    {
        while ( first.size() > 0 )
        {
            deque.push_back(first.front());
            output_records(deque, csv);
            deque.pop_front();
            first.pop_front();
        }
    }
};

struct field
{
    std::string name;
    boost::optional< unsigned int > index;
    unsigned int offset;
    unsigned int size;
    field( const std::string& name ) : name( name ) {}
};

class flatten_op
{
public:
    flatten_op( const comma::command_line_options& options, const comma::csv::options& csv )
    {
        for( auto f: comma::split( options.value< std::string >( "--output-fields,--output,-o" ), ',' ))
        {
            if( !f.empty() ) { output_fields.push_back( field( f )); }
        }

        for( auto f: comma::split( options.value< std::string >( "--header-fields,--header", "" ), ',' ))
        {
            if( !f.empty() ) { header_fields.push_back( field( f )); }
        }

        std::vector< std::string > input_fields = comma::split( csv.fields, "," );
        for( unsigned int i = 0; i < input_fields.size(); ++i )
        {
            for( auto& f: output_fields )
            {
                if( f.name == input_fields[i] )
                {
                    f.index = i;
                    if( csv.binary() )
                    {
                        f.offset = csv.format().offset( i ).offset;
                        f.size = csv.format().offset( i ).size;
                    }
                }
            }
            for( auto& f: header_fields )
            {
                if( f.name == input_fields[i] )
                {
                    f.index = i;
                    if( csv.binary() )
                    {
                        f.offset = csv.format().offset( i ).offset;
                        f.size = csv.format().offset( i ).size;
                    }
                }
            }
            if( input_fields[i] == "block" )
            {
                block_field = field( input_fields[i] );
                block_field->index = i;
                if( csv.binary() )
                {
                    block_field->offset = csv.format().offset( i ).offset;
                    block_field->size = csv.format().offset( i ).size;
                }
            }
        }
    }

    int run( const comma::csv::options& csv )
    {
        if( !block_field ) { std::cerr << comma::verbose.app_name() << ": " << operation << " operation requires block field" << std::endl; return 1; }
        if( output_fields.empty() ) { std::cerr << comma::verbose.app_name() << ": " << operation << " operation requires at least one output field" << std::endl; return 1; }
        for( auto f: output_fields )
        {
            if( !f.index ) { std::cerr << comma::verbose.app_name() << ": \"" << f.name << "\" not found in input fields " << csv.fields << std::endl; return 1; }
        }
        for( auto f: header_fields )
        {
            if( !f.index ) { std::cerr << comma::verbose.app_name() << ": \"" << f.name << "\" not found in input fields " << csv.fields << std::endl; return 1; }
        }

        comma::uint32 current_block = std::numeric_limits< comma::uint32 >::max();
        if( csv.binary() )
        {
            std::vector< char > buf( csv.format().size() );
            comma::uint32* block;

            while( std::cin.good() && !std::cin.eof() )
            {
                std::cin.read( &buf[0], csv.format().size() );
                if( std::cin.gcount() == 0 ) { continue; }
                if( std::cin.gcount() < int( csv.format().size() ))
                {
                    std::cerr << comma::verbose.app_name() << ": expected " << csv.format().size()
                              << " bytes, got only " << std::cin.gcount() << std::endl;
                    return 1;
                }
                block = reinterpret_cast< comma::uint32* >( &buf[ block_field->offset ] );
                if( current_block != *block )
                {
                    for( auto f: header_fields ) { std::cout.write( &buf[ f.offset ], f.size ); }
                    current_block = *block;
                }
                for( auto f: output_fields ) { std::cout.write( &buf[ f.offset ], f.size ); }
            }
        }
        else
        {
            std::string delimiter;
            bool first_line = true;
            while( std::cin.good() && !std::cin.eof() )
            {
                std::string line;
                std::getline( std::cin, line );
                if( !line.empty() && *line.rbegin() == '\r' ) { line = line.substr( 0, line.length() - 1 ); } // windows... sigh...
                if( line.empty() ) { continue; }
                std::vector< std::string > v = comma::split( line, csv.delimiter );
                comma::uint32 block = boost::lexical_cast< comma::uint32 >( v[ *(block_field->index) ] );
                if( current_block != block )
                {
                    if( !first_line ) { std::cout << std::endl; }
                    delimiter = "";
                    for( auto f: header_fields )
                    {
                        std::cout << delimiter << v[ *(f.index) ];
                        delimiter = csv.delimiter;
                    }
                    current_block = block;
                    first_line = false;
                }
                for( auto f: output_fields )
                {
                    std::cout << delimiter << v[ *(f.index) ];
                    delimiter = csv.delimiter;
                }
            }
            std::cout << std::endl;
        }
        return 0;
    }

private:
    std::vector< field > output_fields;
    std::vector< field > header_fields;
    boost::optional< field > block_field;
};

class unflatten_op
{
public:
    unflatten_op( const comma::command_line_options& options, const comma::csv::options& csv )
    {
        for( auto f: comma::split( options.value< std::string >( "--header-fields,--header", "" ), ',' ))
        {
            if( !f.empty() ) { header_fields.push_back( field( f )); }
        }
        for( auto f: comma::split( options.value< std::string >( "--recurring-fields,--recurring", "" ), ',' ))
        {
            if( !f.empty() ) { recurring_fields.push_back( field( f )); }
        }
        std::vector< field > all_fields;

        for( unsigned int i = 0; i < header_fields.size(); ++i )
        {
            header_fields[i].index = i;
            if( csv.binary() )
            {
                header_fields[i].offset = csv.format().offset( i ).offset;
                header_fields[i].size = csv.format().offset( i ).size;
            }
        }
        for( unsigned int i = 0; i < recurring_fields.size(); ++i )
        {
            unsigned int index = header_fields.size() + i;
            recurring_fields[i].index = index;
            if( csv.binary() )
            {
                recurring_fields[i].offset = csv.format().offset( index ).offset;
                recurring_fields[i].size = csv.format().offset( index ).size;
            }
        }
    }

    int run( const comma::csv::options& csv )
    {
        if( csv.binary() )
        {
            std::vector< char > buf( csv.format().size() );
            while( std::cin.good() && !std::cin.eof() )
            {
                std::cin.read( &buf[0], csv.format().size() );
                if( std::cin.gcount() == 0 ) { continue; }
                if( std::cin.gcount() < int( csv.format().size() ))
                {
                    std::cerr << comma::verbose.app_name() << ": expected " << csv.format().size()
                              << " bytes, got only " << std::cin.gcount() << std::endl;
                    return 1;
                }

                std::size_t header_size = 0;
                std::size_t recurring_size = 0;
                for( auto f: header_fields ) { header_size += f.size; }
                for( auto f: recurring_fields ) { recurring_size += f.size; }

                for( std::size_t offset = recurring_fields[0].offset;
                     offset < csv.format().size(); offset += recurring_size )
                {
                    std::cout.write( &buf[ 0 ], header_size );
                    std::cout.write( &buf[ offset ], recurring_size );
                }
            }
        }
        else
        {
            while( std::cin.good() && !std::cin.eof() )
            {
                std::string line;
                std::getline( std::cin, line );
                if( !line.empty() && *line.rbegin() == '\r' ) { line = line.substr( 0, line.length() - 1 ); } // windows... sigh...
                if( line.empty() ) { continue; }
                std::vector< std::string > v = comma::split( line, csv.delimiter );

                if(( v.size() - header_fields.size() ) % recurring_fields.size() != 0 )
                {
                    std::cerr << comma::verbose.app_name() << ": input record has " << v.size() << " fields which doesn't match header and recurring parameters" << std::endl;
                    return 1;
                }

                for( unsigned int i = header_fields.size(); i < v.size(); i += recurring_fields.size() )
                {
                    std::string delimiter;
                    for( unsigned int j = 0; j < header_fields.size(); ++j )
                    {
                        std::cout << delimiter << v[j];
                        delimiter = csv.delimiter;
                    }
                    for( unsigned int j = 0; j < recurring_fields.size(); ++j )
                    {
                        std::cout << delimiter << v[i+j];
                        delimiter = csv.delimiter;
                    }
                    std::cout << std::endl;
                }
            }
        }
        return 0;
    }

private:
    std::vector< field > header_fields;
    std::vector< field > recurring_fields;
};

namespace comma { namespace visiting {

template <> struct traits< concatenate_op::input_t >
{
    template < typename K, typename V > static void visit( const K&, const concatenate_op::input_t& p, V& v ) { v.apply("block", p.block); }
    template < typename K, typename V > static void visit( const K&, concatenate_op::input_t& p, V& v ) { v.apply("block", p.block); }
};

} } // namespace comma { namespace visiting {

int main( int ac, char** av )
{
    try
    {
        comma::command_line_options options( ac, av, usage );
        if( options.exists( "--bash-completion" ) ) bash_completion( ac, av );
        std::vector< std::string > unnamed = options.unnamed( "--size,-n,--sliding-window,-w,--step,--verbose,-v", "-.*" );
        comma::csv::options csv( options );
        if( unnamed.empty() ) { std::cerr << comma::verbose.app_name() << ": please specify operation" << std::endl; exit( 1 ); }
        operation = unnamed[0];
        if( operation == "concatenate" || operation == "loop" )
        {
            if (csv.fields.empty()) { csv.fields="a"; }
            return concatenate_op().run( options, csv );
        }
        else if( operation == "flatten" ) { return flatten_op( options, csv ).run( csv ); }
        else if( operation == "unflatten" ) { return unflatten_op( options, csv ).run( csv ); }
        std::cerr << comma::verbose.app_name() << ": operation not supported or unknown: '" << operation << '\'' << std::endl;
        return 1;
    }
    catch( std::exception& ex ) { std::cerr << "csv-shape: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-shape: unknown exception" << std::endl; }
    return 1;
}
