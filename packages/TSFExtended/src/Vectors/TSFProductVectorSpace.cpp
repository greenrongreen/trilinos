/* @HEADER@ */
/* ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
//                 Copyright (2004) Sandia Corporation
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
// **********************************************************************/
/* @HEADER@ */


#include "TSFProductVectorSpace.hpp"
#include "TSFProductVector.hpp"


using namespace TSFExtended;
using namespace Teuchos;
using std::ostream;


//========================================================================
template <class Scalar>
ProductVectorSpace<Scalar>::
  ProductVectorSpace(const Teuchos::Array<const VectorSpace<Scalar> >
		     &vecSpaces)
    :vecSpaces_(vecSpaces),
     numBlocks_(vecSpaces.size()),
     isFinal_(true)
  {
    isSet_.resize(vecSpaces.size());
    finalize();
  }
    



//========================================================================
template <class Scalar>
void ProductVectorSpace<Scalar>::finalize()
{
  isFinal_ = true;
  isInCore_ = true;
  dim_ = 0;
  for (int i = 0; i < numBlocks_; i++)
    {
      TEST_FOR_EXCEPTION(vecSpaces_[i].ptr() == 0, runtime_error,
			 "Vector space " << i << " not set" << endl)
	isSet_[i] = true;

      if (!vecSpaces_[i].isInCore()) isInCore_ = false;
      dim_ += vecSpaces_[i].dim();
    }
}



//========================================================================
template <class Scalar>
bool ProductVectorSpace<Scalar>::operator==(const VectorSpace<Scalar> &other) 
  const
{
  const ProductVectorSpace<Scalar>* otherPVS = 
    dynamic_cast<const ProductVectorSpace<Scalar>* > (other.ptr());
  if (otherPVS == 0)
    {
      return false;
    }
  for (int i = 0; i < numBlocks_; i++)
    {
      if (vecSpaces_[i] != other->getBlock(i)) 
	{
	  return false;
	} 
    }
  return true;
}



//========================================================================
template <class Scalar>
bool ProductVectorSpace<Scalar>::operator!=(const VectorSpace<Scalar>& other) 
const
{
  return !(operator==(other));
}



//========================================================================
template <class Scalar>
bool ProductVectorSpace<Scalar>::isCompatible(const VectorSpace<Scalar> 
					      &other ) const
{
  const ProductVectorSpace<Scalar>* otherPVS = 
    dynamic_cast<const ProductVectorSpace<Scalar>* > (other.ptr());
  if (otherPVS == 0) return false;

  for (int i = 0; i < numBlocks_; i++)
    {
      if (!vecSpaces_[i].isCompatible(other.getBlock(i)))
	{
	  return false;
	}
    }
  return true;
}







//========================================================================
template <class Scalar>
void ProductVectorSpace<Scalar>::setBlock(const int i, 
					  const VectorSpace<Scalar> &subSp)
  {
    TEST_FOR_EXCEPTION(vecSpace_[i] != subSp, runtime_error,
		       "subspace not compatible with existing space: "
		       << "subSp is " << subSp.describe() 
		       << " vecSpace_{i} is" << vecSpace_[i].describe() 
		       << endl);
    vecSpace_[i] = subSp;
  }
  



//========================================================================
template <class Scalar>
Vector<Scalar> ProductVectorSpace<Scalar>::createMember() const
  {
    return new ProductVector<Scalar>(*this);
  }
