// @HEADER
// ***********************************************************************
// 
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

// ModelEvaluator implementing our problem
#include "twoD_diffusion_ME.hpp"

// Epetra communicator
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

// AztecOO solver
#include "AztecOO.h"

// Stokhos Stochastic Galerkin
#include "Stokhos.hpp"

// Timing utilities
#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_TimeMonitor.hpp"

// I/O utilities
#include "EpetraExt_VectorOut.h"

// Krylov methods
enum Krylov_Method { GMRES, CG };
const int num_krylov_method = 2;
const Krylov_Method krylov_method_values[] = { GMRES, CG };
const char *krylov_method_names[] = { "GMRES", "CG" };

// Collocation preconditioning strategies
enum PrecStrategy { MEAN, REUSE, REBUILD };
const int num_prec_strategy = 3;
const PrecStrategy prec_strategy_values[] = { MEAN, REUSE, REBUILD };
const char *prec_strategy_names[] = { "Mean", "Reuse", "Rebuild" };

// Random field types
enum SG_RF { UNIFORM, CC_UNIFORM, RYS, LOGNORMAL };
const int num_sg_rf = 4;
const SG_RF sg_rf_values[] = { UNIFORM, CC_UNIFORM, RYS, LOGNORMAL };
const char *sg_rf_names[] = { "Uniform", "CC-Uniform", "Rys", "Log-Normal" };

// Sparse grid rules
enum SG_RULE { DEFAULT_RULE, CC, GP, GL, GW, GH, GK };
const int num_sg_rule = 7;
const SG_RULE sg_rule_values[] = { DEFAULT_RULE, CC, GP, GL, GW, GH, GK };
const char *sg_rule_names[] = { "Default", "Clenshaw-Curtis", "Gauss-Patterson", "Gauss-Legendre", "Golub-Welsch", "Gauss-Hermite", "Genz-Keister" };

// Sparse grid growth rules
enum SG_GROWTH { DEFAULT_GROWTH, SLOW_LIN, MOD_LIN, 
		 SLOW_EXP, MOD_EXP, FULL_EXP };
const int num_sg_growth = 6;
const SG_GROWTH sg_growth_values[] = { DEFAULT_GROWTH, SLOW_LIN, MOD_LIN, 
				       SLOW_EXP, MOD_EXP, FULL_EXP };
const char *sg_growth_names[] = { "Default", "Slow Linear", "Moderate Linear",
				  "Slow Exponential", "Moderate Exponential", 
				  "Full Exponential" };

