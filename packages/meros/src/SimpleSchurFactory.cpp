  #include "SimpleSchurFactory.h"
  #include "SchurFactory.h"
  #include "SimpleSchurFactoryBase.h"
  #include "TSFVectorSpaceBase.h"
  #include "TSFVectorBase.h"
  #include "TSFLinearOperatorBase.h"
  #include "TSFLinearSolverBase.h"
  #include "TSFIdentityOperator.h"
  #include "PetraMatrix.h"
  #include "TSFOperatorSourceBase.h"
  #include "SimpleOperatorSource.h"
  #include "Aztec2TSF.h"

  using namespace SPP;
  using namespace TSF;
  using std::string;

  SimpleSchurFactory::SimpleSchurFactory(TSFLinearSolver& schurSolver)
  : schurSolver_(schurSolver)
  {}

  TSFLinearOperator SimpleSchurFactory
  ::getSchurInvApprox(const TSFOperatorSource& OpSrc) const
  {
  cerr << "In DiagSchurFactor: getSchurInvApprox" << endl;
  TSFLinearOperator Xinv; 
 
  // Cast OpSrc to DiagRightOperatorSource type.
  const SimpleOperatorSource* diagSrcPtr = 
    dynamic_cast<const SimpleOperatorSource*>(OpSrc.ptr());

  // Add check that operator source is really the right type.

  // Get saddle operator 
  TSFLinearOperator S = diagSrcPtr->getOp();

  // Get needed blocks from saddle operator
  TSFLinearOperator Bt = S.getBlock(0,1);
  TSFLinearOperator B = S.getBlock(1,0);
  TSFLinearOperator C = S.getBlock(1,1);

  TSFLinearOperator F = S.getBlock(0,0);

  // Check for C

  // Get negative Dinv operator
  TSFLinearOperator Dinv = diagSrcPtr->getDinv();

  // If using Multigrid,
  // do explicit matrix multiplies and adds to get Schur approx
  // Need explicit operations so we have a matrix for multigrid in the end
  // Could put a check here: if schurSolver is multigrid, use explicit mults
  // otherwise, the regular TSF implicit mult is OK and cheaper

  TSFLinearOperator schurOp;
  //  TSFLinearOperator tempOp1;
  //  TSF_MatrixMult(Dinv, Bt, tempOp1); // tempOp1 = Dinv*Bt
  //  TSF_MatrixMult(B, tempOp1, schurOp); // tempOp2 = B*tempOp1
  

  // If not ML, can use TSF's regular deferred multiplications
  schurOp = B*Dinv*Bt;

  Xinv = schurOp.inverse(schurSolver_);

  return Xinv;

 }

 string SimpleSchurFactory::toString() const
 {
	return "SimpleSchurFactory toString";
 }
