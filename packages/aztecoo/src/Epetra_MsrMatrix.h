
/* Copyright (2001) Sandia Corportation. Under the terms of Contract 
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this 
 * work by or on behalf of the U.S. Government.  Export of this program
 * may require a license from the United States Government. */


/* NOTICE:  The United States Government is granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in ths data to reproduce, prepare derivative works, and
 * perform publicly and display publicly.  Beginning five (5) years from
 * July 25, 2001, the United States Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable
 * worldwide license in this data to reproduce, prepare derivative works,
 * distribute copies to the public, perform publicly and display
 * publicly, and to permit others to do so.
 * 
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
 * OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
 * ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
 * RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
 * INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
 * THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS. */

#ifndef _EPETRA_MSRMATRIX_H_
#define _EPETRA_MSRMATRIX_H_

#include "Epetra_Object.h"
#include "Epetra_CompObject.h"
#include "Epetra_RowMatrix.h"
#include "az_aztec.h"
#ifdef AZTEC_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif


class Epetra_Map;
class Epetra_BlockMap;
class Epetra_Import;
class Epetra_Export;
class Epetra_Vector;
class Epetra_MultiVector;

//! Epetra_MsrMatrix: A class for constructing and using real-valued double-precision sparse compressed row matrices.

/*! The Epetra_MsrMatrix provides basic support for existing Aztec users who have an investment in the Aztec
    DMSR matrix format.  A user may pass an existing Aztec DMSR matrix to the constructor for this class.  The
    data from the DMSR matrix will \e not be copied.  Thus, any changes the user makes to the DMSR matrix data will
    be reflected in the associated Epetra_MsrMatrix object.
*/    

class Epetra_MsrMatrix: public Epetra_Object, public Epetra_CompObject, public virtual Epetra_RowMatrix  {
      
 public:

  //@{ \name Constructors/Destructor.
  //! Epetra_MsrMatrix constuctor using existing Aztec DMSR matrix.
  /*! Creates a Epetra_MsrMatrix object by encapsulating an existing Aztec DMSR matrix. The
      Aztec matrix must come in as an AZ_MATRIX pointer, and AZ_transform must have called.  
      Also, the AZ_matrix_type must be AZ_MSR_MATRIX.  (If the matrix is stored in Amat, this
      information is contained in Amat->data_org[AZ_matrix_type].)
    
    \param In
           Amat - A completely constructed Aztec DMSR matrix.
    \param In
           proc_config - An Aztec array containing information about the parallel machine.
  */
  Epetra_MsrMatrix(int * proc_config, AZ_MATRIX * Amat);

  //! Epetra_MsrMatrix Destructor
  virtual ~Epetra_MsrMatrix();
  //@}
  
  //@{ \name Extraction methods.

    //! Returns a copy of the specified local row in user-provided arrays.
    /*! 
    \param In
           MyRow - Local row to extract.
    \param In
	   Length - Length of Values and Indices.
    \param Out
	   NumEntries - Number of nonzero entries extracted.
    \param Out
	   Values - Extracted values for this row.
    \param Out
	   Indices - Extracted global column indices for the corresponding values.
	  
    \return Integer error code, set to 0 if successful.
  */
    int ExtractMyRowCopy(int MyRow, int Length, int & NumEntries, double *Values, int * Indices) const;

    //! Returns a copy of the main diagonal in a user-provided vector.
    /*! 
    \param Out
	   Diagonal - Extracted main diagonal.

    \return Integer error code, set to 0 if successful.
  */
    int ExtractDiagonalCopy(Epetra_Vector & Diagonal) const;
    //@}

  //@{ \name Computational methods.

