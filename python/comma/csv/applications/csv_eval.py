#!/usr/bin/python

import sys
import argparse
import numpy
import comma.csv
from signal import signal, SIGPIPE, SIG_DFL
import re

class stream:
  def __init__( self, args ):
    self.args = args
    self.csv_options = dict( full_xpath=False, flush=self.args.flush, delimiter=self.args.delimiter, precision=self.args.precision )
    self.initialize_input()
    self.initialize_output()

  def initialize_input( self ):
    if not self.args.fields or not self.args.fields.translate( None, ',' ).strip(): raise Exception( "specify input stream fields, e.g. --fields=x,y" )
    self.nonblank_input_fields = filter( None, self.args.fields.split(',') )
    if self.args.binary:
      given_fields = self.args.fields.split(',')
      types = comma.csv.format.to_numpy( self.args.binary )
      omitted_fields = [''] * ( len( types ) - len( given_fields ) )
      fields = [ "__blank{}".format( index ) if not name else name for index,name in enumerate( given_fields + omitted_fields ) ]
      input_t = comma.csv.struct( ','.join( fields ), *types )
      self.input = comma.csv.stream( input_t, binary=True, **self.csv_options )
    else:
      input_t = comma.csv.struct( ','.join( self.nonblank_input_fields ), *('float64',)*len( self.nonblank_input_fields ) )
      self.input = comma.csv.stream( input_t, fields=self.args.fields, **self.csv_options )

  def initialize_output( self ):
    inferred_fields = ','.join( e.split('=',1)[0].strip() for e in self.args.expressions.split(';') )
    fields = self.args.append_fields if self.args.append_fields else inferred_fields
    format = self.args.append_binary if self.args.binary and self.args.append_binary else ','.join( ('d',)*len( fields.split(',') ) )
    if self.args.verbose:
      print >> sys.stderr, "append fields: '{}'".format( fields )
      print >> sys.stderr, "append format: '{}'".format( format )
      print >> sys.stderr, "numpy format: '{}'".format( ','.join( comma.csv.format.to_numpy( format ) ) )
    try: check_fields( fields.split(','), input_fields=self.args.fields.split(',') )
    except Exception, message: raise Exception( "attached fields error: " + str( message ) )
    output_t = comma.csv.struct( fields, *comma.csv.format.to_numpy( format ) )
    if self.args.binary:
      self.output = comma.csv.stream( output_t, tied=self.input, binary=True, **self.csv_options )
    else:
      self.output = comma.csv.stream( output_t, tied=self.input, **self.csv_options )

def get_dict( module, update={}, delete=[] ):
  d = module.__dict__.copy()
  d.update( update )
  for k in set( delete ).intersection( d.keys() ): del d[k]
  return d

def check_fields( fields, input_fields=(), env=get_dict( numpy ) ):
  for name in fields:
    if not re.match( r'^[a-z_]\w*$', name, re.I ): raise Exception( "'{}' is not a valid field name".format( name ) )
    if name == '__input' or name == '__output' or name in env: raise Exception( "'{}' is a reserved name".format( name ) )
    if name in input_fields: raise Exception( "'{}' is an input field name".format( name ) )

add_semicolon = lambda _: _ + ( ';' if _ and _[-1] != ';' else '' )

def evaluate( expressions, stream, dangerous=False ):
  initialize_input = ';'.join( "{name} = __input['{name}']".format( name=name ) for name in stream.nonblank_input_fields )
  initialize_output = ';'.join( "__output['{name}'] = {name}".format( name=name ) for name in stream.output.struct.fields )
  code = compile( add_semicolon( initialize_input ) + add_semicolon( expressions ) + initialize_output, '<string>', 'exec' )
  restricted_numpy = get_dict( numpy ) if dangerous else get_dict( numpy, update=dict(__builtins__={}), delete=['sys'] )
  output = numpy.empty( stream.input.size, dtype=stream.output.struct )
  for i in stream.input.iter():
    if output.size != i.size: output = numpy.empty( i.size, dtype=stream.output.struct )
    exec code in restricted_numpy, { '__input': i, '__output': output }
    stream.output.write( output )

