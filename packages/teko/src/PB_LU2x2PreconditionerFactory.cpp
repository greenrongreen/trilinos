#include "PB_LU2x2PreconditionerFactory.hpp"

// PB includes
#include "PB_LU2x2InverseOp.hpp"
#include "PB_BlockUpperTriInverseOp.hpp"

// default strategies
#include "PB_LU2x2DiagonalStrategy.hpp"

using Teuchos::rcp;
using Teuchos::RCP;

namespace PB {

// construct a PreconditionerFactory
LU2x2PreconditionerFactory::LU2x2PreconditionerFactory(LinearOp & invA00, LinearOp & invS)
      : invOpsStrategy_(rcp(new StaticLU2x2Strategy(invA00,invA00,invS))), useFullLDU_(true)
{ }

/** @brief Build a simple static LU2x2 preconditioner */
LU2x2PreconditionerFactory::LU2x2PreconditionerFactory(LinearOp & hatInvA00,LinearOp & tildeInvA00,LinearOp & invS)
      : invOpsStrategy_(rcp(new StaticLU2x2Strategy(hatInvA00,tildeInvA00,invS))), useFullLDU_(true)
{ }

LU2x2PreconditionerFactory::LU2x2PreconditionerFactory(const RCP<const LU2x2Strategy> & strategy)
   : invOpsStrategy_(strategy), useFullLDU_(true)
{ }

LU2x2PreconditionerFactory::LU2x2PreconditionerFactory()
   : invOpsStrategy_(Teuchos::null), useFullLDU_(true)
{ }

// for PreconditionerFactoryBase
///////////////////////////////////////////////////////////////////////

// initialize a newly created preconditioner object
LinearOp LU2x2PreconditionerFactory::buildPreconditionerOperator(BlockedLinearOp & A,BlockPreconditionerState & state) const
{
   PB_DEBUG_SCOPE("LU2x2PreconditionerFactory::buildPreconditionerOperator",10);
   LinearOp hatInvA00   = invOpsStrategy_->getHatInvA00(A,state);
   LinearOp tildeInvA00 = invOpsStrategy_->getTildeInvA00(A,state);
   LinearOp invS        = invOpsStrategy_->getInvS(A,state);

   // build the SchurSolve LinearOp
   if(useFullLDU())
      return createLU2x2InverseOp(A,hatInvA00,tildeInvA00,invS,"LU2x2-Full");
   else  {
      std::vector<LinearOp> invDiag(2);
      invDiag[0] = hatInvA00;
      invDiag[1] = scale(-1.0,invS);
      return createBlockUpperTriInverseOp(A,invDiag,"LU2x2-Upper");
   }
}

/** \brief This function builds the internals of the preconditioner factory
  *        from a parameter list.
  *        
  * This function builds the internals of the preconditioner factory
  * from a parameter list. Furthermore, it allows a preconditioner factory
  * developer to easily add a factory to the build system. This function
  * is required for building a preconditioner from a parameter list.
  *
  * \param[in] settings Parmaeter list to use as the internal settings
  *
  * \note The default implementation does nothing.
  */
void LU2x2PreconditionerFactory::initializeFromParameterList(const Teuchos::ParameterList & settings)
{
   PB_DEBUG_SCOPE("LU2x2PreconditionerFactory::initializeFromParameterList",10);

   // use Golub & Wathen type or full LDU decomposition for inverse solve?
   bool useLDU = true; 
   if(settings.isParameter("Use LDU"))
      useLDU = settings.get<bool>("Use LDU");
   setFullLDU(useLDU);
   
   // build strategy object
   std::string stratName = settings.get<std::string>("Strategy Name");
   const Teuchos::ParameterList & pl = settings.sublist("Strategy Settings");
   invOpsStrategy_ = buildStrategy(stratName,pl,getInverseLibrary());
}

/////////////////////////////////////////////////////
// Static members and methods
/////////////////////////////////////////////////////

//! for creating the preconditioner factories objects
CloneFactory<LU2x2Strategy> LU2x2PreconditionerFactory::strategyBuilder_;

/** \brief Builder function for creating strategies.
  *
  * Builder function for creating strategies.
  * 
  * \param[in] name     String name of strategy to build
  * \param[in] settings Parameter list describing the parameters for the
  *                     strategy to build
  * \param[in] invLib   Inverse library for the strategy to use.
  *
  * \returns If the name is associated with a strategy
  *          a pointer is returned, otherwise Teuchos::null is returned.
  */
RCP<LU2x2Strategy> LU2x2PreconditionerFactory::buildStrategy(const std::string & name, 
                                                             const Teuchos::ParameterList & settings,
                                                             const RCP<const InverseLibrary> & invLib)
{
   PB_DEBUG_SCOPE("LU2x2PreconditionerFactory::buildStrategy",10);

   // initialize the defaults if necessary
   if(strategyBuilder_.cloneCount()==0) initializeStrategyBuilder();

   // request the preconditioner factory from the CloneFactory
   RCP<LU2x2Strategy> strategy = strategyBuilder_.build(name);

   if(strategy==Teuchos::null) {
      PB_DEBUG_MSG("Warning: Could not build LU2x2Strategy named \"" 
                 << name << "\"...pressing on, failure expected",0)
      return Teuchos::null;
   }

   // now that inverse library has been set,
   // pass in the parameter list
   strategy->initializeFromParameterList(settings,*invLib);

   return strategy;
}

/** \brief Add a strategy to the builder. This is done using the
  *        clone pattern. 
  *
  * Add a strategy to the builder. This is done using the
  * clone pattern. If your class does not support the Cloneable interface then
  * you can use the AutoClone class to construct your object.
  *
  * \note If this method is called twice with the same string, the latter clone pointer
  *       will be used.
  *
  * \param[in] name String to associate with this object
  * \param[in] clone Pointer to Cloneable object
  */
void LU2x2PreconditionerFactory::addStrategy(const std::string & name,const RCP<Cloneable> & clone)
{
   PB_DEBUG_SCOPE("LU2x2PreconditionerFactory::addStrategy",10);

   // initialize the defaults if necessary
   if(strategyBuilder_.cloneCount()==0) initializeStrategyBuilder();

   // add clone to builder
   strategyBuilder_.addClone(name,clone); 
}

//! This is where the default objects are put into the strategyBuilder_
void LU2x2PreconditionerFactory::initializeStrategyBuilder()
{
   PB_DEBUG_SCOPE("LU2x2PreconditionerFactory::initializeStrategyBuilder",10);

   RCP<Cloneable> clone;

   // add various strategies to the factory
   clone = rcp(new AutoClone<LU2x2DiagonalStrategy>());
   strategyBuilder_.addClone("Diagonal Strategy",clone);
}

} // end namespace PB