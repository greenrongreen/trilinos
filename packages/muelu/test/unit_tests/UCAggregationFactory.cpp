#include "Teuchos_UnitTestHarness.hpp"
#include "MueLu_TestHelpers.hpp"
#include "MueLu_Version.hpp"

#include "MueLu_UCAggregationFactory.hpp"

namespace {

  using std::string; //?? TODO

  using Teuchos::rcp;
  using Teuchos::RCP;
  using Teuchos::ArrayRCP;

  //TODO: should go in the Aggregates class
  template <class LocalOrdinal, 
            class GlobalOrdinal,
            class Node,         
            class LocalMatOps>
  void printAggregates(MueLu::Aggregates<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>& aggregates, Teuchos::FancyOStream& out) {
#include "MueLu_UseShortNamesOrdinal.hpp"
    RCP<LOVector> Final_ = LOVectorFactory::Build( aggregates.GetVertex2AggId()->getMap() );
    
    ArrayRCP<LO> Final = Final_->getDataNonConst(0);
    ArrayRCP<const LO> vertex2AggId = aggregates.GetVertex2AggId()->getData(0);
    ArrayRCP<const LO> procWinner   = aggregates.GetProcWinner()->getData(0);
    
    for (size_t i=0; i<aggregates.GetVertex2AggId()->getMap()->getNodeNumElements(); i++)
      Final[i] = vertex2AggId[i] + procWinner[i]*1000;
    
    out << *Final_ << std::endl;
  }

  TEUCHOS_UNIT_TEST_TEMPLATE_5_DECL(UCAggregationFactory, Constructor, Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps)
  {
    MUELU_TEST_EPETRA_ONLY_FOR_DOUBLE_AND_INT(Scalar, LocalOrdinal, GlobalOrdinal) {
#include "MueLu_UseShortNames.hpp"
      
      out << "version: " << MueLu::Version() << std::endl;
      MueLu::AggregationOptions aggOptions; //TODO: this parameter should be optional
      RCP<UCAggregationFactory> aggFact = rcp(new UCAggregationFactory(aggOptions));
      TEUCHOS_TEST_EQUALITY(aggFact != Teuchos::null, true, out, success);
    } 
  } // Constructor

  TEUCHOS_UNIT_TEST_TEMPLATE_5_DECL(UCAggregationFactory, Build, Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps)
  {
    MUELU_TEST_EPETRA_ONLY_FOR_DOUBLE_AND_INT(Scalar, LocalOrdinal, GlobalOrdinal) {
      //    typedef double Scalar;
#include "MueLu_UseShortNames.hpp"
      
      out << "version: " << MueLu::Version() << std::endl;

      RCP<Operator> Op = MueLu::TestHelpers::Factory<SC, LO, GO, NO, LMO>::Build1DPoisson(16);
      RCP<Graph> graph = rcp(new Graph(Op->getCrsGraph(), "someGraphLabel"));

      {
        MueLu::AggregationOptions aggOptions;

        //
        aggOptions.SetPrintFlag(6);
        aggOptions.SetMinNodesPerAggregate(2);
        aggOptions.SetMaxNeighAlreadySelected(5);
        aggOptions.SetOrdering(MueLu::AggOptions::NATURAL);
        aggOptions.SetPhase3AggCreation(0.5);

        UCAggregationFactory aggFact(aggOptions);
        RCP<Aggregates> aggregates;

        aggregates = aggFact.Build(*graph);
        printAggregates(*aggregates, out);
      }

      {
        MueLu::AggregationOptions aggOptions;

        //
        aggOptions.SetPrintFlag(6);
        aggOptions.SetMinNodesPerAggregate(2);
        aggOptions.SetMaxNeighAlreadySelected(5);
        aggOptions.SetOrdering(MueLu::AggOptions::RANDOM);
        aggOptions.SetPhase3AggCreation(0.5);

        UCAggregationFactory aggFact(aggOptions);
        RCP<Aggregates> aggregates;

        aggregates = aggFact.Build(*graph);
        printAggregates(*aggregates, out);
      }

      {
        MueLu::AggregationOptions aggOptions;

        //
        aggOptions.SetPrintFlag(6);
        aggOptions.SetMinNodesPerAggregate(2);
        aggOptions.SetMaxNeighAlreadySelected(5);
        aggOptions.SetOrdering(MueLu::AggOptions::GRAPH);
        aggOptions.SetPhase3AggCreation(0.5);

        UCAggregationFactory aggFact(aggOptions);
        RCP<Aggregates> aggregates;

        aggregates = aggFact.Build(*graph);
        printAggregates(*aggregates, out);
      }
    }
  } // Build

    // 
    // INSTANTIATIONS
    //

  typedef double Scalar;                             // Scalar is not relevant for this test
  typedef Kokkos::DefaultNode::DefaultNodeType Node; // Kokkos Node is not relevant for this test   

  typedef long int LongInt;                          // macros dislike parameters with space...
#ifdef HAVE_TEUCHOS_LONG_LONG_INT
  typedef long long int LongLongInt;  
#endif
  
#define UNIT_TEST_GROUP_5(SC, LO, GO, NO, LMO)                          \
  TEUCHOS_UNIT_TEST_TEMPLATE_5_INSTANT(UCAggregationFactory, Constructor, SC, LO, GO, NO, LMO) \
  TEUCHOS_UNIT_TEST_TEMPLATE_5_INSTANT(UCAggregationFactory, Build,       SC, LO, GO, NO, LMO)

#define UNIT_TEST_GROUP_2(LO, GO)                                       \
  typedef Kokkos::DefaultKernels<Scalar,LO,Node>::SparseOps LMO ## LO;  \
  UNIT_TEST_GROUP_5(Scalar, LO, GO, Node, LMO ## LO)

  UNIT_TEST_GROUP_2(int, int)
  UNIT_TEST_GROUP_2(int, LongInt)
  UNIT_TEST_GROUP_2(LongInt, LongInt)

#ifdef HAVE_TEUCHOS_LONG_LONG_INT
  UNIT_TEST_GROUP_2(LongInt, LongLongInt)
#endif

} // namespace <anonymous>
