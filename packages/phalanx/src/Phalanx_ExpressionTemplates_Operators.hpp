// @HEADER
// ************************************************************************
// 
//        Phalanx: A Partial Differential Equation Field Evaluation 
//       Kernel for Flexible Management of Complex Dependency Chains
//                  Copyright (2008) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// 
// Questions? Contact Roger Pawlowski (rppawlo@sandia.gov), Sandia
// National Laboratories.
// 
// ************************************************************************
// @HEADER

#ifndef PHALANX_EXPRESSION_TEMPLATES_OPERATORS_HPP
#define PHALANX_EXPRESSION_TEMPLATES_OPERATORS_HPP

#include <cassert>
#include "Phalanx_ConfigDefs.hpp"
#include "Phalanx_ExpressionTemplates_Operands.hpp"
#include "Phalanx_ExpressionTemplates_Array.hpp"

namespace PHX {

  //! Addition of two arrays
  template<typename Ordinal, typename Scalar, typename R1, typename R2>
  PHX::ExprArray<Ordinal, Scalar, PHX::ExprAdd<Ordinal,Scalar,R1,R2> > 
  operator+(PHX::ExprArray<Ordinal,Scalar,R1> const& a, 
	    PHX::ExprArray<Ordinal,Scalar,R2> const& b) {
    return 
      PHX::ExprArray<Ordinal, Scalar, PHX::ExprAdd<Ordinal,Scalar,R1,R2> > 
      (PHX::ExprAdd<Ordinal,Scalar,R1,R2>(a.rep(), b.rep()));
  }    

}

#endif
