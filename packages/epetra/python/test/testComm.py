#! /usr/bin/env python

# @HEADER
# ************************************************************************
#
#              PyTrilinos.Epetra: Python Interface to Epetra
#                   Copyright (2005) Sandia Corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? Contact Michael A. Heroux (maherou@sandia.gov)
#
# ************************************************************************
# @HEADER

# Imports.  Users importing an installed version of PyTrilinos should use the
# "from PyTrilinos import ..." syntax.  Here, the setpath module adds the build
# directory, including "PyTrilinos", to the front of the search path.  We thus
# use "import ..." for Trilinos modules.  This prevents us from accidentally
# picking up a system-installed version and ensures that we are testing the
# build module.
import sys

try:
    import setpath
    import Epetra
except ImportError:
    from PyTrilinos import Epetra
    print >>sys.stderr, "Using system-installed Epetra"

import os
import unittest
from   Numeric    import *

##########################################################################

class EpetraSerialCommTestCase(unittest.TestCase):
    "TestCase class for SerialComm communicator objects"

    def setUp(self):
        self.comm     = Epetra.SerialComm()
        self.filename = os.path.splitext(os.path.split(__file__)[1])[0] + ".dat"
        self.file     = open(self.filename, 'w')

    def tearDown(self):
        self.file.close()

    def testMyPID(self):
        "Test Epetra.SerialComm MyPID method"
        self.assertEqual(self.comm.MyPID()  , 0)

    def testNumProc(self):
        "Test Epetra.SerialComm NumProc method"
        self.assertEqual(self.comm.NumProc(), 1)

    def testStr(self):
        "Test Epetra.SerialComm __str__ method"
        self.assertEqual(str(self.comm), "::Processor 0 of 1 total processors.")

    def testPrint(self):
        "Test Epetra.SerialComm Print method"
        self.comm.Print(self.file)
        self.file.close()
        ifile = open(self.filename,'r')
        self.assertEqual(ifile.read(), "::Processor 0 of 1 total processors.")
        ifile.close()

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraSerialCommTestCase ))

    # Run the test suite
    print >>sys.stderr, \
          "\n*******************\nTesting Epetra.Comm\n*******************\n"
    result = unittest.TextTestRunner(verbosity=2).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    sys.exit(len(result.errors) + len(result.failures))
