#! /usr/bin/env python

# System imports
from   distutils.core import *
from   distutils      import sysconfig
import commands
import os
import sys

# Build the python library directory name and library name
pythonDir = [sysconfig.get_config_var('LIBPL'  )      ]
pythonLib = [sysconfig.get_config_var('LIBRARY')[3:-2]]

# Trilinos data
searchList = ["/usr/local", "/Users/aalorbe/local", "~/scratch2/local",
              "/usr/netpub/Trilinos-10_31_03", "/smallHD/scratch/install"]
trilHome = None
for dir in searchList:
    epetraHeader = dir + "/include/Epetra_Comm.h"
    if os.path.isfile(epetraHeader):
        trilHome = dir
        break
if not trilHome:
    raise RunTimeError, "Trilinos not found"
if trilHome == "/usr/local":
    trilIncDir = [ ]
else:
    trilIncDir = [trilHome + "/include"]
trilLibDir   = [trilHome + "/lib"]
epetraLib    = ["epetra"   ]
epetraExtLib = ["epetraext"]
noxEpetraLib = ["noxepetra"]
noxLib       = ["nox"      ]
aztecLib     = ["aztecoo"  ]
ifpackLib    = ["ifpack"   ]

# LAPACK hack
sysName = os.uname()[0]
if sysName == "Darwin":
    lapackFwk = ["-framework", "veclib"]
    lapackLib = [ ]
else:
    lapackFwk = [ ]
    lapackLib = ["blas", "lapack", "g2c"]

# SWIG Python library
swigStrings = commands.getoutput("swig -python -ldflags").split()
swigDir     = [swigStrings[0][2:]]
swigLib     = [swigStrings[1][2:]]
if sysName == "Linux":
    swigPath = ["-Wl,-rpath"] + swigDir
else:
    swigPath = [ ]

# Standard C++ library
if sysName == "Linux":
    stdCXX = ["stdc++"]
else:
    stdCXX = [ ]

# Standard extension data
stdIncDirs = trilIncDir
stdLibDirs = trilLibDir + swigDir
stdLibs    = stdCXX + swigLib
stdArgs    = swigPath

# Epetra extension module
RawEpetra = Extension("PyTrilinos._RawEpetra",
                      ["src/RawEpetra_wrap.cxx",
                       "src/Epetra_NumPyVector.cxx",
                       "src/Epetra_VectorHelper.cxx",
                       "src/NumPyArray.cxx",
                       "src/NumPyWrapper.cxx"],
                      include_dirs    = stdIncDirs,
                      library_dirs    = stdLibDirs + pythonDir,
                      libraries       = stdLibs + epetraLib + lapackLib + pythonLib,
                      extra_link_args = swigPath + lapackFwk
                      )

# EpetraExt extension module
EpetraExt = Extension("PyTrilinos._EpetraExt",
                      ["src/EpetraExt_wrap.cxx"],
                      include_dirs    = stdIncDirs,
                      library_dirs    = stdLibDirs + pythonDir,
                      libraries       = stdLibs + epetraExtLib + epetraLib + \
                                        lapackLib + pythonLib,
                      extra_link_args = swigPath + lapackFwk
                      )

# NOX_Epetra extension module
NOX_Epetra = Extension("PyTrilinos.NOX._Epetra",
                       ["src/NOX/Epetra_wrap.cxx",
                        "src/Epetra_VectorHelper.cxx",
                        "src/NumPyArray.cxx",
                        "src/NumPyWrapper.cxx",
                        "src/NOX/Callback.cxx",
                        "src/NOX/PyInterface.cxx"],
                       include_dirs    = stdIncDirs,
                       library_dirs    = stdLibDirs,
                       libraries       = stdLibs + noxEpetraLib + noxLib +     \
                                         epetraExtLib + epetraLib + aztecLib + \
                                         ifpackLib + lapackLib,
                       extra_link_args = swigPath + lapackFwk
                       )

# NOX.Abstract extension module
NOX_Abstract = Extension("PyTrilinos.NOX._Abstract",
                         ["src/NOX/Abstract_wrap.cxx"],
                         include_dirs    = stdIncDirs,
                         library_dirs    = stdLibDirs,
                         libraries       = stdLibs + noxLib,
                         extra_link_args = swigPath
                         )

# NOX.Parameter extension module
NOX_Parameter = Extension("PyTrilinos.NOX._Parameter",
                          ["src/NOX/Parameter_wrap.cxx"],
                          include_dirs    = stdIncDirs,
                          library_dirs    = stdLibDirs,
                          libraries       = stdLibs + noxLib,
                          extra_link_args = swigPath
                          )

# NOX.Solver extension module
NOX_Solver = Extension("PyTrilinos.NOX._Solver",
                       ["src/NOX/Solver_wrap.cxx"],
                       include_dirs    = stdIncDirs,
                       library_dirs    = stdLibDirs,
                       libraries       = stdLibs + noxLib,
                       extra_link_args = swigPath
                       )

# NOX.StatusTest extension module
NOX_StatusTest = Extension("PyTrilinos.NOX._StatusTest",
                           ["src/NOX/StatusTest_wrap.cxx"],
                           include_dirs    = stdIncDirs,
                           library_dirs    = stdLibDirs,
                           libraries       = stdLibs + noxLib,
                           extra_link_args = swigPath
                           )

# PyTrilinos setup
setup(name         = "PyTrilinos",
      version      = "4.0",
      description  = "Python Trilinos Interface",
      author       = "Bill Spotz",
      author_email = "wfspotz@sandia.gov",
      package_dir  = {"PyTrilinos" : "src"},
      packages     = ["PyTrilinos", "PyTrilinos.NOX"],
      ext_modules  = [ RawEpetra,    EpetraExt,     NOX_Epetra,
                       NOX_Abstract, NOX_Parameter, NOX_Solver,
                       NOX_StatusTest                          ]
      )
