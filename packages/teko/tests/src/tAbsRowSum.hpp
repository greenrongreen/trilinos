#ifndef __tAbsRowSum_hpp__
#define __tAbsRowSum_hpp__

// Teuchos includes
#include "Teuchos_RCP.hpp"

// Epetra includes
#include "Epetra_Map.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"

// PB includes
#include "PB_Utilities.hpp"

#include <string>

#include "Test_Utils.hpp"

namespace PB {
namespace Test {

class tAbsRowSum : public UnitTest {
public:
   virtual ~tAbsRowSum() {}

   virtual void initializeTest();
   virtual int runTest(int verbosity,std::ostream & stdstrm,std::ostream & failstrm,int & totalrun);
   virtual bool isParallel() const { return true; }

   bool test_absRowSum(int verbosity,std::ostream & os);
   bool test_invAbsRowSum(int verbosity,std::ostream & os);

protected:
   double tolerance_;
};

} // end namespace Tests
} // end namespace PB

#endif
