/*
// @HEADER
// ***********************************************************************
// 
//                Amesos: Direct Sparse Solver Package
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
// ***********************************************************************
// @HEADER
*/

#ifndef THYRA_AMESOS_LINEAR_OP_WITH_SOLVE_FACTORY_HPP
#define THYRA_AMESOS_LINEAR_OP_WITH_SOLVE_FACTORY_HPP

#include "Thyra_LinearOpWithSolveFactoryBase.hpp"
#include "Thyra_AmesosTypes.hpp"
#include "Amesos_BaseSolver.h"
#include "Teuchos_StandardMemberCompositionMacros.hpp"
#include "Teuchos_StandardCompositionMacros.hpp"

namespace Thyra {

/** \brief Concrete <tt>LinearOpWithSolveFactoryBase</tt> adapter subclass that uses
 * Amesos direct solvers.
 * 
 */
class AmesosLinearOpWithSolveFactory : public LinearOpWithSolveFactoryBase<double> {
public:

  /** @name Public types */
  //@{


  //@}

  /** @name Constructors/initializers/accessors */
  //@{

  /** \brief Constructor which sets the defaults.
   */
  AmesosLinearOpWithSolveFactory(
    const Amesos::ESolverType                solverType
#ifdef HAVE_AMESOS_KLU
                                                                    = Amesos::KLU
#else
                                                                    = Amesos::LAPACK
#endif
    ,const Amesos::ERefactorizationPolicy    refactorizationPolicy  = Amesos::REPIVOT_ON_REFACTORIZATION
    );

  /** \brief Set the type of solver to use.
   *
   * Note, do not change the solver type between refactorizations of the same
   * matrix (i.e. in calls to <tt>this->initializeOp()</tt>)!  Doing so will
   * have undefined behavior.
   */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( Amesos::ESolverType, solverType )

  /** \brief Set the refactorization policy.
   *
   * This option can be changed inbetween refactorizations (i.e. between calls
   * to <tt>this->initializeOp()</tt>).
   */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( Amesos::ERefactorizationPolicy, refactorizationPolicy )

  /** \brief Set the parameter list that will be used to control the solver.
   *
   * See Amesos documentation for what these options are.
   *
   * <b>Developers note:</b> From what I can tell of the documenatation and
   * looking at some source code, this parameter list will not be modified but
   * its use is very confused. I think that this should be changed to a
   * constant parameter list but I don't want to do this because I don't want
   * to be held accountable in case it is changed.  If users needs to
   * guarantee that their parameter list will not change, then a copy should
   * be made before calling this function.
   */
  STANDARD_NONCONST_COMPOSITION_MEMBERS( Teuchos::ParameterList, paramList )

  //@}

  /** @name Overridden from LinearOpWithSolveFactoryBase */
  //@{

  /** \brief Returns true if <tt>dynamic_cast<const EpetraLinearOpBase*>(fwdOp)!=NULL</tt> . */
  bool isCompatible( const LinearOpBase<double> &fwdOp ) const;

  /** \brief . */
  Teuchos::RefCountPtr<LinearOpWithSolveBase<double> > createOp() const;

  /** \brief . */
  void initializeOp(
    const Teuchos::RefCountPtr<const LinearOpBase<double> >    &fwdOp
    ,LinearOpWithSolveBase<double>                             *Op
    ) const;

  /** \brief . */
  void uninitializeOp(
    LinearOpWithSolveBase<double>                       *Op
    ,Teuchos::RefCountPtr<const LinearOpBase<double> >  *fwdOp
    ) const;

  //@}

};

//@}

} // namespace Thyra

#endif // THYRA_AMESOS_LINEAR_OP_WITH_SOLVE_FACTORY_HPP