def add_csv_options( parser ):
  parser.add_argument( "--fields", "-f", default='x,y,z', help="comma-separated field names of input stream (default: %(default)s)", metavar='<names>' )
  parser.add_argument( "--binary", "-b", default='', help="assume input stream is binary and use binary format (by default, stream is ascii)", metavar='<format>' )
  parser.add_argument( "--delimiter", "-d", default=',', help="csv delimiter of ascii stream (default: %(default)s)", metavar='<delimiter>' )
  parser.add_argument( "--precision", default=12, help="floating point precision of ascii output (default: %(default)s)", metavar='<precision>' )
  parser.add_argument( "--flush", action="store_true", help="flush output stream" )
  parser.add_argument( "--append-fields", "-F", help="output fields appended to input stream (by default, inferred from expressions)", metavar='<names>' )
  parser.add_argument( "--append-binary", "-B", help="for binary stream, binary format of appended fields (by default, 'd' for each)", metavar='<format>' )

def main():
  description="""
evaluate numerical expressions and append computed values to csv stream
"""
  epilog="""
notes:
    1) in ascii mode, specified input fields are treated as floating point numbers
    2) output fields appended to input stream are inferred from expressions (by default) or specified by --append-fields
    3) if --append-fields is omitted, only simple assignment statements are allowed in expressions
    4) in binary mode, appended fields are assigned type 'd' (by default) or format is specified by --append-binary

examples:
    ( echo 1,2; echo 3,4 ) | {script_name} --fields=x,y --precision=2 'a=2/(x+y);b=x-sin(y)*a**2'
    ( echo 1,2; echo 3,4 ) | csv-to-bin 2d | {script_name} --binary=2d --fields=x,y 'a=2/(x+y);b=x-sin(y)*a**2' -v | csv-from-bin 4d

    # define intermediate variable
    ( echo 1; echo 2 ) | csv-to-bin d | {script_name} --binary=d --fields=x 'a=2;y=a*x' --append-fields=y -v | csv-from-bin 2d

    # take minimum
    ( echo 1,2; echo 4,3 ) | csv-to-bin 2d | {script_name} --binary=2d --fields=x,y 'c=minimum(x,y)' -v | csv-from-bin 3d

    # clip index
    ( echo a,2; echo b,5 ) | csv-to-bin s[1],ui | {script_name} --binary=s[1],ui --fields=,id 'i=clip(id,3,inf)' --append-binary=ui -v | csv-from-bin s[1],ui,ui

    # compare fields
    ( echo 1,2; echo 4,3 ) | csv-to-bin 2i | {script_name} --binary=2i --fields=i,j 'flag=i+1==j' --append-binary=b -v | csv-from-bin 2i,b
    ( echo 1,2; echo 4,3 ) | csv-to-bin 2d | {script_name} --binary=2d --fields=x,y 'flag=x<y' --append-binary=b -v | csv-from-bin 2d,b
    ( echo 0,1; echo 1,2; echo 4,3 ) | csv-to-bin 2d | {script_name} --binary=2d --fields=x,y 'flag=logical_and(x<y,y<2)' --append-binary=b -v | csv-from-bin 2d,b

    # negate boolean
    ( echo 0; echo 1 ) | csv-to-bin b | {script_name} --binary=b --fields=flag 'a=logical_not(flag)' --append-binary=b -v | csv-from-bin 2b
    
""".format( script_name=sys.argv[0].split('/')[-1] )
  parser = argparse.ArgumentParser( description=description, epilog=epilog, formatter_class=lambda prog: argparse.RawTextHelpFormatter( prog, max_help_position=50 ) )  
  parser.add_argument( "expressions", help="semicolon-separated numerical expressions to evaluate (see examples)" )
  parser.add_argument( "--verbose", "-v", action="store_true", help="increase output verbosity" )
  parser.add_argument( "--dangerous", action="store_true", help=argparse.SUPPRESS )
  add_csv_options( parser )
  args = parser.parse_args()
  signal( SIGPIPE, SIG_DFL )
  evaluate( args.expressions.strip(';'), stream( args ), dangerous=args.dangerous )

if __name__ == '__main__':
  main()
