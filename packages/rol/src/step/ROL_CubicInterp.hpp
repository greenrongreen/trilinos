// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

#ifndef ROL_CUBICINTERP_H
#define ROL_CUBICINTERP_H

/** \class ROL::CubicInterp
    \brief Implements cubic interpolation back tracking line search.
*/

#include "ROL_LineSearch.hpp"

namespace ROL { 

template<class Real>
class CubicInterp : public LineSearch<Real> {
private:
  Real rho_;
  Teuchos::RCP<Vector<Real> > xnew_; 

public:

  virtual ~CubicInterp() {}

  // Constructor
  CubicInterp( Teuchos::ParameterList &parlist ) : LineSearch<Real>(parlist) { 
    rho_ = parlist.get("Backtracking Rate",0.5);
  }

  void initialize( const Vector<Real> &x, const Vector<Real> &g,
                   Objective<Real> &obj, BoundConstraint<Real> &con ) {
    LineSearch<Real>::initialize(x,g,obj,con);
    xnew_ = x.clone();
  }

  void run( Real &alpha, Real &fval, int &ls_neval, int &ls_ngrad,
            const Real &gs, const Vector<Real> &s, const Vector<Real> &x, 
            Objective<Real> &obj, BoundConstraint<Real> &con ) {
    Real tol = std::sqrt(ROL_EPSILON);
    ls_neval = 0;
    ls_ngrad = 0;
    // Get initial line search parameter
    alpha = LineSearch<Real>::getInitialAlpha(ls_neval,ls_ngrad,fval,gs,x,s,obj,con);
    // Update iterate
    LineSearch<Real>::updateIterate(*xnew_,x,s,alpha,con);
    // Get objective value at xnew
    Real fold = fval;
    obj.update(*xnew_);
    fval = obj.value(*xnew_,tol);
    ls_neval++;
    // Initialize
    Real fvalp  = 0.0;
    Real alpha1 = 0.0;
    Real alpha2 = 0.0;
    Real a      = 0.0;
    Real b      = 0.0;
    Real x1     = 0.0;
    Real x2     = 0.0;
    bool first_iter = true;
    // Perform cubic interpolation back tracking
    while ( !LineSearch<Real>::status(LINESEARCH_CUBICINTERP,ls_neval,ls_ngrad,alpha,fold,gs,fval,x,s,obj,con) ) {
      if ( first_iter ) { // Minimize quadratic interpolate
        alpha1 = -gs*alpha*alpha/(2.0*(fval-fold-gs*alpha));
        first_iter = false;
      }
      else {              // Minimize cubic interpolate
        x1 = fval-fold-alpha*gs;
        x2 = fvalp-fval-alpha2*gs;
        a = (1.0/(alpha - alpha2))*( x1/(alpha*alpha) - x2/(alpha2*alpha2));
        b = (1.0/(alpha - alpha2))*(-x1*alpha2/(alpha*alpha) + x2*alpha/(alpha2*alpha2));
        if ( std::abs(a) < ROL_EPSILON ) {
          alpha1 = -gs/(2.0*b);
        }
        else {
          alpha1 = (-b+sqrt(b*b-3.0*a*gs))/(3.0*a);
        }
        if ( alpha1 > 0.5*alpha ) {
          alpha1 = 0.5*alpha;
        }
      }
      alpha2 = alpha;
      fvalp  = fval;
      // Back track if necessary
      if ( alpha1 <= 0.1*alpha ) {
        alpha *= 0.1;
      }
      else if ( alpha1 >= 0.5*alpha ) {
        alpha *= 0.5;
      }
      else {
        alpha = alpha1;
      }
      // Update iterate
      LineSearch<Real>::updateIterate(*xnew_,x,s,alpha,con);
      // Get objective value at xnew
      obj.update(*xnew_);
      fval = obj.value(*xnew_,tol);
      ls_neval++;
    }
  }
};

}

#endif
