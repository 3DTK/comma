import unittest
import numpy as np
import sys
import comma


class test_stream(unittest.TestCase):
    def test_use_defaults(self):
        s = comma.csv.stream(comma.csv.struct('x', 'f8'))
        self.assertEqual(s.delimiter, ',')
        self.assertFalse(s.flush)
        self.assertEqual(s.precision, 12)
        self.assertEqual(s.source, sys.stdin)
        self.assertEqual(s.target, sys.stdout)
        self.assertIsNone(s.tied)
        self.assertTrue(s.full_xpath)

    def test_override_defaults(self):
        from io import StringIO
        source = StringIO("")
        target = StringIO("")
        t = comma.csv.stream(comma.csv.struct('id', 'S4'), delimiter=';')
        s = comma.csv.stream(comma.csv.struct('x', 'f8'), delimiter=';', flush=True, precision=4, source=source, target=target, tied=t, full_xpath=False)
        self.assertEqual(s.delimiter, ';')
        self.assertTrue(s.flush)
        self.assertEqual(s.precision, 4)
        self.assertEqual(s.source, source)
        self.assertEqual(s.target, target)
        self.assertEqual(s.tied, t)
        self.assertFalse(s.full_xpath)

    def test_fields_from_struct(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t)
        self.assertTupleEqual(s.fields, ('id', 'event/t', 'event/point/x', 'event/point/y', 'event/point/z'))

    def test_fields_simple(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields='id,event/t,event/point/x,event/point/y,event/point/z')
        self.assertTupleEqual(s.fields, ('id', 'event/t', 'event/point/x', 'event/point/y', 'event/point/z'))

    def test_fields_mixedup(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields='event/point/z,id,event/point/x,event/t,event/point/y')
        self.assertTupleEqual(s.fields, ('event/point/z', 'id', 'event/point/x', 'event/t', 'event/point/y'))

    def test_fields_missing(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields='event/point/z,event/point/y')
        self.assertTupleEqual(s.fields, ('event/point/z', 'event/point/y'))

    def test_fields_blanks(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields=',,event/point/z,,event/point/y,')
        self.assertTupleEqual(s.fields, ('', '', 'event/point/z', '', 'event/point/y', ''))

    def test_fields_unknown_fields(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields='u1,u2,event/point/z,,event/point/y,u3')
        self.assertTupleEqual(s.fields, ('u1', 'u2', 'event/point/z', '', 'event/point/y', 'u3'))

    def test_fields_leaves(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields=',z,id,,t,y', full_xpath=False)
        self.assertTupleEqual(s.fields, ('', 'event/point/z', 'id', '', 'event/t', 'event/point/y'))

    def test_fields_shorthand(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        s = comma.csv.stream(record_t, fields=',event/point,,,id')
        self.assertTupleEqual(s.fields, ('', 'event/point/x', 'event/point/y', 'event/point/z', '', '', 'id'))

    def test_fields_invalid_fields(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('t,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,event', 'u4', event_t)
        self.assertRaises(ValueError, comma.csv.stream, record_t, fields='event/point', full_xpath=False)

    def test_fields_ambiguous_leaves(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('id,point', 'datetime64[us]', point_t)
        record_t = comma.csv.struct('id,x,event', 'u4', 'S4', event_t)
        self.assertRaises(ValueError, comma.csv.stream, record_t, fields='x', full_xpath=False)

    def test_fields_duplicates(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        self.assertRaises(ValueError, comma.csv.stream, point_t, fields='x,y,z,x')

    def test_tied_is_not_stream(self):
        t = comma.csv.struct('x', 'f8')
        self.assertRaises(TypeError, comma.csv.stream, t, tied='not_a_stream')

    def test_mismatched_tied(self):
        t_binary = comma.csv.stream(comma.csv.struct('i', 'u1'), binary=True)
        t_ascii = comma.csv.stream(comma.csv.struct('i', 'u1'))
        s = comma.csv.struct('x', 'f8')
        self.assertRaises(ValueError, comma.csv.stream, s, tied=t_binary)
        self.assertRaises(ValueError, comma.csv.stream, s, binary=True, tied=t_ascii)

    def test_binary(self):
        point_t = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        t1 = comma.csv.stream(point_t, fields=',x')
        self.assertFalse(t1.binary)
        t2 = comma.csv.stream(point_t, fields=',x', format='u1,f8', binary=False)
        self.assertFalse(t2.binary)
        t3 = comma.csv.stream(point_t, fields='x', binary=False)
        self.assertFalse(t3.binary)

        t4 = comma.csv.stream(point_t, fields=',x', format='u1,f8')
        self.assertTrue(t4.binary)
        t5 = comma.csv.stream(point_t, fields='x', binary=True)
        self.assertTrue(t5.binary)
        t6 = comma.csv.stream(point_t, fields=',x', format='u1,f8', binary=True)
        self.assertTrue(t6.binary)

    def test_size(self):
        self.assertEqual(comma.csv.stream.buffer_size_in_bytes, 65536)
        s = comma.csv.struct('x,id', 'f8', 'u4')
        tied = comma.csv.stream(comma.csv.struct('i', 'u2'))
        t1 = comma.csv.stream(s)
        self.assertEqual(t1.size, comma.csv.stream.buffer_size_in_bytes / 12)
        t2 = comma.csv.stream(s, flush=True)
        self.assertEqual(t2.size, 1)
        t3 = comma.csv.stream(s, tied=tied)
        self.assertEqual(t3.size, comma.csv.stream.buffer_size_in_bytes / 2)

    def test_ascii_simple_single_field(self):
        s = comma.csv.struct('x', 'f8')
        t = comma.csv.stream(s)
        self.assertEqual(t.input_dtype, np.dtype([('f0', 'f8')]))
        self.assertDictEqual(t.ascii_converters, {})
        self.assertTupleEqual(t.usecols, (0,))
        self.assertEqual(t.filling_values, None)
        self.assertTupleEqual(t.missing_fields, ())
        self.assertEqual(t.complete_fields, t.fields)
        self.assertEqual(t.complete_dtype, t.input_dtype)
        self.assertIsNone(t.missing_dtype)
        self.assertFalse(t.data_extraction_fields)

    def test_ascii_simple_multiple_fields(self):
        s = comma.csv.struct('x,y,z', 'f8', 'f8', 'f8')
        t = comma.csv.stream(s)
        self.assertEqual(t.input_dtype, np.dtype([('f0', 'f8'), ('f1', 'f8'), ('f2', 'f8')]))
        self.assertDictEqual(t.ascii_converters, {})
        self.assertTupleEqual(t.usecols, (0, 1, 2))
        self.assertTupleEqual(t.filling_values, ('', '', ''))
        self.assertTupleEqual(t.missing_fields, ())
        self.assertEqual(t.complete_fields, t.fields)
        self.assertEqual(t.complete_dtype, t.input_dtype)
        self.assertIsNone(t.missing_dtype)
        self.assertFalse(t.data_extraction_fields)

    def test_ascii_general_fields(self):
        s = comma.csv.struct('t,x,y,z', 'datetime64[us]', 'f8', 'f8', 'f8')
        t = comma.csv.stream(s, fields='n1,n2,t,,y,')
        # f0,f1,... are standard numpy field names, which are used when field names are not specified
        self.assertEqual(t.input_dtype, np.dtype([('f0', 'S'), ('f1', 'S'), ('f2', 'M8[us]'), ('f3', 'S'), ('f4', 'f8'), ('f5', 'S')]))
        self.assertDictEqual(t.ascii_converters, {2: comma.csv.time.to_numpy })
        self.assertTupleEqual(t.usecols, (0, 1, 2, 3, 4, 5))
        self.assertTupleEqual(t.filling_values, ('', '', '', '', '', ''))
        self.assertTupleEqual(t.missing_fields, ('x', 'z'))
        self.assertTupleEqual(t.complete_fields, ('n1', 'n2', 't', '', 'y', '', 'x', 'z'))
        self.assertEqual(t.missing_dtype, np.dtype([('f6', 'f8'), ('f7', 'f8')]))
        self.assertEqual(t.complete_dtype, np.dtype([('f0', 'S'), ('f1', 'S'), ('f2', 'M8[us]'), ('f3', 'S'), ('f4', 'f8'), ('f5', 'S'), ('f6', 'f8'), ('f7', 'f8')]))
        self.assertTupleEqual(t.data_extraction_fields, ('f2', 'f6', 'f4', 'f7'))

    def test_ascii_structured(self):
        point_t = comma.csv.struct('t,x,y,z', 'datetime64[us]', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('dt,point', 'timedelta64[us]', point_t)
        record_t = comma.csv.struct('id,name,event', 'u4', 'S36', event_t)
        t = comma.csv.stream(record_t, fields=',name,,,id,event/point/x,event/point/t,')
        self.assertEqual(t.input_dtype, np.dtype([('f0', 'S'), ('f1', 'S36'), ('f2', 'S'), ('f3', 'S'), ('f4', 'u4'), ('f5', 'f8'), ('f6', 'M8[us]'), ('f7', 'S')]))
        self.assertDictEqual(t.ascii_converters, {6: comma.csv.time.to_numpy })
        self.assertTupleEqual(t.usecols, (0, 1, 2, 3, 4, 5, 6, 7))
        self.assertTupleEqual(t.filling_values, ('', '', '', '', '', '', '', ''))
        self.assertTupleEqual(t.missing_fields, ('event/dt', 'event/point/y', 'event/point/z'))
        self.assertTupleEqual(t.complete_fields, ('', 'name', '', '', 'id', 'event/point/x', 'event/point/t', '', 'event/dt', 'event/point/y', 'event/point/z'))
        self.assertEqual(t.missing_dtype, np.dtype([('f8', 'm8[us]'), ('f9', 'f8'), ('f10', 'f8')]))
        self.assertEqual(t.complete_dtype, np.dtype([('f0', 'S'), ('f1', 'S36'), ('f2', 'S'), ('f3', 'S'), ('f4', 'u4'), ('f5', 'f8'), ('f6', 'M8[us]'), ('f7', 'S'), ('f8', 'm8[us]'), ('f9', 'f8'), ('f10', 'f8')]))
        self.assertEqual(t.data_extraction_fields, ('f4', 'f1', 'f8', 'f6', 'f5', 'f9', 'f10'))

    def test_binary_structured(self):
        point_t = comma.csv.struct('t,x,y,z', 'datetime64[us]', 'f8', 'f8', 'f8')
        event_t = comma.csv.struct('dt,point', 'timedelta64[us]', point_t)
        record_t = comma.csv.struct('id,name,event', 'u4', 'S36', event_t)
        t = comma.csv.stream(record_t, fields=',name,,,id,event/point/x,event/point/t,', format='i1,S36,i2,i2,u4,f8,datetime64[us],S10')
        self.assertEqual(t.input_dtype, np.dtype([('f0', 'i1'), ('f1', 'S36'), ('f2', 'i2'), ('f3', 'i2'), ('f4', 'u4'), ('f5', 'f8'), ('f6', 'M8[us]'), ('f7', 'S10')]))
        self.assertRaises(AttributeError, getattr, t, 'ascii_converters')
        self.assertRaises(AttributeError, getattr, t, 'usecols')
        self.assertRaises(AttributeError, getattr, t, 'filling_values')
        self.assertTupleEqual(t.missing_fields, ('event/dt', 'event/point/y', 'event/point/z'))
        self.assertTupleEqual(t.complete_fields, ('', 'name', '', '', 'id', 'event/point/x', 'event/point/t', '', 'event/dt', 'event/point/y', 'event/point/z'))
        self.assertEqual(t.missing_dtype, np.dtype([('f8', 'm8[us]'), ('f9', 'f8'), ('f10', 'f8')]))
        self.assertEqual(t.complete_dtype, np.dtype([('f0', 'i1'), ('f1', 'S36'), ('f2', 'i2'), ('f3', 'i2'), ('f4', 'u4'), ('f5', 'f8'), ('f6', 'M8[us]'), ('f7', 'S10'), ('f8', 'm8[us]'), ('f9', 'f8'), ('f10', 'f8')]))
        self.assertEqual(t.data_extraction_fields, ('f4', 'f1', 'f8', 'f6', 'f5', 'f9', 'f10'))

    def test_numpy_scalar_to_string(self):
        stream = comma.csv.stream(comma.csv.struct('i', 'S1'))
        stream_with_precision = comma.csv.stream(comma.csv.struct('i', 'S1'), precision=3)
        def f(value): return stream.numpy_scalar_to_string(np.float64(value))
        self.assertEqual(f(1.6789), '1.6789')
        self.assertEqual(f(0.12345678901234567890), '0.123456789012')
        self.assertEqual(f(-1.2345), '-1.2345')
        self.assertEqual(f(0.0), '0')
        self.assertEqual(f(10), '10')
        self.assertEqual(f(-12345678), '-12345678')
        def p(value): return stream_with_precision.numpy_scalar_to_string(np.float64(value))
        self.assertEqual(p(1.6789), '1.68')
        self.assertEqual(p(-1.2345), '-1.23')
        self.assertEqual(p(0.0), '0')
        self.assertEqual(p(10), '10')
        self.assertEqual(p(-1000), '-1e+03')
        def i(value): return stream.numpy_scalar_to_string(np.int64(value))
        self.assertEqual(i(0), '0')
        self.assertEqual(i(-123456789), '-123456789')
        def s(value): return stream.numpy_scalar_to_string(np.str_(value))
        self.assertEqual(s('test'), 'test')
        def t(value): return stream.numpy_scalar_to_string(np.datetime64(value, 'us'))
        self.assertEqual(t('2015-01-02 01:02:03.123456'), '20150102T010203.123456')
        self.assertEqual(t('2015-01-02 01:02:03'), '20150102T010203')
        def c(value): return stream.numpy_scalar_to_string(np.complex128(value))
        self.assertRaises(NotImplementedError, c, 1 + 2j)


if __name__ == '__main__':
    unittest.main()
