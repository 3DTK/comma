import unittest
import numpy as np
from comma.numpy import *
import comma.numpy

class test_merge_arrays(unittest.TestCase):
    def test_mismatched_size(self):
        dtype_a = np.dtype([('name', 'u4')])
        dtype_b = np.dtype([('name', 'f8')])
        a = np.array([(1,), (2,)], dtype=dtype_a)
        b = np.array([(4,), (5,), (6,)], dtype=dtype_b)
        self.assertRaises(ValueError, merge_arrays, a, b)

    def test_merge_empty(self):
        dtype_a = np.dtype([('name', 'u4')])
        dtype_b = np.dtype([('name', 'f8')])
        a = np.array([], dtype=dtype_a)
        b = np.array([], dtype=dtype_b)
        expected = np.array([], dtype=[('first', dtype_a), ('second', dtype_b)])
        c = merge_arrays(a, b)
        np.testing.assert_equal(c, expected)

    def test_both_single_column(self):
        dtype_a = np.dtype([('name', 'u4')])
        dtype_b = np.dtype([('name', 'f8')])
        a = np.array([(1,), (2,), (3,)], dtype=dtype_a)
        b = np.array([(4,), (5,), (6,)], dtype=dtype_b)
        expected = np.array([((1,), (4,)), ((2,), (5,)), ((3,), (6,))], dtype=[('first', dtype_a), ('second', dtype_b)])
        c = merge_arrays(a, b)
        np.testing.assert_equal(c, expected)

    def test_first_single_column(self):
        dtype_a = [('name', 'u4')]
        dtype_b = [('name_1', 'f8'), ('name_2', 'S1')]
        a = np.array([(1,), (2,), (3,)], dtype=dtype_a)
        b = np.array([(4, 'a'), (5, 'b'), (6, 'c')], dtype=dtype_b)
        expected = np.array([((1,), (4, 'a')), ((2,), (5, 'b')), ((3,), (6, 'c'))], dtype=[('first', dtype_a), ('second', dtype_b)])
        c = merge_arrays(a, b)
        np.testing.assert_equal(c, expected)

    def test_second_single_column(self):
        dtype_a = [('name', 'u4'), ('name_2', 'S1')]
        dtype_b = [('name_1', 'f8')]
        a = np.array([(1, 'a'), (2, 'b'), (3, 'c')], dtype=dtype_a)
        b = np.array([(4,), (5,), (6,)], dtype=dtype_b)
        expected = np.array([((1, 'a'), (4,)), ((2, 'b'), (5,)), ((3, 'c'), (6,))], dtype=[('first', dtype_a), ('second', dtype_b)])
        c = merge_arrays(a, b)
        np.testing.assert_equal(c, expected)

    def test_both_multicolumn(self):
        dtype_a = [('name', 'u4'), ('name_2', 'S1')]
        dtype_b = [('name_1', 'f8'), ('name_2', 'S1')]
        a = np.array([(1, 'a'), (2, 'b'), (3, 'c')], dtype=dtype_a)
        b = np.array([(4, 'x'), (5, 'y'), (6, 'z')], dtype=dtype_b)
        expected = np.array([((1, 'a'), (4, 'x')), ((2, 'b'), (5, 'y')), ((3, 'c'), (6, 'z'))], dtype=[('first', dtype_a), ('second', dtype_b)])
        c = merge_arrays(a, b)
        np.testing.assert_equal(c, expected)


class test_unrolled_types_of_flat_dtype(unittest.TestCase):
    def test_scalar_from_type(self):
        dtype = np.dtype(np.uint32)
        expected = ('u4',)
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_scalar_from_string(self):
        dtype = np.dtype('u4')
        expected = ('u4',)
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_scalar_array_2d(self):
        dtype = np.dtype('(2,3)u4')
        expected = ('u4', 'u4', 'u4', 'u4', 'u4', 'u4')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_array_1d(self):
        dtype = np.dtype([('name', '3u4')])
        expected = ('u4', 'u4', 'u4')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_array_1d_long(self):
        dtype = np.dtype([('name', '12u4')])
        expected = ('u4',) * 12
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_array_2d(self):
        dtype = np.dtype([('name', '(2,3)u4')])
        expected = ('u4', 'u4', 'u4', 'u4', 'u4', 'u4')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_array_2d_with_other_fields(self):
        dtype = np.dtype([('name1', 'S2'), ('name2', '(2,3)u4'), ('name3', 'f8')])
        expected = ('S2', 'u4', 'u4', 'u4', 'u4', 'u4', 'u4', 'f8')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_no_array(self):
        dtype = np.dtype([('name1', 'S2'), ('name2', 'f8')])
        expected = ('S2', 'f8')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_time(self):
        dtype = np.dtype([('name1', 'datetime64[us]'), ('name2', 'timedelta64[us]')])
        expected = ('M8[us]', 'm8[us]')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)

    def test_time_array(self):
        dtype = np.dtype([('name', '2datetime64[us]')])
        expected = ('M8[us]', 'M8[us]')
        self.assertTupleEqual(types_of_dtype(dtype, unroll=True), expected)


