// @HEADER
// ***********************************************************************
// 
//              Meros: Segregated Preconditioning Package
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

// saddle_pcd_epetra.cpp

// Example program that reads in Epetra matrices from files, creates a
// Meros Pressure Convection-Diffusion (PCD) preconditioner and does a
// solve.
// This version passes the epetra matrices directly to Meros rather
// than making Thyra wrappers first.

#include "Teuchos_ConfigDefs.hpp"
#include "Teuchos_MPISession.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_DefaultComm.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ParameterXMLFileReader.hpp"
#include "Teuchos_RefCountPtr.hpp"

#include "Thyra_SolveSupportTypes.hpp"
#include "Thyra_LinearOpBase.hpp"
#include "Thyra_LinearOpBaseDecl.hpp"
#include "Thyra_VectorDecl.hpp"
#include "Thyra_VectorImpl.hpp" 
#include "Thyra_VectorSpaceImpl.hpp"
#include "Thyra_LinearOperatorDecl.hpp"
#include "Thyra_LinearOperatorImpl.hpp"
#include "Thyra_SpmdVectorBase.hpp"
#include "Thyra_DefaultZeroLinearOp.hpp"
#include "Thyra_DefaultBlockedLinearOpDecl.hpp"
#include "Thyra_LinearOpWithSolveFactoryHelpers.hpp"
#include "Thyra_PreconditionerFactoryHelpers.hpp"
#include "Thyra_DefaultInverseLinearOpDecl.hpp"
#include "Thyra_DefaultInverseLinearOp.hpp"
#include "Thyra_PreconditionerFactoryBase.hpp"
#include "Thyra_DefaultPreconditionerDecl.hpp"
#include "Thyra_DefaultPreconditioner.hpp"

#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#include "mpi.h"
#else
#include "Epetra_SerialComm.h"
#endif

#include "Epetra_Comm.h"
#include "Epetra_Map.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"

#include "EpetraExt_CrsMatrixIn.h"
#include "EpetraExt_VectorIn.h"

#include "Thyra_EpetraLinearOp.hpp"
#include "Thyra_EpetraThyraWrappers.hpp"

#include "AztecOO.h"
#include "Thyra_AztecOOLinearOpWithSolveFactory.hpp"
#include "Thyra_AztecOOLinearOpWithSolve.hpp"

#include "Meros_ConfigDefs.h"
#include "Meros_Version.h"
#include "Meros_PCDPreconditionerFactory.h"
#include "Meros_PCDOperatorSource.h"
#include "Meros_AztecSolveStrategy.hpp"
#include "Meros_InverseOperator.hpp"
#include "Meros_ZeroOperator.hpp"
#include "Meros_IdentityOperator.hpp"
#include "Meros_LinearSolver.hpp"


using namespace Teuchos;
using namespace EpetraExt;
using namespace Thyra;
using namespace Meros;

