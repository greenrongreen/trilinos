//@HEADER
/*
************************************************************************

              EpetraExt: Extended Linear Algebra Services Package 
                Copyright (2001) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.
 
This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact Michael A. Heroux (maherou@sandia.gov) 

************************************************************************
*/
//@HEADER

#ifndef EPETRAEXT_BLOCKVECTOR_H
#define EPETRAEXT_BLOCKVECTOR_H

#include "Epetra_Vector.h" 

#include <vector>

//! EpetraExt::BlockVector: A class for constructing a distributed block vector

/*! The EpetraExt::BlockVector allows construction of a block vector made up of Epetra_Vector blocks as well as access to the full systems as a Epetra_Vector.  It derives from and extends the Epetra_Vector class

<b>Constructing EpetraExt::BlockVector objects</b>

*/    

namespace EpetraExt {

class BlockVector: public Epetra_Vector {
 public:

  //@{ \name Constructors/Destructor.
  //! BlockVector constuctor with one block row per processor.
  /*! Creates a BlockVector object and allocates storage.  
    
	\param In
	BaseMap - Map determining local structure, can be distrib. over subset of proc.'s
	\param In 
	GlobalMap - Full map describing the overall global structure, generally generated by the construction of a BlockCrsMatrix object
	\param In 
	NumBlocks - Number of local blocks
  */
  BlockVector( const Epetra_BlockMap & BaseMap, const Epetra_BlockMap & GlobalMap);
  
  //! Copy constructor.
  BlockVector( const BlockVector & MV );

  //! Destructor
  virtual ~BlockVector();
  //@}
  
  //! Extract a single block from a Block Vector: block row is global, not a stencil value
  int ExtractBlockValues( Epetra_Vector & BaseVec, int BlockRow) const;

  //! Load a single block into a Block Vector: block row is global, not a stencil value
  int LoadBlockValues(const Epetra_Vector & BaseVec, int BlockRow);
	
 protected:

  Epetra_BlockMap BaseMap_;

  int Offset_;

};

} //namespace EpetraExt

#endif /* EPETRAEXT_BLOCKVECTOR_H */
