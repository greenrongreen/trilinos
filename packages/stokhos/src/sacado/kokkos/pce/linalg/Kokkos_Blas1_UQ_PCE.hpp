// @HEADER
// ***********************************************************************
//
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#ifndef KOKKOS_BLAS1_UQ_PCE_HPP
#define KOKKOS_BLAS1_UQ_PCE_HPP

#include "Sacado_UQ_PCE.hpp"
#include "Kokkos_View_UQ_PCE.hpp"
#include "Kokkos_InnerProductSpaceTraits_UQ_PCE.hpp"
#include "Kokkos_Blas1_MV.hpp"

//----------------------------------------------------------------------------
// Specializations of Kokkos Vector/MultiVector math functions
//----------------------------------------------------------------------------

namespace KokkosBlas {

// Rank-1 vector add with Sacado::UQ::PCE scalar type, constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM,
          typename YS, typename YL, typename YD, typename YM>
void
update(const typename Sacado::UQ::PCE<XS>::value_type& av,
       const Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM >& x,
       const typename Sacado::UQ::PCE<XS>::value_type& bv,
       const Kokkos::View< Sacado::UQ::PCE<YS>*, YL, YD, YM >& y,
       const typename Sacado::UQ::PCE<XS>::value_type& cv,
       const Kokkos::View< Sacado::UQ::PCE<RS>*, RL, RD, RM >& r)
{
  typedef Kokkos::View< Sacado::UQ::PCE<RS>*, RL, RD, RM > RVector;
  typedef Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM > XVector;
  typedef Kokkos::View< Sacado::UQ::PCE<YS>*, YL, YD, YM > YVector;

  typename RVector::flat_array_type r_flat = r;
  typename XVector::flat_array_type x_flat = x;
  typename YVector::flat_array_type y_flat = y;

  update( av, x_flat, bv, y_flat, cv , r_flat);
}

// Rank-1 vector add with Sacado::UQ::PCE scalar type, non-constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM,
          typename YS, typename YL, typename YD, typename YM>
void
update(const Sacado::UQ::PCE<XS>& av,
       const Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM >& x,
       const Sacado::UQ::PCE<XS>& bv,
       const Kokkos::View< Sacado::UQ::PCE<YS>*, YL, YD, YM >& y,
       const Sacado::UQ::PCE<XS>& cv,
       const Kokkos::View< Sacado::UQ::PCE<RS>*, RL, RD, RM >& r)
{
  if (Sacado::is_constant(av) && Sacado::is_constant(bv)) {
   return update( av.fastAccessCoeff(0), x, bv.fastAccessCoeff(0), y , cv.fastAccessCoeff(0), r);
  }
  else {
    Kokkos::Impl::raise_error("V_Add not implemented for non-constant a or b");
  }
}

// Rank-2 vector add with Sacado::UQ::PCE scalar type, constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM,
          typename YS, typename YL, typename YD, typename YM>
void
update( const typename Sacado::UQ::PCE<XS>::value_type& av,
        const Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM >& x,
        const typename Sacado::UQ::PCE<XS>::value_type& bv,
        const Kokkos::View< Sacado::UQ::PCE<YS>**, YL, YD, YM >& y,
        const typename Sacado::UQ::PCE<XS>::value_type& cv,
        const Kokkos::View< Sacado::UQ::PCE<RS>**, RL, RD, RM >& r)
{
  typedef Kokkos::View< Sacado::UQ::PCE<RS>**, RL, RD, RM > RVector;
  typedef Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM > XVector;
  typedef Kokkos::View< Sacado::UQ::PCE<YS>**, YL, YD, YM > YVector;

  typename RVector::flat_array_type r_flat = r;
  typename XVector::flat_array_type x_flat = x;
  typename YVector::flat_array_type y_flat = y;

  update( av, x_flat, bv, y_flat , cv, r_flat );
}

// Rank-2 vector add with Sacado::UQ::PCE scalar type, non-constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM,
          typename YS, typename YL, typename YD, typename YM>
void
update( const Sacado::UQ::PCE<XS>& av,
        const Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM >& x,
        const Sacado::UQ::PCE<XS>& bv,
        const Kokkos::View< Sacado::UQ::PCE<YS>**, YL, YD, YM >& y,
        const Sacado::UQ::PCE<XS>& cv,
        const Kokkos::View< Sacado::UQ::PCE<RS>**, RL, RD, RM >& r)
{
  if (Sacado::is_constant(av) && Sacado::is_constant(bv)) {
    return update( av.fastAccessCoeff(0), x, bv.fastAccessCoeff(0), y, cv.fastAccessCoeff(0) , r);
  }
  else {
    Kokkos::Impl::raise_error("MV_Add not implemented for non-constant a or b");
  }
}

// Rank-1 dot product
template <typename XS, typename XL, typename XD, typename XM,
          typename YS, typename YL, typename YD, typename YM>
typename Kokkos::Details::InnerProductSpaceTraits< Sacado::UQ::PCE<XS> >::dot_type
dot( const Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM >& x,
       const Kokkos::View< Sacado::UQ::PCE<YS>*, YL, YD, YM >& y)
{
  typedef Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM > XVector;
  typedef Kokkos::View< Sacado::UQ::PCE<YS>*, YL, YD, YM > YVector;

  typename XVector::flat_array_type x_flat = x;
  typename YVector::flat_array_type y_flat = y;

  return dot( x_flat, y_flat);
}

// Rank-2 dot product
template <typename rVector,
          typename XS, typename XL, typename XD, typename XM,
          typename YS, typename YL, typename YD, typename YM>
void
dot( const rVector& r,
        const Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM >& x,
        const Kokkos::View< Sacado::UQ::PCE<YS>**, YL, YD, YM >& y)
{
  typedef Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM > XVector;
  typedef Kokkos::View< Sacado::UQ::PCE<YS>**, YL, YD, YM > YVector;

  typename XVector::flat_array_type x_flat = x;
  typename YVector::flat_array_type y_flat = y;

  dot( r, x_flat, y_flat );
}

template<class VT1, class VT2, class VT3>
struct MV_ElementWiseMultiplyFunctor;

template <typename CT, typename CD, typename CM,
          typename AT, typename AD, typename AM,
          typename BT, typename BD, typename BM>
struct MV_ElementWiseMultiplyFunctor<
  Kokkos::View< CT,Kokkos::LayoutLeft,CD,CM,Kokkos::Impl::ViewPCEContiguous >,
  Kokkos::View< AT,Kokkos::LayoutLeft,AD,AM,Kokkos::Impl::ViewPCEContiguous >,
  Kokkos::View< BT,Kokkos::LayoutLeft,BD,BM,Kokkos::Impl::ViewPCEContiguous > >
{
  typedef Kokkos::View< CT,Kokkos::LayoutLeft,CD,CM,Kokkos::Impl::ViewPCEContiguous > CVector;
  typedef Kokkos::View< AT,Kokkos::LayoutLeft,AD,AM,Kokkos::Impl::ViewPCEContiguous > AVector;
  typedef Kokkos::View< BT,Kokkos::LayoutLeft,BD,BM,Kokkos::Impl::ViewPCEContiguous > BVector;

  typedef typename CVector::array_type CArray;
  typedef typename AVector::array_type AArray;
  typedef typename BVector::array_type BArray;

  typedef typename CArray::execution_space        execution_space;
  typedef typename CArray::size_type            size_type;

  typename CArray::const_value_type m_c;
  CArray m_C;
  typename AArray::const_value_type m_ab;
  typename AArray::const_type m_A ;
  typename BArray::const_type m_B ;
  const size_type m_n;
  const size_type m_n_pce;

  MV_ElementWiseMultiplyFunctor(typename CVector::const_value_type c,
                                CVector C,
                                typename AVector::const_value_type ab,
                                typename AVector::const_type A,
                                typename BVector::const_type B,
                                const size_type n) :
    m_c(c.fastAccessCoeff(0)),
    m_C(C),
    m_ab(ab.fastAccessCoeff(0)),
    m_A(A),
    m_B(B),
    m_n(n),
    m_n_pce(C.sacado_size()) {}

  KOKKOS_INLINE_FUNCTION
  void operator()( const size_type i) const
  {
    // Currently specialized for use case where A is degree-0
    typename AArray::const_value_type Ai = m_A(0,i);
    for (size_type k=0; k<m_n; ++k) {
#ifdef KOKKOS_HAVE_PRAGMA_IVDEP
#pragma ivdep
#endif
      for (size_type l=0; l<m_n_pce; ++l)
        m_C(l,i,k) = m_c*m_C(l,i,k) + m_ab*Ai*m_B(l,i,k);
    }
  }
};

template<class VT1, class VT2, class VT3>
struct V_ElementWiseMultiplyFunctor;

template <typename CT, typename CD, typename CM,
          typename AT, typename AD, typename AM,
          typename BT, typename BD, typename BM>
struct V_ElementWiseMultiplyFunctor<
  Kokkos::View< CT,Kokkos::LayoutLeft,CD,CM,Kokkos::Impl::ViewPCEContiguous >,
  Kokkos::View< AT,Kokkos::LayoutLeft,AD,AM,Kokkos::Impl::ViewPCEContiguous >,
  Kokkos::View< BT,Kokkos::LayoutLeft,BD,BM,Kokkos::Impl::ViewPCEContiguous > >
{
  typedef Kokkos::View< CT,Kokkos::LayoutLeft,CD,CM,Kokkos::Impl::ViewPCEContiguous > CVector;
  typedef Kokkos::View< AT,Kokkos::LayoutLeft,AD,AM,Kokkos::Impl::ViewPCEContiguous > AVector;
  typedef Kokkos::View< BT,Kokkos::LayoutLeft,BD,BM,Kokkos::Impl::ViewPCEContiguous > BVector;

  typedef typename CVector::array_type CArray;
  typedef typename AVector::array_type AArray;
  typedef typename BVector::array_type BArray;

  typedef typename CArray::execution_space        execution_space;
  typedef typename CArray::size_type            size_type;

  typename CArray::const_value_type m_c;
  CArray m_C;
  typename AArray::const_value_type m_ab;
  typename AArray::const_type m_A ;
  typename BArray::const_type m_B ;
  const size_type m_n_pce;

  V_ElementWiseMultiplyFunctor(typename CVector::const_value_type c,
                               CVector C,
                               typename AVector::const_value_type ab,
                               typename AVector::const_type A,
                               typename BVector::const_type B) :
    m_c(c.fastAccessCoeff(0)),
    m_C(C),
    m_ab(ab.fastAccessCoeff(0)),
    m_A(A),
    m_B(B),
    m_n_pce(C.sacado_size()) {}

  KOKKOS_INLINE_FUNCTION
  void operator()( const size_type i) const
  {
    // Currently specialized for use case where A is degree-0
    typename AArray::const_value_type Ai = m_A(0,i);
#ifdef KOKKOS_HAVE_PRAGMA_IVDEP
#pragma ivdep
#endif
    for (size_type l=0; l<m_n_pce; ++l)
      m_C(l,i) = m_c*m_C(l,i) + m_ab*Ai*m_B(l,i);
  }
};

// Rank-1 vector multiply with Sacado::UQ::PCE scalar type, constant c, ab
template <typename CS, typename CL, typename CD, typename CM,
          typename AS, typename AL, typename AD, typename AM,
          typename BS, typename BL, typename BD, typename BM>
void
mult(
  const typename Sacado::UQ::PCE<CS>::value_type& c,
  const Kokkos::View< Sacado::UQ::PCE<CS>*, CL, CD, CM >& C,
  const typename Sacado::UQ::PCE<AS>::value_type& ab,
  const Kokkos::View< Sacado::UQ::PCE<AS>*, AL, AD, AM >& A,
  const Kokkos::View< Sacado::UQ::PCE<BS>*, BL, BD, BM >& B )
{
  typedef Kokkos::View< Sacado::UQ::PCE<CS>*, CL, CD, CM > CVector;
  typedef Kokkos::View< Sacado::UQ::PCE<AS>*, AL, AD, AM > AVector;
  typedef Kokkos::View< Sacado::UQ::PCE<BS>*, BL, BD, BM > BVector;

  typedef Kokkos::View< typename CVector::data_type, typename CVector::array_layout, typename CVector::execution_space, typename CVector::memory_traits > CView;
  typedef Kokkos::View< typename AVector::data_type, typename AVector::array_layout, typename AVector::execution_space, typename AVector::memory_traits > AView;
  typedef Kokkos::View< typename BVector::data_type, typename BVector::array_layout, typename BVector::execution_space, typename BVector::memory_traits > BView;

  V_ElementWiseMultiplyFunctor<CView,AView,BView> op(c,C,ab,A,B) ;
  Kokkos::parallel_for( C.dimension_0() , op );
}

// Rank-1 vector multiply with Sacado::UQ::PCE scalar type, non-constant c, ab
template <typename CS, typename CL, typename CD, typename CM,
          typename AS, typename AL, typename AD, typename AM,
          typename BS, typename BL, typename BD, typename BM>
void
mult(
  const Sacado::UQ::PCE<CS>& c,
  const Kokkos::View< Sacado::UQ::PCE<CS>*, CL, CD, CM >& C,
  const Sacado::UQ::PCE<AS>& ab,
  const Kokkos::View< Sacado::UQ::PCE<AS>*, AL, AD, AM >& A,
  const Kokkos::View< Sacado::UQ::PCE<BS>*, BL, BD, BM >& B )
{
  if (Sacado::is_constant(c) && Sacado::is_constant(ab)) {
    return mult( c.fastAccessCoeff(0), C,
                                  ab.fastAccessCoeff(0), A, B );
  }
  else {
    Kokkos::Impl::raise_error(
      "V_ElementWiseMultiply not implemented for non-constant c or ab");
  }
}

// Rank-2 vector multiply with Sacado::UQ::PCE scalar type, constant c, ab
template <typename CS, typename CL, typename CD, typename CM,
          typename AS, typename AL, typename AD, typename AM,
          typename BS, typename BL, typename BD, typename BM>
void
mult(
  const typename Sacado::UQ::PCE<CS>::value_type& c,
  const Kokkos::View< Sacado::UQ::PCE<CS>**, CL, CD, CM >& C,
  const typename Sacado::UQ::PCE<AS>::value_type& ab,
  const Kokkos::View< Sacado::UQ::PCE<AS>*,  AL, AD, AM >& A,
  const Kokkos::View< Sacado::UQ::PCE<BS>**, BL, BD, BM >& B )
{
  typedef Kokkos::View< Sacado::UQ::PCE<CS>**, CL, CD, CM > CVector;
  typedef Kokkos::View< Sacado::UQ::PCE<AS>*,  AL, AD, AM > AVector;
  typedef Kokkos::View< Sacado::UQ::PCE<BS>**, BL, BD, BM > BVector;

  typedef Kokkos::View< typename CVector::data_type, typename CVector::array_layout, typename CVector::execution_space, typename CVector::memory_traits > CView;
  typedef Kokkos::View< typename AVector::data_type, typename AVector::array_layout, typename AVector::execution_space, typename AVector::memory_traits > AView;
  typedef Kokkos::View< typename BVector::data_type, typename BVector::array_layout, typename BVector::execution_space, typename BVector::memory_traits > BView;

  MV_ElementWiseMultiplyFunctor<CView,AView,BView> op(c,C,ab,A,B,C.dimension_1()) ;
  Kokkos::parallel_for( C.dimension_0() , op );
}

// Rank-2 vector multiply with Sacado::UQ::PCE scalar type, non-constant c, ab
template <typename CS, typename CL, typename CD, typename CM,
          typename AS, typename AL, typename AD, typename AM,
          typename BS, typename BL, typename BD, typename BM>
void
mult(
  const Sacado::UQ::PCE<CS>& c,
  const Kokkos::View< Sacado::UQ::PCE<CS>**, CL, CD, CM >& C,
  const Sacado::UQ::PCE<AS>& ab,
  const Kokkos::View< Sacado::UQ::PCE<AS>*,  AL, AD, AM >& A,
  const Kokkos::View< Sacado::UQ::PCE<BS>**, BL, BD, BM >& B )
{
  if (Sacado::is_constant(c) && Sacado::is_constant(ab)) {
    return mult( c.fastAccessCoeff(0), C,
                                   ab.fastAccessCoeff(0), A, B );
  }
  else {
    Kokkos::Impl::raise_error(
      "MV_ElementWiseMultiply not implemented for non-constant c or ab");
  }
}

// Rank-1 vector scale with Sacado::UQ::PCE scalar type, constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM>
void
scal( const Kokkos::View< Sacado::UQ::PCE<RS>*, RL, RD, RM >& r,
             const typename Sacado::UQ::PCE<XS>::value_type& a,
             const Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM >& x )
{
  typedef Kokkos::View< Sacado::UQ::PCE<RS>*, RL, RD, RM > RVector;
  typedef Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM > XVector;

  typename RVector::flat_array_type r_flat = r;
  typename XVector::flat_array_type x_flat = x;

  scal( r_flat, a, x_flat );
}

// Rank-1 vector scale with Sacado::UQ::PCE scalar type, non-constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM>
void
scal( const Kokkos::View< Sacado::UQ::PCE<RS>*, RL, RD, RM >& r,
             const Sacado::UQ::PCE<XS>& a,
             const Kokkos::View< Sacado::UQ::PCE<XS>*, XL, XD, XM >& x )
{
  if (Sacado::is_constant(a)) {
   return scal( r, a.fastAccessCoeff(0), x );
  }
  else {
    Kokkos::Impl::raise_error("V_MulScalar not implemented for non-constant a");
  }
}

// Rank-2 vector scale with Sacado::UQ::PCE scalar type, constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM>
void
scal( const Kokkos::View< Sacado::UQ::PCE<RS>**, RL, RD, RM >& r,
              const typename Sacado::UQ::PCE<XS>::value_type& a,
              const Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM >& x )
{
  typedef Kokkos::View< Sacado::UQ::PCE<RS>**, RL, RD, RM > RVector;
  typedef Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM > XVector;

  typename RVector::flat_array_type r_flat = r;
  typename XVector::flat_array_type x_flat = x;

  scal( r_flat, a, x_flat );
}

// Rank-2 vector scale with Sacado::UQ::PCE scalar type, non-constant a, b
template <typename RS, typename RL, typename RD, typename RM,
          typename XS, typename XL, typename XD, typename XM>
void
scal( const Kokkos::View< Sacado::UQ::PCE<RS>**, RL, RD, RM >& r,
              const Sacado::UQ::PCE<XS>& a,
              const Kokkos::View< Sacado::UQ::PCE<XS>**, XL, XD, XM >& x )
{
  if (Sacado::is_constant(a)) {
    return scal( r, a.fastAccessCoeff(0), x );
  }
  else {
    Kokkos::Impl::raise_error("MV_MulScalar not implemented for non-constant a or b");
  }
}

template <typename T>
struct V_ReciprocalThresholdSelfFunctor;

template <typename T, typename D, typename M>
struct V_ReciprocalThresholdSelfFunctor<
  Kokkos::View< T,Kokkos::LayoutLeft,D,M,Kokkos::Impl::ViewPCEContiguous > >
{
  typedef Kokkos::View< T,Kokkos::LayoutLeft,D,M,Kokkos::Impl::ViewPCEContiguous > XVector;
  typedef typename XVector::array_type array_type;

  typedef typename array_type::execution_space           execution_space;
  typedef typename array_type::size_type               size_type;
  typedef typename array_type::non_const_value_type   value_type;
  typedef Kokkos::Details::ArithTraits<value_type>           KAT;
  typedef typename KAT::mag_type                        mag_type;

  const array_type m_x;
  const value_type m_min_val;
  const value_type m_min_val_mag;
  const size_type  m_n_pce;

  V_ReciprocalThresholdSelfFunctor(
    const XVector& x,
    const typename XVector::non_const_value_type& min_val) :
    m_x(x),
    m_min_val(min_val.fastAccessCoeff(0)),
    m_min_val_mag(KAT::abs(m_min_val)),
    m_n_pce(x.sacado_size()) {}
  //--------------------------------------------------------------------------

  KOKKOS_INLINE_FUNCTION
  void operator()( const size_type i) const
  {
    if (KAT::abs(m_x(0,i)) < m_min_val_mag)
      m_x(0,i) = m_min_val;
    else
      m_x(0,i) = KAT::one() / m_x(0,i);
    for (size_type l=1; l<m_n_pce; ++l)
      m_x(l,i) = KAT::zero();
  }
};

template <typename T>
struct MV_ReciprocalThresholdSelfFunctor;

template <typename T, typename D, typename M>
struct MV_ReciprocalThresholdSelfFunctor<
  Kokkos::View< T,Kokkos::LayoutLeft,D,M,Kokkos::Impl::ViewPCEContiguous > >
{
  typedef Kokkos::View< T,Kokkos::LayoutLeft,D,M,Kokkos::Impl::ViewPCEContiguous > XVector;
  typedef typename XVector::array_type array_type;

  typedef typename array_type::execution_space           execution_space;
  typedef typename array_type::size_type               size_type;
  typedef typename array_type::non_const_value_type   value_type;
  typedef Kokkos::Details::ArithTraits<value_type>           KAT;
  typedef typename KAT::mag_type                        mag_type;

  const array_type m_x;
  const value_type m_min_val;
  const value_type m_min_val_mag;
  const size_type  m_n;
  const size_type  m_n_pce;

  MV_ReciprocalThresholdSelfFunctor(
    const XVector& x,
    const typename XVector::non_const_value_type& min_val,
    const size_type& n) :
    m_x(x),
    m_min_val(min_val.fastAccessCoeff(0)),
    m_min_val_mag(KAT::abs(m_min_val)),
    m_n(n),
    m_n_pce(x.sacado_size()) {}
  //--------------------------------------------------------------------------

  KOKKOS_INLINE_FUNCTION
  void operator()( const size_type i) const
  {
    for (size_type k=0; k<m_n; ++k) {
      if (KAT::abs(m_x(0,i,k)) < m_min_val_mag)
        m_x(0,i,k) = m_min_val;
      else
        m_x(0,i,k) = KAT::one() / m_x(0,i,k);
      for (size_type l=1; l<m_n_pce; ++l)
        m_x(l,i,k) = KAT::zero();
    }
  }
};

} // namespace Kokkos

#endif /* #ifndef KOKKOS_MV_UQ_PCE_HPP */
