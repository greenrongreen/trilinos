// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
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
// Questions? Contact Pavel Bochev (pbboche@sandia.gov) or
//                    Denis Ridzal (dridzal@sandia.gov) or
//                    Robert Kirby (robert.c.kirby@ttu.edu)
//                    FIAT (fiat-dev@fenics.org) (must join mailing list first, see www.fenics.org)
//
// ************************************************************************
// @HEADER

/** \file   Intrepid_F2_TET_I8_FEM_FIATDef.hpp
    \brief  Definition file for FEM basis functions of degree 7 for 2-forms on TET cells.
    \author Created by R. Kirby via the FIAT project
*/

namespace Intrepid {

template<class Scalar>
void Basis_F2_TET_I8_FEM_FIAT<Scalar>::initialize() {

  // Basis-dependent initializations
  int tagSize  = 4;         // size of DoF tag
  int posScDim = 0;         // position in the tag, counting from 0, of the subcell dim 
  int posScId  = 1;         // position in the tag, counting from 0, of the subcell id
  int posBfId  = 2;         // position in the tag, counting from 0, of DoF Id relative to the subcell

  // An array with local DoF tags assigned to the basis functions, in the order of their local enumeration 
  int tags[] = { 2 , 0 , 0 , 36 , 2 , 0 , 1 , 36 , 2 , 0 , 2 , 36 , 2 , 0 , 3 , 36 , 2 , 0 , 4 , 36 , 2 , 0 , 5 , 36 , 2 , 0 , 6 , 36 , 2 , 0 , 7 , 36 , 2 , 0 , 8 , 36 , 2 , 0 , 9 , 36 , 2 , 0 , 10 , 36 , 2 , 0 , 11 , 36 , 2 , 0 , 12 , 36 , 2 , 0 , 13 , 36 , 2 , 0 , 14 , 36 , 2 , 0 , 15 , 36 , 2 , 0 , 16 , 36 , 2 , 0 , 17 , 36 , 2 , 0 , 18 , 36 , 2 , 0 , 19 , 36 , 2 , 0 , 20 , 36 , 2 , 0 , 21 , 36 , 2 , 0 , 22 , 36 , 2 , 0 , 23 , 36 , 2 , 0 , 24 , 36 , 2 , 0 , 25 , 36 , 2 , 0 , 26 , 36 , 2 , 0 , 27 , 36 , 2 , 0 , 28 , 36 , 2 , 0 , 29 , 36 , 2 , 0 , 30 , 36 , 2 , 0 , 31 , 36 , 2 , 0 , 32 , 36 , 2 , 0 , 33 , 36 , 2 , 0 , 34 , 36 , 2 , 0 , 35 , 36 , 2 , 1 , 0 , 36 , 2 , 1 , 1 , 36 , 2 , 1 , 2 , 36 , 2 , 1 , 3 , 36 , 2 , 1 , 4 , 36 , 2 , 1 , 5 , 36 , 2 , 1 , 6 , 36 , 2 , 1 , 7 , 36 , 2 , 1 , 8 , 36 , 2 , 1 , 9 , 36 , 2 , 1 , 10 , 36 , 2 , 1 , 11 , 36 , 2 , 1 , 12 , 36 , 2 , 1 , 13 , 36 , 2 , 1 , 14 , 36 , 2 , 1 , 15 , 36 , 2 , 1 , 16 , 36 , 2 , 1 , 17 , 36 , 2 , 1 , 18 , 36 , 2 , 1 , 19 , 36 , 2 , 1 , 20 , 36 , 2 , 1 , 21 , 36 , 2 , 1 , 22 , 36 , 2 , 1 , 23 , 36 , 2 , 1 , 24 , 36 , 2 , 1 , 25 , 36 , 2 , 1 , 26 , 36 , 2 , 1 , 27 , 36 , 2 , 1 , 28 , 36 , 2 , 1 , 29 , 36 , 2 , 1 , 30 , 36 , 2 , 1 , 31 , 36 , 2 , 1 , 32 , 36 , 2 , 1 , 33 , 36 , 2 , 1 , 34 , 36 , 2 , 1 , 35 , 36 , 2 , 2 , 0 , 36 , 2 , 2 , 1 , 36 , 2 , 2 , 2 , 36 , 2 , 2 , 3 , 36 , 2 , 2 , 4 , 36 , 2 , 2 , 5 , 36 , 2 , 2 , 6 , 36 , 2 , 2 , 7 , 36 , 2 , 2 , 8 , 36 , 2 , 2 , 9 , 36 , 2 , 2 , 10 , 36 , 2 , 2 , 11 , 36 , 2 , 2 , 12 , 36 , 2 , 2 , 13 , 36 , 2 , 2 , 14 , 36 , 2 , 2 , 15 , 36 , 2 , 2 , 16 , 36 , 2 , 2 , 17 , 36 , 2 , 2 , 18 , 36 , 2 , 2 , 19 , 36 , 2 , 2 , 20 , 36 , 2 , 2 , 21 , 36 , 2 , 2 , 22 , 36 , 2 , 2 , 23 , 36 , 2 , 2 , 24 , 36 , 2 , 2 , 25 , 36 , 2 , 2 , 26 , 36 , 2 , 2 , 27 , 36 , 2 , 2 , 28 , 36 , 2 , 2 , 29 , 36 , 2 , 2 , 30 , 36 , 2 , 2 , 31 , 36 , 2 , 2 , 32 , 36 , 2 , 2 , 33 , 36 , 2 , 2 , 34 , 36 , 2 , 2 , 35 , 36 , 2 , 3 , 0 , 36 , 2 , 3 , 1 , 36 , 2 , 3 , 2 , 36 , 2 , 3 , 3 , 36 , 2 , 3 , 4 , 36 , 2 , 3 , 5 , 36 , 2 , 3 , 6 , 36 , 2 , 3 , 7 , 36 , 2 , 3 , 8 , 36 , 2 , 3 , 9 , 36 , 2 , 3 , 10 , 36 , 2 , 3 , 11 , 36 , 2 , 3 , 12 , 36 , 2 , 3 , 13 , 36 , 2 , 3 , 14 , 36 , 2 , 3 , 15 , 36 , 2 , 3 , 16 , 36 , 2 , 3 , 17 , 36 , 2 , 3 , 18 , 36 , 2 , 3 , 19 , 36 , 2 , 3 , 20 , 36 , 2 , 3 , 21 , 36 , 2 , 3 , 22 , 36 , 2 , 3 , 23 , 36 , 2 , 3 , 24 , 36 , 2 , 3 , 25 , 36 , 2 , 3 , 26 , 36 , 2 , 3 , 27 , 36 , 2 , 3 , 28 , 36 , 2 , 3 , 29 , 36 , 2 , 3 , 30 , 36 , 2 , 3 , 31 , 36 , 2 , 3 , 32 , 36 , 2 , 3 , 33 , 36 , 2 , 3 , 34 , 36 , 2 , 3 , 35 , 36 , 3 , 0 , 0 , 252 , 3 , 0 , 1 , 252 , 3 , 0 , 2 , 252 , 3 , 0 , 3 , 252 , 3 , 0 , 4 , 252 , 3 , 0 , 5 , 252 , 3 , 0 , 6 , 252 , 3 , 0 , 7 , 252 , 3 , 0 , 8 , 252 , 3 , 0 , 9 , 252 , 3 , 0 , 10 , 252 , 3 , 0 , 11 , 252 , 3 , 0 , 12 , 252 , 3 , 0 , 13 , 252 , 3 , 0 , 14 , 252 , 3 , 0 , 15 , 252 , 3 , 0 , 16 , 252 , 3 , 0 , 17 , 252 , 3 , 0 , 18 , 252 , 3 , 0 , 19 , 252 , 3 , 0 , 20 , 252 , 3 , 0 , 21 , 252 , 3 , 0 , 22 , 252 , 3 , 0 , 23 , 252 , 3 , 0 , 24 , 252 , 3 , 0 , 25 , 252 , 3 , 0 , 26 , 252 , 3 , 0 , 27 , 252 , 3 , 0 , 28 , 252 , 3 , 0 , 29 , 252 , 3 , 0 , 30 , 252 , 3 , 0 , 31 , 252 , 3 , 0 , 32 , 252 , 3 , 0 , 33 , 252 , 3 , 0 , 34 , 252 , 3 , 0 , 35 , 252 , 3 , 0 , 36 , 252 , 3 , 0 , 37 , 252 , 3 , 0 , 38 , 252 , 3 , 0 , 39 , 252 , 3 , 0 , 40 , 252 , 3 , 0 , 41 , 252 , 3 , 0 , 42 , 252 , 3 , 0 , 43 , 252 , 3 , 0 , 44 , 252 , 3 , 0 , 45 , 252 , 3 , 0 , 46 , 252 , 3 , 0 , 47 , 252 , 3 , 0 , 48 , 252 , 3 , 0 , 49 , 252 , 3 , 0 , 50 , 252 , 3 , 0 , 51 , 252 , 3 , 0 , 52 , 252 , 3 , 0 , 53 , 252 , 3 , 0 , 54 , 252 , 3 , 0 , 55 , 252 , 3 , 0 , 56 , 252 , 3 , 0 , 57 , 252 , 3 , 0 , 58 , 252 , 3 , 0 , 59 , 252 , 3 , 0 , 60 , 252 , 3 , 0 , 61 , 252 , 3 , 0 , 62 , 252 , 3 , 0 , 63 , 252 , 3 , 0 , 64 , 252 , 3 , 0 , 65 , 252 , 3 , 0 , 66 , 252 , 3 , 0 , 67 , 252 , 3 , 0 , 68 , 252 , 3 , 0 , 69 , 252 , 3 , 0 , 70 , 252 , 3 , 0 , 71 , 252 , 3 , 0 , 72 , 252 , 3 , 0 , 73 , 252 , 3 , 0 , 74 , 252 , 3 , 0 , 75 , 252 , 3 , 0 , 76 , 252 , 3 , 0 , 77 , 252 , 3 , 0 , 78 , 252 , 3 , 0 , 79 , 252 , 3 , 0 , 80 , 252 , 3 , 0 , 81 , 252 , 3 , 0 , 82 , 252 , 3 , 0 , 83 , 252 , 3 , 0 , 84 , 252 , 3 , 0 , 85 , 252 , 3 , 0 , 86 , 252 , 3 , 0 , 87 , 252 , 3 , 0 , 88 , 252 , 3 , 0 , 89 , 252 , 3 , 0 , 90 , 252 , 3 , 0 , 91 , 252 , 3 , 0 , 92 , 252 , 3 , 0 , 93 , 252 , 3 , 0 , 94 , 252 , 3 , 0 , 95 , 252 , 3 , 0 , 96 , 252 , 3 , 0 , 97 , 252 , 3 , 0 , 98 , 252 , 3 , 0 , 99 , 252 , 3 , 0 , 100 , 252 , 3 , 0 , 101 , 252 , 3 , 0 , 102 , 252 , 3 , 0 , 103 , 252 , 3 , 0 , 104 , 252 , 3 , 0 , 105 , 252 , 3 , 0 , 106 , 252 , 3 , 0 , 107 , 252 , 3 , 0 , 108 , 252 , 3 , 0 , 109 , 252 , 3 , 0 , 110 , 252 , 3 , 0 , 111 , 252 , 3 , 0 , 112 , 252 , 3 , 0 , 113 , 252 , 3 , 0 , 114 , 252 , 3 , 0 , 115 , 252 , 3 , 0 , 116 , 252 , 3 , 0 , 117 , 252 , 3 , 0 , 118 , 252 , 3 , 0 , 119 , 252 , 3 , 0 , 120 , 252 , 3 , 0 , 121 , 252 , 3 , 0 , 122 , 252 , 3 , 0 , 123 , 252 , 3 , 0 , 124 , 252 , 3 , 0 , 125 , 252 , 3 , 0 , 126 , 252 , 3 , 0 , 127 , 252 , 3 , 0 , 128 , 252 , 3 , 0 , 129 , 252 , 3 , 0 , 130 , 252 , 3 , 0 , 131 , 252 , 3 , 0 , 132 , 252 , 3 , 0 , 133 , 252 , 3 , 0 , 134 , 252 , 3 , 0 , 135 , 252 , 3 , 0 , 136 , 252 , 3 , 0 , 137 , 252 , 3 , 0 , 138 , 252 , 3 , 0 , 139 , 252 , 3 , 0 , 140 , 252 , 3 , 0 , 141 , 252 , 3 , 0 , 142 , 252 , 3 , 0 , 143 , 252 , 3 , 0 , 144 , 252 , 3 , 0 , 145 , 252 , 3 , 0 , 146 , 252 , 3 , 0 , 147 , 252 , 3 , 0 , 148 , 252 , 3 , 0 , 149 , 252 , 3 , 0 , 150 , 252 , 3 , 0 , 151 , 252 , 3 , 0 , 152 , 252 , 3 , 0 , 153 , 252 , 3 , 0 , 154 , 252 , 3 , 0 , 155 , 252 , 3 , 0 , 156 , 252 , 3 , 0 , 157 , 252 , 3 , 0 , 158 , 252 , 3 , 0 , 159 , 252 , 3 , 0 , 160 , 252 , 3 , 0 , 161 , 252 , 3 , 0 , 162 , 252 , 3 , 0 , 163 , 252 , 3 , 0 , 164 , 252 , 3 , 0 , 165 , 252 , 3 , 0 , 166 , 252 , 3 , 0 , 167 , 252 , 3 , 0 , 168 , 252 , 3 , 0 , 169 , 252 , 3 , 0 , 170 , 252 , 3 , 0 , 171 , 252 , 3 , 0 , 172 , 252 , 3 , 0 , 173 , 252 , 3 , 0 , 174 , 252 , 3 , 0 , 175 , 252 , 3 , 0 , 176 , 252 , 3 , 0 , 177 , 252 , 3 , 0 , 178 , 252 , 3 , 0 , 179 , 252 , 3 , 0 , 180 , 252 , 3 , 0 , 181 , 252 , 3 , 0 , 182 , 252 , 3 , 0 , 183 , 252 , 3 , 0 , 184 , 252 , 3 , 0 , 185 , 252 , 3 , 0 , 186 , 252 , 3 , 0 , 187 , 252 , 3 , 0 , 188 , 252 , 3 , 0 , 189 , 252 , 3 , 0 , 190 , 252 , 3 , 0 , 191 , 252 , 3 , 0 , 192 , 252 , 3 , 0 , 193 , 252 , 3 , 0 , 194 , 252 , 3 , 0 , 195 , 252 , 3 , 0 , 196 , 252 , 3 , 0 , 197 , 252 , 3 , 0 , 198 , 252 , 3 , 0 , 199 , 252 , 3 , 0 , 200 , 252 , 3 , 0 , 201 , 252 , 3 , 0 , 202 , 252 , 3 , 0 , 203 , 252 , 3 , 0 , 204 , 252 , 3 , 0 , 205 , 252 , 3 , 0 , 206 , 252 , 3 , 0 , 207 , 252 , 3 , 0 , 208 , 252 , 3 , 0 , 209 , 252 , 3 , 0 , 210 , 252 , 3 , 0 , 211 , 252 , 3 , 0 , 212 , 252 , 3 , 0 , 213 , 252 , 3 , 0 , 214 , 252 , 3 , 0 , 215 , 252 , 3 , 0 , 216 , 252 , 3 , 0 , 217 , 252 , 3 , 0 , 218 , 252 , 3 , 0 , 219 , 252 , 3 , 0 , 220 , 252 , 3 , 0 , 221 , 252 , 3 , 0 , 222 , 252 , 3 , 0 , 223 , 252 , 3 , 0 , 224 , 252 , 3 , 0 , 225 , 252 , 3 , 0 , 226 , 252 , 3 , 0 , 227 , 252 , 3 , 0 , 228 , 252 , 3 , 0 , 229 , 252 , 3 , 0 , 230 , 252 , 3 , 0 , 231 , 252 , 3 , 0 , 232 , 252 , 3 , 0 , 233 , 252 , 3 , 0 , 234 , 252 , 3 , 0 , 235 , 252 , 3 , 0 , 236 , 252 , 3 , 0 , 237 , 252 , 3 , 0 , 238 , 252 , 3 , 0 , 239 , 252 , 3 , 0 , 240 , 252 , 3 , 0 , 241 , 252 , 3 , 0 , 242 , 252 , 3 , 0 , 243 , 252 , 3 , 0 , 244 , 252 , 3 , 0 , 245 , 252 , 3 , 0 , 246 , 252 , 3 , 0 , 247 , 252 , 3 , 0 , 248 , 252 , 3 , 0 , 249 , 252 , 3 , 0 , 250 , 252 , 3 , 0 , 251 , 252 };
  
  // Basis-independent function sets tag and enum data in the static arrays:
  Intrepid::setEnumTagData(tagToEnum_,
                           enumToTag_,
                           tags,
                           numDof_,
                           tagSize,
                           posScDim,
                           posScId,
                           posBfId);
}

template<class Scalar> 
void Basis_F2_TET_I8_FEM_FIAT<Scalar>::getValues(FieldContainer<Scalar>&                  outputValues,
          const Teuchos::Array< Point<Scalar> >& inputPoints,
          const EOperator                        operatorType) const {

  // Determine parameters to shape outputValues: number of points = size of input array
  int numPoints = inputPoints.size();
	
  // Incomplete polynomial basis of degree 7 (I7) has 396 basis functions on a tetrahedron that are 2-forms in 3D
  int    numFields = 396;
  EField fieldType = FIELD_FORM_2;
  int    spaceDim  = 3;

  // temporaries
  int countPt  = 0;               // point counter
  Teuchos::Array<int> indexV(3);  // multi-index for values
  Teuchos::Array<int> indexD(2);  // multi-index for divergences

  // Shape the FieldContainer for the output values using these values:
  outputValues.resize(numPoints,
                     numFields,
                     fieldType,
                     operatorType,
                     spaceDim);

#ifdef HAVE_INTREPID_DEBUG
  for (countPt=0; countPt<numPoints; countPt++) {
    // Verify that all points are inside the TET reference cell
    TEST_FOR_EXCEPTION( !MultiCell<Scalar>::inReferenceCell(CELL_TET, inputPoints[countPt]),
                        std::invalid_argument,
                        ">>> ERROR (Basis_F2_TET_I8_FEM_FIAT): Evaluation point is outside the TET reference cell");
  }
#endif
  switch(operatorType) {
    case OPERATOR_VALUE:   
    {
      Teuchos::SerialDenseMatrix<int,Scalar> expansions(165,numPoints);
      Teuchos::SerialDenseMatrix<int,Scalar> result(396,numPoints);
      OrthogonalExpansions<Scalar>::tabulate(CELL_TET,8,inputPoints,expansions);
      result.multiply(Teuchos::NO_TRANS,Teuchos::NO_TRANS,1,*vdm0_,expansions,0);
      for (countPt=0;countPt<numPoints;countPt++) {
        for (int i=0;i<396;i++) {
          outputValues(countPt,i,0) = result(i,countPt);
        }
      }
      result.multiply(Teuchos::NO_TRANS,Teuchos::NO_TRANS,1,*vdm1_,expansions,0);
      for (countPt=0;countPt<numPoints;countPt++) {
        for (int i=0;i<396;i++) {
          outputValues(countPt,i,1) = result(i,countPt);
        }
      }
      result.multiply(Teuchos::NO_TRANS,Teuchos::NO_TRANS,1,*vdm2_,expansions,0);
      for (countPt=0;countPt<numPoints;countPt++) {
        for (int i=0;i<396;i++) {
          outputValues(countPt,i,2) = result(i,countPt);
        }
      }
    }
    break;

    case OPERATOR_D1:
    case OPERATOR_D2:
    case OPERATOR_D3:
    case OPERATOR_D4:
    case OPERATOR_D5:
    case OPERATOR_D6:
    case OPERATOR_D7:
    case OPERATOR_D8:
    case OPERATOR_D9:
    case OPERATOR_D10:
    {
      TEST_FOR_EXCEPTION( true , std::invalid_argument, ">>>ERROR F2_TET_I8_FEM_FIAT: operator not implemented" );
    }
    break; 
    case OPERATOR_CURL:
    {
      TEST_FOR_EXCEPTION( true , std::invalid_argument, ">>>ERROR F2_TET_I8_FEM_FIAT: operator not implemented" );
    }
    break; 
    case OPERATOR_DIV:
    {
      Teuchos::SerialDenseMatrix<int,Scalar> expansions(165,numPoints);
      Teuchos::SerialDenseMatrix<int,Scalar> tmp(165,165);
      Teuchos::SerialDenseMatrix<int,Scalar> div(396,numPoints);
      div.putScalar(0.0);
      OrthogonalExpansions<Scalar>::tabulate(CELL_TET,8,inputPoints,expansions);
      tmp.multiply(Teuchos::TRANS,Teuchos::NO_TRANS,1.0,*dmats0_,expansions,0.0);
      div.multiply(Teuchos::NO_TRANS,Teuchos::NO_TRANS,1.0,*vdm0_,tmp,1.0);
      tmp.multiply(Teuchos::TRANS,Teuchos::NO_TRANS,1.0,*dmats1_,expansions,0.0);
      div.multiply(Teuchos::NO_TRANS,Teuchos::NO_TRANS,1.0,*vdm1_,tmp,1.0);
      tmp.multiply(Teuchos::TRANS,Teuchos::NO_TRANS,1.0,*dmats2_,expansions,0.0);
      div.multiply(Teuchos::NO_TRANS,Teuchos::NO_TRANS,1.0,*vdm2_,tmp,1.0);
      for (countPt=0;countPt<numPoints;countPt++) {
        for (int i=0;i<396;i++) {
          outputValues(countPt,i) = div(i,countPt);
        }
      }
    }
    break;

    default:
      TEST_FOR_EXCEPTION( ( (operatorType != OPERATOR_VALUE) &&
                            (operatorType != OPERATOR_GRAD)  &&
                            (operatorType != OPERATOR_CURL)  &&
                            (operatorType != OPERATOR_DIV)   &&
                            (operatorType != OPERATOR_D1)    &&
                            (operatorType != OPERATOR_D2)    &&
                            (operatorType != OPERATOR_D3)    &&
                            (operatorType != OPERATOR_D4)    &&
                            (operatorType != OPERATOR_D5)    &&
                            (operatorType != OPERATOR_D6)    &&
                            (operatorType != OPERATOR_D7)    &&
                            (operatorType != OPERATOR_D8)    &&
                            (operatorType != OPERATOR_D9)    &&
                            (operatorType != OPERATOR_D10) ),
                          std::invalid_argument,
                          ">>> ERROR (Basis_F2_TET_I8_FEM_DEFAULT): Invalid operator type");
  }


}


template<class Scalar>
void Basis_F2_TET_I8_FEM_FIAT<Scalar>::getValues(FieldContainer<Scalar>&                  outputValues,
                                                    const Teuchos::Array< Point<Scalar> >& inputPoints,
                                                    const Cell<Scalar>&                    cell) const {
  TEST_FOR_EXCEPTION( (true),
                      std::logic_error,
                      ">>> ERROR (Basis_F2_TET_I8_FEM_FIAT): FEM Basis calling an FV/D member function");
}



template<class Scalar>
int Basis_F2_TET_I8_FEM_FIAT<Scalar>::getNumLocalDof() const {
    return numDof_;   
}



template<class Scalar>
int Basis_F2_TET_I8_FEM_FIAT<Scalar>::getLocalDofEnumeration(const LocalDofTag dofTag) {
  if (!isSet_) {
    initialize();
    isSet_ = true;
  }
  return tagToEnum_[dofTag.tag_[0]][dofTag.tag_[1]][dofTag.tag_[2]];
}



template<class Scalar>
LocalDofTag Basis_F2_TET_I8_FEM_FIAT<Scalar>::getLocalDofTag(int id) {
  if (!isSet_) {
    initialize();
    isSet_ = true;
  }
  return enumToTag_[id];
}



template<class Scalar>
const Teuchos::Array<LocalDofTag> & Basis_F2_TET_I8_FEM_FIAT<Scalar>::getAllLocalDofTags() {
  if (!isSet_) {
    initialize();
    isSet_ = true;
  }
  return enumToTag_;
}



template<class Scalar>
ECell Basis_F2_TET_I8_FEM_FIAT<Scalar>::getCellType() const {
  return CELL_TET;
}



template<class Scalar>
EBasis Basis_F2_TET_I8_FEM_FIAT<Scalar>::getBasisType() const {
  return BASIS_FEM_FIAT;
}



template<class Scalar>
ECoordinates Basis_F2_TET_I8_FEM_FIAT<Scalar>::getCoordinateSystem() const {
  return COORDINATES_CARTESIAN;
}



template<class Scalar>
int Basis_F2_TET_I8_FEM_FIAT<Scalar>::getDegree() const {
  return 1;
}


}// namespace Intrepid