    //! Returns the result of a Epetra_MsrMatrix multiplied by a Epetra_MultiVector X in Y.
    /*! 
    \param In
	   TransA -If true, multiply by the transpose of matrix, otherwise just use matrix.
    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to multiply with matrix.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectorscontaining result.

    \return Integer error code, set to 0 if successful.
  */
    int Multiply(bool TransA, const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    //! Returns the result of a Epetra_MsrMatrix multiplied by a Epetra_MultiVector X in Y.
    /*! 
    \param In
	   Upper -If true, solve Ux = y, otherwise solve Lx = y.
    \param In
	   Trans -If true, solve transpose problem.
    \param In
	   UnitDiagonal -If true, assume diagonal is unit (whether it's stored or not).
    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to solve for.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.
  */
    int Solve(bool Upper, bool Trans, bool UnitDiagonal, const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    //! Computes the sum of absolute values of the rows of the Epetra_MsrMatrix, results returned in x.
    /*! The vector x will return such that x[i] will contain the inverse of sum of the absolute values of the 
        \e this matrix will be scaled such that A(i,j) = x(i)*A(i,j) where i denotes the global row number of A
        and j denotes the global column number of A.  Using the resulting vector from this function as input to LeftScale()
	will make the infinity norm of the resulting matrix exactly 1.
    \param Out
	   x -A Epetra_Vector containing the row sums of the \e this matrix. 
	   \warning It is assumed that the distribution of x is the same as the rows of \e this.

    \return Integer error code, set to 0 if successful.
  */
    int InvRowSums(Epetra_Vector& x) const;

    //! Scales the Epetra_MsrMatrix on the left with a Epetra_Vector x.
    /*! The \e this matrix will be scaled such that A(i,j) = x(i)*A(i,j) where i denotes the row number of A
        and j denotes the column number of A.
    \param In
	   x -A Epetra_Vector to solve for.

    \return Integer error code, set to 0 if successful.
  */
    int LeftScale(const Epetra_Vector& x);

    //! Computes the sum of absolute values of the columns of the Epetra_MsrMatrix, results returned in x.
    /*! The vector x will return such that x[j] will contain the inverse of sum of the absolute values of the 
        \e this matrix will be sca such that A(i,j) = x(j)*A(i,j) where i denotes the global row number of A
        and j denotes the global column number of A.  Using the resulting vector from this function as input to 
	RighttScale() will make the one norm of the resulting matrix exactly 1.
    \param Out
	   x -A Epetra_Vector containing the column sums of the \e this matrix. 
	   \warning It is assumed that the distribution of x is the same as the rows of \e this.

    \return Integer error code, set to 0 if successful.
  */
    int InvColSums(Epetra_Vector& x) const;

    //! Scales the Epetra_MsrMatrix on the right with a Epetra_Vector x.
    /*! The \e this matrix will be scaled such that A(i,j) = x(j)*A(i,j) where i denotes the global row number of A
        and j denotes the global column number of A.
    \param In
	   x -The Epetra_Vector used for scaling \e this.

    \return Integer error code, set to 0 if successful.
  */
    int RightScale(const Epetra_Vector& x);
  //@}

  //@{ \name Matrix Properties Query Methods.


    //! If FillComplete() has been called, this query returns true, otherwise it returns false.
    bool Filled() const {return(true);};

    //! If matrix is lower triangular, this query returns true, otherwise it returns false.
    bool LowerTriangular() const {return(false);};

    //! If matrix is upper triangular, this query returns true, otherwise it returns false.
    bool UpperTriangular() const {return(false);};

  //@}
  
  //@{ \name Atribute access functions

    //! Returns the infinity norm of the global matrix.
    /* Returns the quantity \f$ \| A \|_\infty\f$ such that
       \f[\| A \|_\infty = \max_{1\lei\lem} \sum_{j=1}^n |a_{ij}| \f].
    */ 
    double NormInf() const;

    //! Returns the one norm of the global matrix.
    /* Returns the quantity \f$ \| A \|_1\f$ such that
       \f[\| A \|_1= \max_{1\lej\len} \sum_{i=1}^m |a_{ij}| \f].
    */ 
    double NormOne() const;

    //! Returns the number of nonzero entries in the global matrix.
    int NumGlobalNonzeros() const {return(NumGlobalNonzeros_);};

    //! Returns the number of global matrix rows.
    int NumGlobalRows() const {return(RangeMap().NumGlobalPoints());};

    //! Returns the number of global matrix columns.
    int NumGlobalCols() const {return(DomainMap().NumGlobalPoints());};

    //! Returns the number of global nonzero diagonal entries.
    int NumGlobalDiagonals() const{return(DomainMap().NumGlobalPoints());};
    
    //! Returns the number of nonzero entries in the calling processor's portion of the matrix.
    int NumMyNonzeros() const {return(NumMyNonzeros_);};

    //! Returns the number of matrix rows owned by the calling processor.
    int NumMyRows() const {return(RangeMap().NumMyPoints());};

    //! Returns the number of matrix columns owned by the calling processor.
    int NumMyCols() const {return(BlockImportMap().NumMyPoints());};

    //! Returns the number of local nonzero diagonal entries.
    int NumMyDiagonals() const {return(RangeMap().NumMyPoints());};

    //! Returns the Epetra_BlockMap object associated with the domain of this matrix operator.
    const Epetra_BlockMap & DomainMap() const {return(*DomainMap_);};

    //! Returns the Epetra_BlockMap object associated with the range of this matrix operator.
    const Epetra_BlockMap & RangeMap() const  {return(*RangeMap_);};

    //! Returns the RowMap object as an Epetra_BlockMap (the Epetra_Map base class) needed for implementing Epetra_RowMatrix.
    const Epetra_BlockMap & BlockRowMap() const {return(RangeMap());};

    //! Returns the Import object as an Epetra_BlockMap (the Epetra_Map base class) needed for implementing Epetra_RowMatrix.
    const Epetra_BlockMap & BlockImportMap() const {return(*ImportMap_);};

    //! Returns the Epetra_Import object that contains the import operations for distributed operations.
    virtual const Epetra_Import * Importer() const {return(Importer_);};

    //! Returns a pointer to the Epetra_Comm communicator associated with this matrix.
    const Epetra_Comm & Comm() const {return(*Comm_);};
  //@}
  
  
  //@{ \name I/O Methods.

  //! Print method
  virtual void Print(ostream & os) const;
  //@}

  //@{ \name Additional methods required to support the Epetra_Operator interface.

    //! Returns a character string describing the operator
    char * Label() const {return(Epetra_Object::Label());};
    
    //! If set true, transpose of this operator will be applied.
    /*! This flag allows the transpose of the given operator to be used implicitly.  Setting this flag
        affects only the Apply() and ApplyInverse() methods.  If the implementation of this interface 
	does not support transpose use, this method should return a value of -1.
      
    \param In
	   UseTranspose -If true, multiply by the transpose of operator, otherwise just use operator.

    \return Always returns 0.
  */
  int SetUseTranspose(bool UseTranspose) {return(-1);};

    //! Returns the result of a Epetra_Operator applied to a Epetra_MultiVector X in Y.
    /*! 
    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to multiply with matrix.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.
  */
  int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const {
    return(Epetra_MsrMatrix::Multiply(Epetra_MsrMatrix::UseTranspose(), X, Y));};

    //! Returns the result of a Epetra_Operator inverse applied to an Epetra_MultiVector X in Y.
    /*! In this implementation, we use several existing attributes to determine how virtual
        method ApplyInverse() should call the concrete method Solve().  We pass in the UpperTriangular(), 
	the Epetra_MsrMatrix::UseTranspose(), and NoDiagonal() methods. The most notable warning is that
	if a matrix has no diagonal values we assume that there is an implicit unit diagonal that should
	be accounted for when doing a triangular solve.

    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to solve for.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.
  */
  int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const {return(-1);};

    //! Returns true because this class can compute an Inf-norm.
    virtual bool HasNormInf() const {return(true);};

    //! Returns the current UseTranspose setting.
    virtual bool UseTranspose() const {return(false);};

  //@}
  //@{ \name Additional methods required to implement RowMatrix interface.

    //! Return the current number of values stored for the specified local row.
    /*! Similar to NumMyEntries() except NumEntries is returned as an argument
        and error checking is done on the input value MyRow.
    \param In
           MyRow - Local row.
    \param Out
	   NumEntries - Number of nonzero values.
	  
    \return Integer error code, set to 0 if successful.
  */
    int NumMyRowEntries(int MyRow, int & NumEntries) const;
  //@}

 private:

    int GetRow(int Row) const;
    AZ_MATRIX * Amat_;
    int * proc_config_;
    mutable double * Values_;
    mutable int * Indices_;
    mutable int MaxNumEntries_;
    
#ifdef AZTEC_MPI
  Epetra_MpiComm * Comm_;
#else
  Epetra_SerialComm * Comm_;
#endif  
    Epetra_BlockMap * DomainMap_;
    Epetra_BlockMap * RangeMap_;
    Epetra_BlockMap * ImportMap_;
    Epetra_Import * Importer_;
    mutable Epetra_MultiVector * ImportVector_;
 
    int NumGlobalNonzeros_;
    int NumMyNonzeros_;
    int NumMyRows_;
    int NumMyCols_;
    mutable double NormInf_;
    mutable double NormOne_;
    

 //! Copy constructor (not accessible to users).
  Epetra_MsrMatrix(const Epetra_MsrMatrix & Matrix) {};
};
#endif /* _EPETRA_MSRMATRIX_H_ */