class test_types_of_dtype(unittest.TestCase):
    def test_scalar_from_type(self):
        dtype = np.dtype(np.uint32)
        expected = ('u4',)
        self.assertTupleEqual(types_of_dtype(dtype), expected)

    def test_scalar_from_string(self):
        dtype = np.dtype('u4')
        expected = ('u4',)
        self.assertTupleEqual(types_of_dtype(dtype), expected)

    def test_scalar_array_2d(self):
        dtype = np.dtype('(2,3)u4')
        expected = ('(2,3)u4',)
        self.assertTupleEqual(types_of_dtype(dtype), expected)

    def test_array_2d_with_other_fields(self):
        dtype = np.dtype([('name1', 'S2'), ('name2', '(2,3)u4'), ('name3', 'f8')])
        expected = ('S2', '(2,3)u4', 'f8')
        self.assertTupleEqual(types_of_dtype(dtype), expected)

    def test_no_array(self):
        dtype = np.dtype([('name1', 'S2'), ('name2', 'f8')])
        expected = ('S2', 'f8')
        self.assertTupleEqual(types_of_dtype(dtype), expected)

# ATTENTION!!!
# Nice idea but does not work. See https://github.com/numpy/numpy/issues/6359
# for a discussion of descr purpuses and usages (TL;DR: obscure, undocumented,
# intended for other purposes). See a unit test from 9e14edfa644d062 showing
# problems with the current implementation.
class test_structured_dtype(unittest.TestCase):
    def test_multiple_types(self):
        expected = np.dtype('S2,u4,f8')
        self.assertEqual(structured_dtype('S2,u4,f8'), expected)

    def test_single_type_string(self):
        expected = np.dtype([('f0', 'S2')])
        self.assertEqual(structured_dtype('S2'), expected)

    def test_single_type_time(self):
        expected = np.dtype([('f0', 'datetime64[us]')])
        self.assertEqual(structured_dtype('datetime64[us]'), expected)

    def test_structure_out_of_order(self):
        try:
            names1 = ['word', 'a3', 'byte', 'a2' ]
            formats1 = [np.dtype('uint16'), np.dtype(('<f8', (3,))), np.dtype('uint8'), np.dtype(('<f8', (2,))) ]
            offsets1 = [0, 2, 26, 27 ]
            itemsize = 43

            ndtype1 = np.dtype( dict( names=names1, formats=formats1, offsets=offsets1, itemsize=itemsize ) )
            sorted_fields1 = sorted( list( ndtype1.fields.items() ), key = lambda t: t[1] )

            names2 = ['a3', 'word', 'a2', 'byte' ]
            formats2 = [np.dtype(('<f8', (3,))), np.dtype('uint16'), np.dtype(('<f8', (2,))), np.dtype('uint8') ]
            offsets2 = [2, 0, 27, 26 ]

            ndtype2 = np.dtype( dict( names=names2, formats=formats2, offsets=offsets2, itemsize=itemsize ) )
            sorted_fields2 = sorted( list( ndtype2.fields.items() ), key = lambda t: t[1] )

            self.assertEqual( sorted_fields1, sorted_fields2 )
            self.assertEqual( len( ndtype1.descr ), 4 )
            self.assertEqual( len( ndtype2.descr ), 5 )  # shall be 4
            self.assertEqual( len( functions.types_of_dtype( ndtype1 ) ), 4 )
            self.assertEqual( len( functions.types_of_dtype( ndtype2 ) ), 5 )  # shall be 4
        except ValueError:
            import sys
            print(file=sys.stderr)
            print("ATTENTION: test_structure_out_of_order failed due to the version of numpy on this computer", file=sys.stderr)
            print("           your applications using comma.csv will mostly work; sometimes they will fail", file=sys.stderr)
            print("           early (meaning you will know straight away) until types_of_dtype is rewritten", file=sys.stderr)
            print("           See todo comment in python/comma/numpy/functions.py", file=sys.stderr)
            print(file=sys.stderr)
            for s in sys.exc_info(): print("           " + str( s ), file=sys.stderr)
            print(file=sys.stderr)

        if False:
            import sys
            self.assertEqual( sorted( ndtype1.descr ), sorted( ndtype2.descr ) )
            self.assertEqual( sorted( functions.types_of_dtype( ndtype1 ) ), sorted( functions.types_of_dtype( ndtype2 ) ) )

            print("observe the differences:", file=sys.stderr)

            print("ndtype1: ", ndtype1, file=sys.stderr)
            print("ndtype2: ", ndtype2, file=sys.stderr)

            print("ndtype1.fields: ", ndtype1.fields, file=sys.stderr)
            print("ndtype2.fields: ", ndtype2.fields, file=sys.stderr)
            print("fields identical: ", sorted_fields1 == sorted_fields2, file=sys.stderr)

            print("ndtype1.descr: ", ndtype1.descr, file=sys.stderr)
            print("ndtype2.descr: ", ndtype2.descr, file=sys.stderr)

            print("types_of_dtype( ndtype1 ): ", comma.numpy.functions.types_of_dtype( ndtype1 ), file=sys.stderr)
            print("types_of_dtype( ndtype2 ): ", comma.numpy.functions.types_of_dtype( ndtype2 ), file=sys.stderr)


if __name__ == '__main__':
    unittest.main()
