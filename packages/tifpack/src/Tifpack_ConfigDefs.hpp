/*@HEADER
// ***********************************************************************
// 
//       Tifpack: Tempated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2009) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
//@HEADER
*/

#ifndef _TIFPACK_CONFIGDEFS_HPP_
#define _TIFPACK_CONFIGDEFS_HPP_

#include <Tifpack_config.h>

/* sign function stolen from epetra. Perhaps this will need to
 be a template function which uses Teuchos::ScalarTraits...
*/
#define TIFPACK_SGN(x) (((x) < 0.0) ? -1.0 : 1.0)

#ifdef HAVE_TIFPACK_MPI

#endif

// prints out an error message if variable is not zero,
// and returns this value.
#define TIFPACK_CHK_ERR(ifpack_err) \
{ if (ifpack_err < 0) { \
  std::cerr << "TIFPACK ERROR " << ifpack_err << ", " \
    << __FILE__ << ", line " << __LINE__ << std::endl; \
    return(ifpack_err);  } }

// prints out an error message if variable is not zero,
// and returns void
#define TIFPACK_CHK_ERRV(ifpack_err) \
{ if (ifpack_err < 0) { \
  std::cerr << "TIFPACK ERROR " << ifpack_err << ", " \
    << __FILE__ << ", line " << __LINE__ << std::endl; \
    return;  } }
// prints out an error message and returns
#define TIFPACK_RETURN(ifpack_err) \
{ if (ifpack_err < 0) { \
  std::cerr << "TIFPACK ERROR " << ifpack_err << ", " \
    << __FILE__ << ", line " << __LINE__ << std::endl; \
		       } return(ifpack_err); }

#define TIFPACK_SGN(x) (((x) < 0.0) ? -1.0 : 1.0)  /* sign function */

#include <Tpetra_ConfigDefs.hpp>
namespace Tifpack {
  typedef Tpetra::global_size_t global_size_t;
}

#endif /*_TIFPACK_CONFIGDEFS_HPP_*/