int main(int argc, char *argv[]) {

// Initialize MPI
#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
#endif

  // Create a communicator for Epetra objects
  Teuchos::RCP<Epetra_Comm> Comm;
#ifdef HAVE_MPI
  Comm = Teuchos::rcp(new Epetra_MpiComm(MPI_COMM_WORLD));
#else
  Comm = Teuchos::rcp(new Epetra_SerialComm);
#endif
  
  int MyPID = Comm->MyPID();

  try {

    // Setup command line options
    Teuchos::CommandLineProcessor CLP;
    CLP.setDocString(
      "This example runs a stochastic collocation method.\n");

    int n = 32;
    CLP.setOption("num_mesh", &n, "Number of mesh points in each direction");

    SG_RF randField = UNIFORM;
    CLP.setOption("rand_field", &randField, 
		   num_sg_rf, sg_rf_values, sg_rf_names,
		  "Random field type");

    double mean = 0.2;
    CLP.setOption("mean", &mean, "Mean");

    double sigma = 0.1;
    CLP.setOption("std_dev", &sigma, "Standard deviation");

    double weightCut = 1.0;
    CLP.setOption("weight_cut", &weightCut, "Weight cut");

    int num_KL = 2;
    CLP.setOption("num_kl", &num_KL, "Number of KL terms");

    int p = 3;
    CLP.setOption("order", &p, "Polynomial order");

    bool normalize_basis = true;
    CLP.setOption("normalize", "unnormalize", &normalize_basis, 
		  "Normalize PC basis");

    Krylov_Method krylov_method = CG;
    CLP.setOption("krylov_method", &krylov_method, 
		  num_krylov_method, krylov_method_values, krylov_method_names, 
		  "Krylov method");

    PrecStrategy precStrategy = MEAN;  
    CLP.setOption("prec_strategy", &precStrategy, 
		  num_prec_strategy, prec_strategy_values, prec_strategy_names,
		  "Preconditioner strategy");

    double tol = 1e-12;
    CLP.setOption("tol", &tol, "Solver tolerance");

    SG_RULE sg_rule = DEFAULT_RULE;
    CLP.setOption("sg_rule", &sg_rule, 
		   num_sg_rule, sg_rule_values, sg_rule_names,
		  "Sparse grid rule");

    SG_GROWTH sg_growth = DEFAULT_GROWTH;
    CLP.setOption("sg_growth", &sg_growth, 
		   num_sg_growth, sg_growth_values, sg_growth_names,
		  "Sparse grid growth rule");

    CLP.parse( argc, argv );

    if (MyPID == 0) {
      std::cout << "Summary of command line options:" << std::endl
		<< "\tnum_mesh        = " << n << std::endl
		<< "\trand_field      = " << sg_rf_names[randField] << std::endl
		<< "\tmean            = " << mean << std::endl
		<< "\tstd_dev         = " << sigma << std::endl
		<< "\tnum_kl          = " << num_KL << std::endl
		<< "\torder           = " << p << std::endl
		<< "\tnormalize_basis = " << normalize_basis << std::endl
		<< "\tkrylov_method   = " << krylov_method_names[krylov_method] 
		<< std::endl
		<< "\tprec_strategy   = " << prec_strategy_names[precStrategy] 
		<< std::endl 
		<< "\ttol             = " << tol << std::endl
		<< "\tsg_rule         = " << sg_rule_names[sg_rule] 
		<< std::endl
		<< "\tsg_growth       = " << sg_growth_names[sg_growth]
		<< std::endl;
    }
    
    bool nonlinear_expansion = false;
    if (randField == LOGNORMAL)
      nonlinear_expansion = true;
    
    {
    TEUCHOS_FUNC_TIME_MONITOR("Total Collocation Calculation Time");

    // Create Stochastic Galerkin basis
    Teuchos::Array< Teuchos::RCP<const Stokhos::OneDOrthogPolyBasis<int,double> > > bases(num_KL); 
    for (int i=0; i<num_KL; i++) {
      Teuchos::RCP<Stokhos::OneDOrthogPolyBasis<int,double> > b;
      if (randField == UNIFORM) {
	int sparse_grid_rule = Pecos::GAUSS_LEGENDRE;
	if (sg_rule == CC)
	  sparse_grid_rule = Pecos::CLENSHAW_CURTIS;
	else if (sg_rule == GP)
	  sparse_grid_rule = Pecos::GAUSS_PATTERSON;
	else if (sg_rule == GW)
	  sparse_grid_rule = Pecos::GOLUB_WELSCH;

	int sparse_grid_growth = Pecos::DEFAULT_GROWTH;
	if (sg_growth == SLOW_LIN)
	  sparse_grid_growth = Pecos::SLOW_LINEAR;
	else if (sg_growth == MOD_LIN)
	  sparse_grid_growth = Pecos::MODERATE_LINEAR;
	else if (sg_growth == SLOW_EXP)
	  sparse_grid_growth = Pecos::SLOW_EXPONENTIAL;
	else if (sg_growth == MOD_EXP)
	  sparse_grid_growth = Pecos::MODERATE_EXPONENTIAL;
	else if (sg_growth == FULL_EXP)
	  sparse_grid_growth = Pecos::FULL_EXPONENTIAL;

	b = Teuchos::rcp(new Stokhos::LegendreBasis<int,double>(
				  p, normalize_basis));

	b->setSparseGridRule(sparse_grid_rule);
	b->setSparseGridGrowthRule(sparse_grid_growth);
      }
      else if (randField == CC_UNIFORM) {
	b = 
	  Teuchos::rcp(new Stokhos::ClenshawCurtisLegendreBasis<int,double>(
			 p, normalize_basis, true));
      }
      else if (randField == RYS) {
	int sparse_grid_growth = Pecos::DEFAULT_GROWTH;
	if (sg_growth == SLOW_LIN)
	  sparse_grid_growth = Pecos::SLOW_LINEAR;
	else if (sg_growth == MOD_LIN)
	  sparse_grid_growth = Pecos::MODERATE_LINEAR;
	else if (sg_growth == SLOW_EXP)
	  sparse_grid_growth = Pecos::SLOW_EXPONENTIAL;

	b = Teuchos::rcp(new Stokhos::RysBasis<int,double>(
				  p, weightCut, normalize_basis));

	b->setSparseGridGrowthRule(sparse_grid_growth);
      }
      else if (randField == LOGNORMAL) {
	int sparse_grid_rule = Pecos::GAUSS_HERMITE;
	if (sg_rule == GK)
	  sparse_grid_rule = Pecos::GENZ_KEISTER;
	else if (sg_rule == GW)
	  sparse_grid_rule = Pecos::GOLUB_WELSCH;

	int sparse_grid_growth = Pecos::DEFAULT_GROWTH;
	if (sg_growth == SLOW_LIN)
	  sparse_grid_growth = Pecos::SLOW_LINEAR;
	else if (sg_growth == MOD_LIN)
	  sparse_grid_growth = Pecos::MODERATE_LINEAR;
	else if (sg_growth == SLOW_EXP)
	  sparse_grid_growth = Pecos::SLOW_EXPONENTIAL;
	else if (sg_growth == MOD_EXP)
	  sparse_grid_growth = Pecos::MODERATE_EXPONENTIAL;
	else if (sg_growth == FULL_EXP)
	  sparse_grid_growth = Pecos::FULL_EXPONENTIAL;

	b = Teuchos::rcp(new Stokhos::HermiteBasis<int,double>(
				  p, normalize_basis));

	b->setSparseGridRule(sparse_grid_rule);
	b->setSparseGridGrowthRule(sparse_grid_growth);
      }
      bases[i] = b;
    }
    Teuchos::RCP<const Stokhos::CompletePolynomialBasis<int,double> > basis = 
      Teuchos::rcp(new Stokhos::CompletePolynomialBasis<int,double>(bases));

    // Create sparse grid
    Stokhos::SparseGridQuadrature<int,double> quad(basis,p);
    const Teuchos::Array< Teuchos::Array<double> >& quad_points = 
      quad.getQuadPoints();
    const Teuchos::Array<double>& quad_weights = 
      quad.getQuadWeights();
    int nqp = quad_weights.size();

    // Create application
    twoD_diffusion_ME model(Comm, n, num_KL, sigma, mean, 
    			    basis, nonlinear_expansion);

    // Model data
    Teuchos::RCP<Epetra_Vector> p = 
      Teuchos::rcp(new Epetra_Vector(*(model.get_p_map(0))));
    Teuchos::RCP<Epetra_Vector> x = 
      Teuchos::rcp(new Epetra_Vector(*(model.get_x_map())));
    Teuchos::RCP<Epetra_Vector> f = 
      Teuchos::rcp(new Epetra_Vector(*(model.get_f_map())));
    Teuchos::RCP<Epetra_Vector> g = 
      Teuchos::rcp(new Epetra_Vector(*(model.get_g_map(0))));
    Teuchos::RCP<Epetra_CrsMatrix> A = 
      Teuchos::rcp_dynamic_cast<Epetra_CrsMatrix>(model.create_W());
    EpetraExt::ModelEvaluator::InArgs inArgs = model.createInArgs();
    EpetraExt::ModelEvaluator::OutArgs outArgs = model.createOutArgs();
    EpetraExt::ModelEvaluator::OutArgs outArgs2 = model.createOutArgs();

    // Data to compute/store mean & variance
    Epetra_Vector x2(*(model.get_x_map()));
    Epetra_Vector x_mean(*(model.get_x_map()));
    Epetra_Vector x_var(*(model.get_x_map()));
    Epetra_Vector g2(*(model.get_g_map(0)));
    Epetra_Vector g_mean(*(model.get_g_map(0)));
    Epetra_Vector g_var(*(model.get_g_map(0)));

    // Setup preconditioner
    Teuchos::ParameterList precParams;
    precParams.set("default values", "SA");
    precParams.set("ML output", 0);
    precParams.set("max levels",5);
    precParams.set("increasing or decreasing","increasing");
    precParams.set("aggregation: type", "Uncoupled");
    precParams.set("smoother: type","ML symmetric Gauss-Seidel");
    precParams.set("smoother: sweeps",2);
    precParams.set("smoother: pre or post", "both");
    precParams.set("coarse: max size", 200);
#ifdef HAVE_ML_AMESOS
    precParams.set("coarse: type","Amesos-KLU");
#else
    precParams.set("coarse: type","Jacobi");
#endif
    bool checkFiltering = false;
    if (precStrategy == REUSE) {
      checkFiltering = true;
      precParams.set("reuse: enable", true);
    }
    Teuchos::RCP<ML_Epetra::MultiLevelPreconditioner> M =
      Teuchos::rcp(new ML_Epetra::MultiLevelPreconditioner(*A, precParams, 
							   false));
    if (precStrategy == MEAN) {
      TEUCHOS_FUNC_TIME_MONITOR("Deterministic Preconditioner Calculation");
      *A = *(model.get_mean());
      M->ComputePreconditioner();
    }

    // Setup AztecOO solver
    AztecOO aztec;
    if (krylov_method == GMRES)
      aztec.SetAztecOption(AZ_solver, AZ_gmres);
    else if (krylov_method == CG)
      aztec.SetAztecOption(AZ_solver, AZ_cg);
    aztec.SetAztecOption(AZ_precond, AZ_none);
    aztec.SetAztecOption(AZ_kspace, 100);
    aztec.SetAztecOption(AZ_conv, AZ_r0);
    aztec.SetAztecOption(AZ_output, 0);
    aztec.SetUserOperator(A.get());
    aztec.SetPrecOperator(M.get());
    aztec.SetLHS(x.get());
    aztec.SetRHS(f.get());
    
    x_mean.PutScalar(0.0);
    x_var.PutScalar(0.0);
    // Loop over colloction points
    for (int qp=0; qp<nqp; qp++) {
      TEUCHOS_FUNC_TIME_MONITOR("Collocation Loop");

      if (qp%100 == 0 || qp == nqp-1) 
	std::cout << "Collocation point " << qp+1 <<'/' << nqp << "\n";
      
      // Set parameters
      for (int i=0; i<num_KL; i++)
	(*p)[i] = quad_points[qp][i];

      // Set in/out args
      inArgs.set_p(0, p);
      inArgs.set_x(x);
      outArgs.set_f(f);
      outArgs.set_W(A);

      // Evaluate model at collocation point
      x->PutScalar(0.0);
      model.evalModel(inArgs, outArgs);
      f->Scale(-1.0);

      // Compute preconditioner
      if (precStrategy != MEAN) {
	TEUCHOS_FUNC_TIME_MONITOR("Deterministic Preconditioner Calculation");
	M->ComputePreconditioner(checkFiltering);
      }

      // Solve linear system
      {
	TEUCHOS_FUNC_TIME_MONITOR("Deterministic Solve");
	aztec.Iterate(1000, tol);
      }

      // Compute responses
      outArgs2.set_g(0, g);
      model.evalModel(inArgs, outArgs2);

      // Sum contributions to mean and variance
      x2.Multiply(1.0, *x, *x, 0.0);
      g2.Multiply(1.0, *g, *g, 0.0);
      x_mean.Update(quad_weights[qp], *x, 1.0);
      x_var.Update(quad_weights[qp], x2, 1.0);
      g_mean.Update(quad_weights[qp], *g, 1.0);
      g_var.Update(quad_weights[qp], g2, 1.0);
      
    }
    x2.Multiply(1.0, x_mean, x_mean, 0.0);
    g2.Multiply(1.0, g_mean, g_mean, 0.0);
    x_var.Update(-1.0, x2, 1.0);
    g_var.Update(-1.0, g2, 1.0);

    // Compute standard deviations
    for (int i=0; i<x_var.MyLength(); i++)
      x_var[i] = std::sqrt(x_var[i]);
    for (int i=0; i<g_var.MyLength(); i++)
      g_var[i] = std::sqrt(g_var[i]);

    std::cout.precision(16);
    std::cout << "\nResponse Mean =      " << std::endl << g_mean << std::endl;
    std::cout << "Response Std. Dev. = " << std::endl << g_var << std::endl;
    
    // Save mean and variance to file
    EpetraExt::VectorToMatrixMarketFile("mean_col.mm", x_mean);
    EpetraExt::VectorToMatrixMarketFile("std_dev_col.mm", x_var);

    }
    
    Teuchos::TimeMonitor::summarize(std::cout);
    Teuchos::TimeMonitor::zeroOutTimers();

  }
  
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  catch (string& s) {
    std::cout << s << std::endl;
  }
  catch (char *s) {
    std::cout << s << std::endl;
  }
  catch (...) {
    std::cout << "Caught unknown exception!" <<std:: endl;
  }

#ifdef HAVE_MPI
  MPI_Finalize() ;
#endif

}
