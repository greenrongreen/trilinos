//@HEADER
// ************************************************************************
// 
//          Kokkos: A Fast Kernel Package
//              Copyright (2004) Sandia Corporation
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
// ************************************************************************
//@HEADER

#ifndef KOKKOS_DEFAULTARITHMETIC_H
#define KOKKOS_DEFAULTARITHMETIC_H

#include <Teuchos_TestForException.hpp>
#include <Teuchos_TypeNameTraits.hpp>
#include <stdexcept>

#include "Kokkos_MultiVector.hpp"

#ifndef KERNEL_PREFIX 
  #define KERNEL_PREFIX
#endif

namespace Kokkos {

  template <class Scalar, class Node>
  struct InitOp {
    typename Node::template buffer<Scalar>::buffer_t x;
    Scalar alpha;
    inline KERNEL_PREFIX void execute(int i) const
    {
      x[i] = alpha;
    }
  };

  template <class Scalar, class Node>
  struct AssignOp {
    typename Node::template buffer<      Scalar>::buffer_t x;
    typename Node::template buffer<const Scalar>::buffer_t y;
    inline KERNEL_PREFIX void execute(int i) const
    {
      x[i] = y[i];
    }
  };

  template <class Scalar, class Node>
  struct ScaleOp {
    typename Node::template buffer<const Scalar>::buffer_t x;
    typename Node::template buffer<      Scalar>::buffer_t y;
    inline KERNEL_PREFIX void execute(int i) const
    {
      Scalar tmp = y[i];
      y[i] = x[i]*tmp;
    }
  };

  template <class Scalar, class Node>
  struct RecipScaleOp {
    typename Node::template buffer<const Scalar>::buffer_t x;
    typename Node::template buffer<      Scalar>::buffer_t y;
    inline KERNEL_PREFIX void execute(int i) const
    {
      Scalar tmp = y[i];
      y[i] = tmp / x[i];
    }
  };

  template <class Scalar, class Node>
  struct DotOp {
    typename Node::template buffer<const Scalar>::buffer_t x, y;
    typedef Scalar ReductionType;
    inline static Scalar identity() {return 0.0;}
    Scalar reduce(Scalar x, Scalar y) {return x+y;}
    Scalar generate(int i) {return x[i]*y[i];}
  };

  //! Class DefaultArithmetic, unimplemented
  template <class MV>
  class DefaultArithmetic {
    public:
      //! Multiply one MultiVector by another, element-wise: B *= A
      static void Multiply(const MV &A, MV &B) { 
        TEST_FOR_EXCEPTION(true,std::logic_error,"DefaultArithmetic<" << Teuchos::typeName(A) << ">: no specialization exists for given multivector type.");
      }

      //! Divide one MultiVector by another, element-wise: B /= A
      static void Divide(MV &A, const MV &B) {
        TEST_FOR_EXCEPTION(true,std::logic_error,"DefaultArithmetic<" << Teuchos::typeName(A) << ": no specialization exists for given multivector type.");
      }
  };

  //! class DefaultArithmetic, for Kokkos::MultiVector
  template <class Scalar, class Ordinal, class Node>
  class DefaultArithmetic<MultiVector<Scalar,Ordinal,Node> > {
    public:

      //! Initialize multivector to constant value.
      inline static void Init(MultiVector<Scalar,Ordinal,Node> &A, Scalar alpha) {
        const Ordinal nR = A.getNumRows();
        const Ordinal nC = A.getNumCols();
        Node &node = A.getNode();
        if (A.getStride() == nR) {
          // one kernel invocation for whole multivector
          InitOp<Scalar,Node> wdp;
          wdp.x = A.getValues(0);
          wdp.alpha = alpha;
          node.template parallel_for<InitOp<Scalar,Node> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          InitOp<Scalar,Node> wdp;
          wdp.alpha = alpha;
          for (Ordinal j=0; j<nC; ++j) {
            wdp.x = A.getValues(j);
            node.template parallel_for<InitOp<Scalar,Node> >(0,nR,wdp);
          }
        }
      }

