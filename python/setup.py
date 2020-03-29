#!/usr/bin/env python3

from distutils.core import setup

setup(
        name                = 'comma',
        version             = open('comma/version.py').readlines()[-1].strip().split()[-1].strip('\"'),
        description         = 'comma python utilties',
        url                 = 'https://github.com/acfr/comma',
        license             = 'BSD 3-Clause',
        packages            = [ 'comma', 'comma.csv', 'comma.csv.applications', 'comma.io', 'comma.numpy', 'comma.signal', 'comma.util', 'comma.cpp_bindings', 'comma.application' ],
        package_dir         = { 'comma.cpp_bindings': 'comma/cpp_bindings' },
        package_data        = { 'comma.cpp_bindings': [ '*.so', '*.dll' ] }
     )
