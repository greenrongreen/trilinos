// @HEADER
// ************************************************************************
// 
//            Phalanx: A Partial Differential Equation Assembly 
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

#ifndef PHX_TRAITS_HPP
#define PHX_TRAITS_HPP

// mpl (Meta Programming Library) templates
#include "Sacado_mpl_vector.hpp"
#include "Sacado_mpl_find.hpp"
#include "boost/mpl/map.hpp"
#include "boost/mpl/find.hpp"

// traits Base Class
#include "Phalanx_Traits_Base.hpp"

// Include User Data Types
#include "Phalanx_ConfigDefs.hpp"
#include "Sacado.hpp"
#include "Dimension.hpp"
#include "CellData.hpp"
#include "Phalanx_Allocator_New.hpp"

namespace PHX {

  struct MyTraits : public PHX::TraitsBase {
    
    // ******************************************************************
    // *** Scalar Types
    // ******************************************************************
    
    // Scalar types we plan to use
    typedef double RealType;
    typedef Sacado::Fad::DFad<double> FadType;
    
    // ******************************************************************
    // *** Evaluation Types
    // ******************************************************************
    struct Residual { typedef RealType ScalarT; };
    struct Jacobian { typedef FadType ScalarT;  };
    typedef Sacado::mpl::vector<Residual, Jacobian> EvalTypes;

    // ******************************************************************
    // *** Data Types
    // ******************************************************************
    
    // Create the data types for each evaluation type
    
    // Residual (default scalar type is RealType)
    typedef Sacado::mpl::vector<RealType> ResidualDataTypes;
  
    // Jacobian (default scalar type is Fad<double, double>)
    typedef Sacado::mpl::vector<FadType> JacobianDataTypes;

    // Maps the key EvalType a vector of DataTypes
    typedef boost::mpl::map<
      boost::mpl::pair<Residual, ResidualDataTypes>,
      boost::mpl::pair<Jacobian, JacobianDataTypes>
    >::type EvalToDataMap;

    // ******************************************************************
    // *** Allocator Type
    // ******************************************************************
    typedef PHX::NewAllocator Allocator;

    // ******************************************************************
    // *** User Defined Object Passed in for Evaluation Method
    // ******************************************************************
    typedef std::vector<CellData>& EvalData;
    typedef void* PreEvalData;
    typedef void* PostEvalData;

  };
 
  // ******************************************************************
  // ******************************************************************
  // Debug strings.  Specialize the Evaluation and Data types for the
  // TypeString object in the PHX::TraitsBase class.
  // ******************************************************************
  // ******************************************************************

  // Evaluation Types
  template<>
  struct MyTraits::TypeString<MyTraits::Residual> 
  { static const std::string value; };
  const std::string MyTraits::TypeString<MyTraits::Residual>::value = 
    "Residual";

  template<>
  struct MyTraits::TypeString<MyTraits::Jacobian> 
  { static const std::string value; };
  const std::string MyTraits::TypeString<MyTraits::Jacobian>::value = 
    "Jacobian";

  // Data Types
  template<>
  struct MyTraits::TypeString<MyTraits::RealType> 
  { static const std::string value; };
  const std::string MyTraits::TypeString<MyTraits::RealType>::value = 
    "double";

  template<>
  struct MyTraits::TypeString<MyTraits::FadType> 
  { static const std::string value; };
  const std::string MyTraits::TypeString<MyTraits::FadType>::value = 
    "Sacado::Fad::DFad<double>";

}

#endif