      //! Multiply one MultiVector by another, element-wise: B *= A
      inline static void Multiply(MultiVector<Scalar,Ordinal,Node> &A, const MultiVector<Scalar,Ordinal,Node> &B) {
        const Ordinal nR = A.getNumRows();
        const Ordinal nC = A.getNumCols();
        TEST_FOR_EXCEPTION(((nC != B.getNumCols()) && B.getNumCols() != 1)  ||
                           nR != B.getNumRows(), 
                           std::runtime_error,
                           "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Multiply(A,B): A and B must have the same dimensions.");
        Node &node = B.getNode();
        if (B.getNumCols() == 1) {
          ScaleOp<Scalar,Node> wdp;
          wdp.x = B.getValues(0);
          for (Ordinal j=0; j<nC; ++j) {
            wdp.y = A.getValues(j);
            node.template parallel_for<ScaleOp<Scalar,Node> >(0,nR,wdp);
          }
        }
        else if (A.getStride() == nR && B.getStride() == nR) {
          // one kernel invocation for whole multivector
          ScaleOp<Scalar,Node> wdp;
          wdp.y = A.getValues(0);
          wdp.x = B.getValues(0);
          node.template parallel_for<ScaleOp<Scalar,Node> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          ScaleOp<Scalar,Node> wdp;
          for (Ordinal j=0; j<nC; ++j) {
            wdp.y = A.getValues(j);
            wdp.x = B.getValues(j);
            node.template parallel_for<ScaleOp<Scalar,Node> >(0,nR,wdp);
          }
        }
      }

      //! Divide one MultiVector by another, element-wise: B /= A
      inline static void Divide(MultiVector<Scalar,Ordinal,Node> &A, const MultiVector<Scalar,Ordinal,Node> &B) {
        const Ordinal nR = A.getNumRows();
        const Ordinal nC = A.getNumCols();
        TEST_FOR_EXCEPTION(((nC != B.getNumCols()) && B.getNumCols() != 1)  ||
                           nR != B.getNumRows(), 
                           std::runtime_error,
                           "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Divide(A,B): A and B must have the same dimensions.");
        Node &node = B.getNode();
        RecipScaleOp<Scalar,Node> wdp;
        if (B.getNumCols() == 1) {
          // one kernel invocation for each column
          wdp.x = B.getValues(0);
          for (Ordinal j=0; j<nC; ++j) {
            wdp.y = A.getValues(j);
            node.template parallel_for<RecipScaleOp<Scalar,Node> >(0,nR,wdp);
          }
        }
        else if (A.getStride() == nR && B.getStride() == nR) {
          // one kernel invocation for whole multivector
          wdp.x = B.getValues(0);
          wdp.y = A.getValues(0);
          node.template parallel_for<RecipScaleOp<Scalar,Node> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          for (Ordinal j=0; j<nC; ++j) {
            wdp.y = A.getValues(j);
            wdp.x = B.getValues(j);
            node.template parallel_for<RecipScaleOp<Scalar,Node> >(0,nR,wdp);
          }
        }
      }

      //! Assign one MultiVector to another
      inline static void Assign(MultiVector<Scalar,Ordinal,Node> &A, const MultiVector<Scalar,Ordinal,Node> &B) {
        const Ordinal nR = A.getNumRows();
        const Ordinal nC = A.getNumCols();
        TEST_FOR_EXCEPTION(nC != B.getNumCols() ||
                           nR != B.getNumRows(), 
                           std::runtime_error,
                           "DefaultArithmetic<" << Teuchos::typeName(A) << ">::Divide(A,B): A and B must have the same dimensions.");
        Node &node = A.getNode();
        AssignOp<Scalar,Node> wdp;
        if (A.getStride() == nR && B.getStride() == nR) {
          // one kernel invocation for whole multivector assignment
          wdp.x = A.getValues(0);
          wdp.y = B.getValues(0);
          node.template parallel_for<AssignOp<Scalar,Node> >(0,nR*nC,wdp);
        }
        else {
          // one kernel invocation for each column
          for (Ordinal j=0; j<nC; ++j) {
            wdp.x = A.getValues(j);
            wdp.y = B.getValues(j);
            node.template parallel_for<AssignOp<Scalar,Node> >(0,nR,wdp);
          }
        }
      }
  };

}

#endif
