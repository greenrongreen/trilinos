#ifndef MUELU_DEFAULTFACTORYHANDLER_HPP
#define MUELU_DEFAULTFACTORYHANDLER_HPP

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Exceptions.hpp"

#include "MueLu_DefaultFactoryHandlerBase.hpp"

// Default factories:
#include "MueLu_UCAggregationFactory.hpp"
#include "MueLu_CoalesceDropFactory.hpp"

// used as default:
#include "MueLu_GenericPRFactory.hpp"
#include "MueLu_SaPFactory.hpp"
//#include "MueLu_RAPFactory.hpp"
#include "MueLu_ReUseFactory.hpp"
#include "MueLu_NullspaceFactory.hpp"
#include "MueLu_TransPFactory.hpp"
#include "MueLu_SmootherFactory.hpp"

namespace MueLu {

  //! Class that provides default factories within Needs class.
  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class DefaultFactoryHandler : public DefaultFactoryHandlerBase {
#include "MueLu_UseShortNames.hpp"

  public:
    //@{ Constructors/Destructors.

    //! Constructor.
    DefaultFactoryHandler() {}

    //! Destructor.
    virtual ~DefaultFactoryHandler() { }

    //@}
    
    //@{ Get/Set functions.

    virtual const FactoryBase & GetDefaultFactory(const std::string & varName) {
      if (! DefaultFactoryHandlerBase::IsAvailable(varName)) {

        if (varName == "P")           return SetAndReturnDefaultFactory(varName, rcp(new GenericPRFactory(rcp(new SaPFactory()))));
        if (varName == "R")           return SetAndReturnDefaultFactory(varName, rcp(new TransPFactory()));
        // (varName == "A")           return SetAndReturnDefaultFactory(varName, rcp(new RAPFactory));
        if (varName == "A")           return SetAndReturnDefaultFactory(varName, rcp(new ReUseFactory()));
        if (varName == "Nullspace")   return SetAndReturnDefaultFactory(varName, rcp(new NullspaceFactory()));
        if (varName == "Graph")       return SetAndReturnDefaultFactory(varName, rcp(new CoalesceDropFactory()));
        if (varName == "Aggregates")  return SetAndReturnDefaultFactory(varName, rcp(new UCAggregationFactory()));
        if (varName == "PreSmoother") return SetAndReturnDefaultFactory(varName, rcp(new SmootherFactory()));

        TEST_FOR_EXCEPTION(1, MueLu::Exceptions::RuntimeError, "DefaultFactoryHandler::GetDefaultFactory(): No default factory available for building '"+varName+"'.");

      }

      return DefaultFactoryHandlerBase::GetDefaultFactory(varName);
    }

    //TODO GetDefaultObject, for Smoothers

    //@}
    
  private:

    //! helper
    const FactoryBase & SetAndReturnDefaultFactory(const std::string & varName, const RCP<FactoryBase> factory) {
      DefaultFactoryHandlerBase::SetDefaultFactory(varName, factory);
      return DefaultFactoryHandlerBase::GetDefaultFactory(varName); //return factory;
    }

  }; // class DefaultFactoryHandler

} // namespace MueLu

#define MUELU_DEFAULTFACTORYHANDLER_SHORT
#endif //ifndef MUELU_DEFAULTFACTORYHANDLER_HPP
