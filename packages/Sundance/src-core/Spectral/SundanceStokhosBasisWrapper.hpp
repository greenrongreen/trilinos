/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#ifndef SUNDANCE_STOKHOSBASISWRAPPER_H
#define SUNDANCE_STOKHOSBASISWRAPPER_H

#include "SundanceDefs.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "SundanceMap.hpp"
#include "SundanceSpectralBasisBase.hpp"

#include "Stokhos_OrthogPolyBasis.hpp"
#include "Stokhos_OneDOrthogPolyBasis.hpp"

#ifdef HAVE_SUNDANCE_STOKHOS

#include "cijk.h"
#include "chaos.h"



using namespace std;
using namespace Sundance;

namespace Sundance
{
/** 
 * 
 */
class  StokhosBasisWrapper : public SpectralBasisBase
{
public: 
  /** */
  typedef Stokhos::OrthogPolyBasis<int, double> PolyBasis;
  /** */
  typedef Stokhos::OneDOrthogPolyBasis<int, double> PolyBasis1D;

  /** Construct a basis */
  StokhosBasisWrapper(const RCP<const PolyBasis>& basis);
  /** Construct a basis */
  StokhosBasisWrapper(const RCP<const PolyBasis1D>& basis);
    
  /** Return the dim of the Spectral Basis */
  int getDim() const {return basis_->dimension();}
  
  /** Return the order of the Spectral Basis */
  int getOrder() const {return basis_->order();}
  
  /** Return the maximum number of terms */
  int nterms() const {return basis_->size();}
  
  /** Return the basis element stored in the basis array index */
  int getElement(int i) const {return i;}
  
  /** expectation operator */
  double expectation(int i, int j, int k);
  
  /** Write to a string */
  string toString() const {return basis_->getName();}
  
  /* */
  GET_RCP(SpectralBasisBase);
  
  /** Ordering operator */
  virtual bool lessThan(const SpectralBasisBase* other) const ;

private:
  RCP<const PolyBasis> basis_;
  RCP<Stokhos::Sparse3Tensor<int, double> > cijk_;
  
  /** Fill the c_ijk array */
  void fillCijk() ;
};
}

#endif
#endif
