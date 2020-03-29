# This file is part of comma, a generic and flexible library
# Copyright (c) 2011 The University of Sydney
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University of Sydney nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
# GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
# HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import numpy as np
import re
import os
import time

UNIT = 'us'
TYPE = 'M8[' + UNIT + ']'
DTYPE = np.dtype(TYPE)

NOT_A_DATE_TIME = np.datetime64('NaT')
POSITIVE_INFINITY = np.datetime64('294247-01-09T04:00:54.775807')
NEGATIVE_INFINITY = np.datetime64('-290308-12-22T19:59:05.224191')

def is_undefined(numpy_time): return str(numpy_time) == str(NOT_A_DATE_TIME)

def is_positive_infinity(numpy_time): return numpy_time == POSITIVE_INFINITY

def is_negative_infinity(numpy_time): return numpy_time == NEGATIVE_INFINITY

def to_numpy(t):
    """
    return numpy datetime64 scalar corresponding to the given comma time string

    >>> import numpy as np
    >>> from comma.csv.time import to_numpy
    >>> to_numpy('20150102T123456') == np.datetime64('2015-01-02T12:34:56.000000', 'us')
    True
    >>> to_numpy('20150102T123456.010203') == np.datetime64('2015-01-02T12:34:56.010203', 'us')
    True
    >>> to_numpy('not-a-date-time')
    numpy.datetime64('NaT')
    >>> to_numpy('')
    numpy.datetime64('NaT')
    """
    if t in ['', 'not-a-date-time']: return NOT_A_DATE_TIME
    if t in ['+infinity', '+inf', 'infinity', 'inf']: return POSITIVE_INFINITY
    if t in ['-infinity', '-inf']: return NEGATIVE_INFINITY
    if not (isinstance(t, str) and re.match(r'^(\d{8}T\d{6}(\.\d{0,6})?)$', t)):
        msg = "expected comma time, got '{}'".format(repr(t))
        raise TypeError(msg)
    v = list(t)
    for i in [13, 11]: v.insert(i, ':')
    for i in [6, 4]: v.insert(i, '-')
    return np.datetime64(''.join(v), UNIT)

def from_numpy(t):
    """
    return comma time string if datetime64 scalar is given or
    return time delta integer as a string if timedelta64 scalar is given

    >>> import numpy as np
    >>> from comma.csv.time import from_numpy
    >>> from_numpy(np.datetime64('2015-01-02T12:34:56', 'us'))
    '20150102T123456'
    >>> from_numpy(np.datetime64('2015-01-02T12:34:56.010203', 'us'))
    '20150102T123456.010203'
    >>> from_numpy(np.datetime64('NaT'))
    'not-a-date-time'
    >>> from_numpy(np.timedelta64(123, 'us'))
    '123'
    >>> from_numpy(np.timedelta64(-123, 's'))
    '-123'
    """
    if not ((isinstance(t, np.datetime64) and t.dtype == DTYPE) or
            is_undefined(t) or
            isinstance(t, np.timedelta64)):
        msg = "expected numpy time or timedelta of type '{}' or '{}', got '{}'".format(repr(DTYPE), repr(np.timedelta64), repr(t))
        raise TypeError(msg)
    if isinstance(t, np.timedelta64): return str(t.astype('i8'))
    if is_undefined(t): return 'not-a-date-time'
    if is_negative_infinity(t): return '-infinity'
    if is_positive_infinity(t): return '+infinity'
    return re.sub(r'(\.0{6})?([-+]\d{4}|Z)?$', '', str(t)).translate(str.maketrans('', '', ':-'))

def ascii_converters(types):
    converters = {}
    for i, type in enumerate(types):
        if np.dtype(type) == DTYPE: converters[i] = to_numpy
    return converters

def get_time_zone(): return os.environ.get('TZ')

def set_time_zone(name):
    if name:
        os.environ['TZ'] = name
        time.tzset()
    elif 'TZ' in os.environ:
        del os.environ['TZ']
        time.tzset()

zone = set_time_zone
