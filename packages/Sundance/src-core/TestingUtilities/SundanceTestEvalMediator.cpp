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


#include "SundanceTestEvalMediator.hpp"
#include "SundanceCoordExpr.hpp"
#include "SundanceCellDiameterExpr.hpp"
#include "SundanceDiscreteFuncElement.hpp"
#include "SundanceFunctionalDeriv.hpp"
#include "SundanceEvalManager.hpp"
#include "SundanceOut.hpp"
#include "SundanceExpr.hpp"
#include "SundanceTabs.hpp"
#include "SundanceExceptions.hpp"
#include "SundanceUnknownFunctionStub.hpp"
#include "SundanceTestFunctionStub.hpp"
#include "SundanceDiscreteFunctionStub.hpp"
#include "SundanceSymbPreprocessor.hpp"
#include "SundanceEvalManager.hpp"

using namespace SundanceCore::Internal;
using namespace SundanceCore::Internal;
using namespace SundanceCore;
using namespace SundanceUtils;
using namespace SundanceTesting;
using namespace TSFExtended;
using namespace Teuchos;
using namespace std;



TestEvalMediator::TestEvalMediator(const Expr& fields)
  : AbstractEvalMediator(),
    x_(),
    funcIdToFieldNumberMap_(),
    fields_(fields.totalSize())
{
  EvalManager::stack().setVecSize(1);

  Expr f = fields.flatten();
  for (int i=0; i<f.size(); i++)
    {
      const DiscreteFuncElement* u0 
        = dynamic_cast<const DiscreteFuncElement*>(f[i].ptr().get());
      TEST_FOR_EXCEPTION(u0 == 0, InternalError,
                         "TestEvalMediator ctor: field argument "
                         << f[i] << " is not a discrete function");
      funcIdToFieldNumberMap_.put(u0->funcID(), i);
      const TestDiscreteFunction* df 
        = dynamic_cast<const TestDiscreteFunction*>(u0->master());
      TEST_FOR_EXCEPTION(df==0, InternalError,
                         "TestEvalMediator ctor: field argument "
                         << f[i] << " is not a TestDiscreteFunction");
      fields_[i] = df->field();
    }
}



void TestEvalMediator::evalCoordExpr(const CoordExpr* expr,
                                     RefCountPtr<EvalVector>& vec) const
{
  Tabs tabs;
  SUNDANCE_OUT(this->verbosity() > VerbSilent,
               tabs << "evaluating coord expr " << expr->toXML().toString());
  
  vec->setString(expr->name());

  int direction = expr->dir();
  
  double * const xx = vec->start();

  xx[0] = x_[direction];

  SUNDANCE_VERB_LOW(tabs << "results: " << *vec);
}

void TestEvalMediator::evalCellDiameterExpr(const CellDiameterExpr* expr,
                                     RefCountPtr<EvalVector>& vec) const
{
  Tabs tabs;
  SUNDANCE_OUT(this->verbosity() > VerbSilent,
               tabs << "evaluating cell diameter expr " << expr->toXML().toString());
  
  vec->setString(expr->name());

  double * const xx = vec->start();

  xx[0] = 1.0;

  SUNDANCE_VERB_LOW(tabs << "results: " << *vec);
}

void TestEvalMediator
::evalDiscreteFuncElement(const DiscreteFuncElement* expr,
                          const Array<MultiIndex>& mi,
                          Array<RefCountPtr<EvalVector> >& vec) const 
{
  static Array<string> coordNames;

  Tabs tabs;
  SUNDANCE_OUT(this->verbosity() > VerbSilent,
               tabs << "evaluating discrete func " << expr->toString() 
               << " with multiindices " << mi);

  if (coordNames.size() != 3)
    {
      coordNames.resize(3);
      coordNames[0] = "x";
      coordNames[1] = "y";
      coordNames[2] = "z";
    }

  string funcName = expr->name();
  
  TEST_FOR_EXCEPTION(!funcIdToFieldNumberMap_.containsKey(expr->funcID()),
                     InternalError, "funcID " << expr->funcID()
                     << " not found in TestEvalMediator funcID to field "
                     "map" << funcIdToFieldNumberMap_);

  int fieldIndex = funcIdToFieldNumberMap_.get(expr->funcID());
  
  for (unsigned int i=0; i<mi.size(); i++)
    {
      if (mi[i].order()==0)
        {
          vec[i]->setString(funcName);
        }
      else
        {
          int dir = mi[i].firstOrderDirection();
          string deriv = "D[" + funcName + ", " + coordNames[dir] + "]";
          vec[i]->setString(deriv);
        }

      double * const xx = vec[i]->start();
      SUNDANCE_VERB_HIGH("coeff=" << fields_[fieldIndex].coeff());
      xx[0] = fields_[fieldIndex].coeff() * evalDummyBasis(fieldIndex, mi[i]);
    }

  if (verbosity() > VerbSilent)
    {
      cerr << tabs << "results:" << endl;
      for (unsigned int i=0; i<mi.size(); i++)
        {
          Tabs tab1;
          cerr << tab1 << "mi=" << mi[i].toString() 
               << *vec[i] << endl;
        }
    }
}

double TestEvalMediator::evalDummyBasis(int m, const MultiIndex& mi) const
{
  TEST_FOR_EXCEPTION(mi.order() > 1, RuntimeError, 
                     "TestEvalMediator::evalDummyBasis found multiindex "
                     "order > 1. The bad multiindex was " << mi.toString());

  ADReal result = fields_[m].basis().evaluate(ADField::evalPoint());
  SUNDANCE_VERB_HIGH("basis.value() " << result.value());
  SUNDANCE_VERB_HIGH("basis.gradient() " << result.gradient());

  if (mi.order()==0)
    {
      return result.value();
    }
  else 
    {
      return result.gradient()[mi.firstOrderDirection()];
    }
}


