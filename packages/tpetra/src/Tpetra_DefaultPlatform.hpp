// @HEADER
// ***********************************************************************
// 
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// @HEADER

#ifndef TPETRA_DEFAULT_PLATFORM_HPP
#define TPETRA_DEFAULT_PLATFORM_HPP

#include <Kokkos_DefaultNode.hpp>
#include "Tpetra_ConfigDefs.hpp"
#include "Tpetra_SerialPlatform.hpp"
#ifdef HAVE_MPI
#  include "Tpetra_MpiPlatform.hpp"
#endif

namespace Tpetra {

/** \brief Returns a default platform appropriate for the
 * enviroment.
 *
 * If HAVE_MPI is defined, then an instance of <tt>MpiPlatform</tt> will be
 * created.  Otherwise, a <tt>SerialPlatform</tt>
 * is returned.
 *
 * This class is templated on \c Scalar, \c LocalOrdinal and \c GlobalOrdinal. 
 * The \c LocalOrdinal type, if omitted, defaults to \c int. The \c GlobalOrdinal 
 * type, if omitted, defaults to the \c LocalOrdinal type.
 */
class DefaultPlatform {
public:
#ifdef HAVE_TPETRA_MPI
  typedef MpiPlatform<Kokkos::DefaultNode::DefaultNodeType> DefaultPlatformType;
#else
  typedef SerialPlatform<Kokkos::DefaultNode::DefaultNodeType> DefaultPlatformType;
#endif


  /** \brief Return the default platform.
   */
  static DefaultPlatformType & getDefaultPlatform();

private:

  static Teuchos::RCP<DefaultPlatformType> platform_;

};

} // namespace Tpetra

#endif // TPETRA_DEFAULT_PLATFORM_HPP