int main(int argc, char *argv[]) 
{
  GlobalMPISession mpiSession(&argc, &argv);
  
  // DEBUG 0 = no extra tests or printing
  // DEBUG 1 = print out basic diagnostics as we go.
  //           prints outer saddle system iterations, but not inner solves
  // DEBUG > 1 = test usage of some of the operators before proceeding
  //           prints inner and outer iterations
  int DEBUG = 1;

  // Get stream that can print to just root or all streams!
  Teuchos::RefCountPtr<Teuchos::FancyOStream>
    out = Teuchos::VerboseObjectBase::getDefaultOStream();

  //  Epetra_Comm* Comm;
#ifdef HAVE_MPI
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
  const int myRank = Comm.MyPID();
  const int numProcs = Comm.NumProc();
#else
  Epetra_SerialComm Comm;
  const int myRank = 0;
  const int numProcs = 1;
#endif

  if(DEBUG > 0)
    {
      if (myRank == 0)
	{
	  cout << "Proc " << myRank << ": " 
	       << "Number of processors = " 
	       << numProcs << endl;
	  cout << "Proc " << myRank << ": " 
	       << Meros::Meros_Version() << endl;
	}
    }


  try
    {

      /* ------ Read in matrices and rhs ------------------------- */

      // In this example, we provide Fp and Ap (Qp = I).      
      // Using Q1 matrix example; see Meros/examples/data/q1
      // (2,2 block (C block) is zero in this example)
  
      // Make necessary Epetra maps.
      // Need a velocity space map and a pressure space map.
      const Epetra_Map* velocityMap = new Epetra_Map(578, 0, Comm); 
      const Epetra_Map* pressureMap = new Epetra_Map(192, 0, Comm); 
 

      // Read matrix and vector blocks into Epetra_Crs matrices
      Epetra_CrsMatrix* FMatrix(0);      
      char * filename = "../../../../../packages/meros/example/data/q1/Aq1.mm";
      MatrixMarketFileToCrsMatrix(filename,
				  *velocityMap,
				  *velocityMap, *velocityMap,
				  FMatrix);

      // cerr << "If you get Epetra ERROR -1 here, the file was not found"
      //  << "Need to make symbolic link to data directory" 
      //  << endl;

      Epetra_CrsMatrix* BtMatrix(0);
      filename = "../../../../../packages/meros/example/data/q1/Btq1.mm";
      MatrixMarketFileToCrsMatrix(filename,
				  *velocityMap,
				  *velocityMap, *pressureMap,
				  BtMatrix);

      Epetra_CrsMatrix* BMatrix(0);
      filename = "../../../../../packages/meros/example/data/q1/Bq1.mm";
      MatrixMarketFileToCrsMatrix(filename,
				  *pressureMap,
				  *pressureMap, *velocityMap,
				  BMatrix);

      Epetra_CrsMatrix* FpMatrix(0);
      filename = "../../../../../packages/meros/example/data/q1/Q1Fp.mm";
      MatrixMarketFileToCrsMatrix(filename,
				  *pressureMap,
				  *pressureMap, *pressureMap,
				  FpMatrix);

      Epetra_CrsMatrix* ApMatrix(0);
      filename = "../../../../../packages/meros/example/data/q1/Apq1.mm";
      MatrixMarketFileToCrsMatrix(filename,
				  *pressureMap,
				  *pressureMap, *pressureMap,
				  ApMatrix);

      FMatrix->FillComplete();
      FpMatrix->FillComplete();
      ApMatrix->FillComplete();
      BtMatrix->FillComplete(*pressureMap, *velocityMap);
      BMatrix->FillComplete(*velocityMap, *pressureMap);

      // Make an empty C matrix for the 2,2 block
      Epetra_CrsMatrix* CMatrix = new Epetra_CrsMatrix(View, *pressureMap, 1);
      CMatrix->Scale(0.0);
      CMatrix->FillComplete();

      Epetra_Vector* rhsq1_vel(0);
      filename = "../../../../../packages/meros/example/data/q1/rhsq1_vel.mm";
      MatrixMarketFileToVector(filename, *velocityMap, rhsq1_vel);

      Epetra_Vector* rhsq1_press(0);
      filename ="../../../../../packages/meros/example/data/q1/rhsq1_press.mm";
      MatrixMarketFileToVector(filename, *pressureMap, rhsq1_press);


      // Wrap Epetra vectors into Thyra vectors to test the solve
      RefCountPtr<const Thyra::VectorSpaceBase<double> > epetra_vs_press
        = Thyra::create_VectorSpace(rcp(pressureMap,false));
      RefCountPtr<const Thyra::VectorSpaceBase<double> > epetra_vs_vel
        = Thyra::create_VectorSpace(rcp(velocityMap,false));

      RefCountPtr<VectorBase<double> > rhs1
        = create_Vector(rcp(rhsq1_press, false), epetra_vs_press);
      RefCountPtr<VectorBase<double> > rhs2
        = create_Vector(rcp(rhsq1_vel, false), epetra_vs_vel);

      // Convert the vectors to handled vectors
      RefCountPtr<VectorBase<double> > tmp1 = rhs1;
      const Vector<double> rhs_press = tmp1;

      RefCountPtr<VectorBase<double> > tmp2 = rhs2;
      const Vector<double> rhs_vel = tmp2;



      /* -------- Build the Meros preconditioner factory ---------*/

      // Build a PCD (a.k.a. Fp) block preconditioner with Meros
      // 
      // | inv(F) 0 | | I  -Bt | | I        |
      // | 0      I | |     I  | |   -inv(X)|
      // 
      // where inv(X) = Fp inv(Ap)
      // (pressure mass matrix is identity in this example)
      //
      // We'll do this in 4 steps:
      // 1) Build an AztecOO ParameterList for inv(F) solve
      // 2) Build an AztecOO ParameterList for inv(Ap) solve
      //    The Schur complement approximation inverse requires a solve 
      //    on the pressure Poisson operator Ap.
      // 3) Make a PCDOperatorSource with blockOp, Fp, Ap (and Qp if needed)
      // 4) Build the PCD block preconditioner factory 



      // 1) Build an AztecOO ParameterList for inv(F) solve
      //    This one corresponds to (unpreconditioned) GMRES.

      RefCountPtr<ParameterList> aztecFParams 
	= rcp(new ParameterList("aztecOOFSolverFactory"), true);
      
      RefCountPtr<LinearOpWithSolveFactoryBase<double> > aztecFLowsFactory;


      if(DEBUG> 1)
	{
	  // Print out valid parameters and the existing default params.
	  aztecFLowsFactory = rcp(new AztecOOLinearOpWithSolveFactory());
	  cerr << "\naztecFLowsFactory.getValidParameters():\n" << endl;
	  aztecFLowsFactory->getValidParameters()->print(cerr, 0, true, false);
	  cerr << "\nPrinting initial parameters. " << endl;
	  aztecFLowsFactory->setParameterList(aztecFParams);
	  aztecFLowsFactory->getParameterList()->print(cerr, 0, true, false);
	}

      // forward solve settings
      aztecFParams->sublist("Forward Solve").set("Max Iterations", 100);
      aztecFParams->sublist("Forward Solve").set("Tolerance", 10e-8);
      // aztecOO solver settings
      aztecFParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Aztec Solver", "GMRES");
      aztecFParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Aztec Preconditioner", "none");
      aztecFParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Size of Krylov Subspace", 100);

      if(DEBUG > 1)
	{
	  // turn on AztecOO output
	  aztecFParams->sublist("Forward Solve")
	    .sublist("AztecOO Settings").set("Output Frequency", 10);
	}

      if(DEBUG > 1)
        {
	  // Print out the parameters we just set
	  aztecFLowsFactory->setParameterList(aztecFParams);
	  aztecFLowsFactory->getParameterList()->print(cerr, 0, true, false);
        }

      

      // 2) Build an AztecOO ParameterList for inv(Ap) solve
      //    This one corresponds to unpreconditioned CG.

      RefCountPtr<ParameterList> aztecApParams 
	= rcp(new ParameterList("aztecOOApSolverFactory"), true);
      
      // forward solve settings
      aztecApParams->sublist("Forward Solve").set("Max Iterations", 100);
      aztecApParams->sublist("Forward Solve").set("Tolerance", 10e-8);
      // aztecOO solver settings
      aztecApParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Aztec Solver", "CG");
      aztecApParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Aztec Preconditioner", "none");

      if(DEBUG > 1)
	{
	  // turn on AztecOO output
	  aztecApParams->sublist("Forward Solve")
	    .sublist("AztecOO Settings").set("Output Frequency", 10);
	}


      if(DEBUG > 1)
        {
	  // Print out the parameters we just set
	  RefCountPtr<LinearOpWithSolveFactoryBase<double> > 
	    aztecApLowsFactory = rcp(new AztecOOLinearOpWithSolveFactory());
	  aztecApLowsFactory->setParameterList(aztecApParams);
	  aztecApLowsFactory->getParameterList()->print(cerr, 0, true, false);
        }


      // 3) Make a PCDOperatorSource with blockOp, Fp, Ap 
      //    The pressure mass matrix Qp is the identity in this example.
      RefCountPtr<const LinearOpSourceBase<double> > pcdOpSrcRcp 
	= rcp(new PCDOperatorSource(FMatrix, BtMatrix, BMatrix, CMatrix,
				    FpMatrix, ApMatrix));


      // 4) Build the PCD block preconditioner factory.
//       RefCountPtr<PreconditionerFactoryBase<double> > merosPrecFac
// 	= rcp(new PCDPreconditionerFactory(aztecFParams,
// 					   aztecApParams));

      RefCountPtr<PreconditionerFactoryBase<double> > merosPrecFac
        = rcp(
	      new PCDPreconditionerFactory(
	       rcp(new Thyra::AztecOOLinearOpWithSolveFactory(aztecFParams)),
               rcp(new Thyra::AztecOOLinearOpWithSolveFactory(aztecApParams))
	       )
	      );    
      
      RefCountPtr<PreconditionerBase<double> > Prcp 
	= merosPrecFac->createPrec();

      merosPrecFac->initializePrec(pcdOpSrcRcp, &*Prcp);



      /* --- Now build a solver factory for outer saddle point problem --- */

      // Set up parameter list and AztecOO solver
      RefCountPtr<ParameterList> aztecSaddleParams 
	= rcp(new ParameterList("aztecOOSaddleSolverFactory"));

      RefCountPtr<LinearOpWithSolveFactoryBase<double> >
	aztecSaddleLowsFactory = rcp(new AztecOOLinearOpWithSolveFactory());
      
      double saddleTol = 10e-6;

      // forward solve settings
      aztecSaddleParams->sublist("Forward Solve").set("Max Iterations", 500);
      aztecSaddleParams->sublist("Forward Solve").set("Tolerance", saddleTol);
      // aztecOO solver settings
      aztecSaddleParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Aztec Solver", "GMRES");
      aztecSaddleParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Aztec Preconditioner", "none");
      aztecSaddleParams->sublist("Forward Solve")
	.sublist("AztecOO Settings").set("Size of Krylov Subspace", 500);

      if(DEBUG > 0)
	{
	  // turn on AztecOO output
	  aztecSaddleParams->sublist("Forward Solve")
	    .sublist("AztecOO Settings").set("Output Frequency", 1);
	}


      aztecSaddleLowsFactory->setParameterList(aztecSaddleParams);

      if(DEBUG > 0)
	{
	  // Print out the parameters we've set.
	  aztecSaddleLowsFactory->getParameterList()->print(cerr, 0, 
							    true, false);
	}

 
      // Retrieve block LinearOperator from the LSC operator source
      // so we can do the solve
      RefCountPtr<const PCDOperatorSource> pcdOpSrcPtr 
	= rcp_dynamic_cast<const PCDOperatorSource>(pcdOpSrcRcp);        
      ConstLinearOperator<double> blockOp = pcdOpSrcPtr->getSaddleOp();
      

      // Set up the preconditioned inverse object and do the solve!
      RefCountPtr<LinearOpWithSolveBase<double> > rcpAztecSaddle 
	= aztecSaddleLowsFactory->createOp();
      
      // LinearOperator<double> epetraBlockOp = makeEpetraOperator(blockOp);

      // initializePreconditionedOp<double>(*aztecSaddleLowsFactory, 
      //			 epetraBlockOp.ptr(), 
      //				 Prcp,
      //				 &*rcpAztecSaddle );

//       initializePreconditionedOp<double>(*aztecSaddleLowsFactory, 
// 					 blockOp.constPtr(), 
// 					 Prcp,
// 					 &*rcpAztecSaddle );
      
//       RefCountPtr<LinearOpBase<double> > tmpSaddleInv 
// 	= rcp(new DefaultInverseLinearOp<double>(rcpAztecSaddle));
      
//       LinearOperator<double> saddleInv = tmpSaddleInv;
//       saddleInv.description();


      RefCountPtr<const LinearOpBase<double> > tmpPinv 
        = Prcp->getRightPrecOp();
      ConstLinearOperator<double> Pinv = tmpPinv;

      LinearSolveStrategy<double> azSaddle 
        = new AztecSolveStrategy(*(aztecSaddleParams.get()));

      ConstLinearOperator<double> saddleInv 
	= new InverseOperator<double>(blockOp * Pinv, azSaddle);


      // Build a block rhs from the rhs vectors read in from file
      Vector<double> rhs = blockOp.range().createMember();
      rhs.setBlock(0,rhs_vel);
      rhs.setBlock(1,rhs_press);

      // Build a solution vector and initialize it to zero.
      Vector<double> solnblockvec = blockOp.domain().createMember();
      zeroOut(solnblockvec);

      // Do the solve!
      solnblockvec = saddleInv * rhs;

      // Check out results.
      // cerr << "norm of answer soln: " << norm2(solnblockvec) << endl;
      Vector<double> residvec = blockOp * Pinv * solnblockvec - rhs;
      // Vector<double> residvec = blockOp * solnblockvec - rhs;

      cerr << "norm of final resid " << norm2(residvec) << endl;

      double normResvec = norm2(residvec);

      if(normResvec < 10.0*saddleTol)
	{
	  cerr << "Example PASSED!" << endl;
	  return 0;
	}
      else
	{
	  cerr << "Example FAILED!" << endl;
	  return 1;
	}


    } // end of try block


  catch(std::exception& e)
    {
      cerr << "Caught exception: " << e.what() << endl;
    }

  MPISession::finalize();

} // end of main()

