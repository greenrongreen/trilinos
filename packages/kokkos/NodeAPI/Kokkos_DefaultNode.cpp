// @HEADER
// ***********************************************************************
// 
//          Kokkos: Node API and Parallel Node Kernels
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

#include "Kokkos_DefaultNode.hpp"
#include <Teuchos_ParameterList.hpp>
#include <iostream>

Teuchos::RCP<Kokkos::DefaultNode::DefaultNodeType> Kokkos::DefaultNode::node_ = Teuchos::null;

namespace Kokkos {

  RCP<DefaultNode::DefaultNodeType> DefaultNode::getDefaultNode() 
  {
    if (node_ == null) {
      Teuchos::ParameterList pl;
#ifdef HAVE_KOKKOS_THREADPOOL
      pl.set<int>("Num Threads",1);
      node_ = rcp<TPINode>(new TPINode(pl));
#else
#  ifdef HAVE_KOKKOS_TBB
      pl.set<int>("Num Threads",0);
      node_ = rcp<TBBNode>(new TBBNode(pl));
#  else
      node_ = rcp<SerialNode>(new SerialNode(pl));
#  endif
#endif
    }
    return node_;
  }

}
