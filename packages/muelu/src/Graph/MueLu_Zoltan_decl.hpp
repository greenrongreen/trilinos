#ifndef MUELU_ZOLTAN_DECL_HPP
#define MUELU_ZOLTAN_DECL_HPP

#include "MueLu_ConfigDefs.hpp"

#if defined(HAVE_MUELU_ZOLTAN) && defined(HAVE_MPI)
#include <Teuchos_DefaultMpiComm.hpp> //TODO: fwd decl.
#include <Teuchos_OpaqueWrapper.hpp>  //TODO: fwd decl.

#include <zoltan_cpp.h>

#include <Xpetra_Operator.hpp>
#include <Xpetra_VectorFactory.hpp>

#include "MueLu_BaseClass.hpp"
#include "MueLu_Zoltan_fwd.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"

namespace MueLu {

  /*!
    @class ZoltanInterface
    @brief Interface to Zoltan library.

    This interface provides access to partitioning methods in Zoltan.
    Currently, it supports the RCB algorithm only.
  */

  template <class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType,
            class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class ZoltanInterface : public BaseClass {

    typedef double Scalar; // FIXME
#undef MUELU_ZOLTAN_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors
    //@{

    //! Constructor
    ZoltanInterface(RCP<const Teuchos::Comm<int> > const &comm, RCP<const FactoryBase> AFact = Teuchos::null);

    //! Destructor
    virtual ~ZoltanInterface() { }
    //@}

    //! @name Input
    //@{
    void DeclareInput(Level & level) const;
    //@}

    //! @name Set / Get methods.
    //@{
    //! Set the number of partitions over which the data should be divided.
    void SetNumberOfPartitions(GO const numPartitions);
    //@}

    //! @name Build methods.
    //@{
    void Build(Level &level);

    //@}

    //! @name Query methods (really functions) required by Zoltan.
    //@{

    /*! Callback function that returns the local number of objects. Required by Zoltan.

    In this case, the number of objects is the number of local rows.
     
    @param data (in) void pointer to an Xpetra::Operator.
    @param ierr (out) error code.
    */
    static int GetLocalNumberOfRows(void *data, int *ierr);

    /*! Callback function that returns the local number of nonzeros in the matrix. Required by Zoltan.

    FIXME: Note that this will not work properly for non-point matrices.

    @param data (in) void pointer to an Xpetra::Operator
    @param weights (out) array whose <tt>i</tt><sup>th</sup> entry is the number of nonzeros in local row \c i.
    @param ierr (out) error code
    */
    static void GetLocalNumberOfNonzeros(void *data, int NumGidEntries, int NumLidEntries, ZOLTAN_ID_PTR gids,
                                         ZOLTAN_ID_PTR lids, int wgtDim, float *weights, int *ierr);

    /*! Callback function that returns the problem dimension. Required by Zoltan.

    @param data (in) void pointer to integer dimension
    @param ierr (out) error code
    */
    static int GetProblemDimension(void *data, int *ierr);


    /*! Callback function that returns the problem dimension. Required by Zoltan.

    @param data (in) void pointer to Xpetra::MultiVector.
    @param coordinates (out) array of double coordinates, arranged like so: [x1 y1 z1 x2 y2 z2 ...].
    @param ierr (out) error code

    TODO -- should I return a view of the coordinates instead of copying them?
    */
    static void GetProblemGeometry(void *data, int numGIDEntries, int numLIDEntries, int numObjectIDs, 
                                   ZOLTAN_ID_PTR gids, ZOLTAN_ID_PTR lids, int dim, double *coordinates, int *ierr);

    //@}

  private:

    RCP<const Teuchos::MpiComm<int> > comm_;
    RCP<const FactoryBase> AFact_;
    RCP<const Teuchos::OpaqueWrapper<MPI_Comm> > zoltanComm_;
    RCP<Zoltan> zoltanObj_;
    float zoltanVersion_;
    size_t problemDimension_;
    GO numPartitions_;

  };  //class ZoltanInterface

} //namespace MueLu

#define MUELU_ZOLTANINTERFACE_SHORT
#endif //if defined(HAVE_MUELU_ZOLTAN) && defined(HAVE_MPI)

#endif // MUELU_ZOLTAN_DECL_HPP
