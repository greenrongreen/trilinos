#ifndef __Teko_EpetraInverseOpWrapper_hpp__
#define __Teko_EpetraInverseOpWrapper_hpp__

#include "Teko_EpetraOperatorWrapper.hpp"

namespace Teko {
namespace Epetra {

class EpetraInverseOpWrapper : public EpetraOperatorWrapper {
public:
    EpetraInverseOpWrapper(const RCP<const MappingStrategy> & forwardMaps) 
       : EpetraOperatorWrapper(forwardMaps) {}

    /** */
    virtual int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    /** */
    virtual int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

//    /** */
//    virtual const Epetra_Map& OperatorDomainMap() const; 
//
//    /** */
//    virtual const Epetra_Map& OperatorRangeMap() const;
protected:
    EpetraInverseOpWrapper() {}
};

} // end Epetra
} // end Teko

#endif