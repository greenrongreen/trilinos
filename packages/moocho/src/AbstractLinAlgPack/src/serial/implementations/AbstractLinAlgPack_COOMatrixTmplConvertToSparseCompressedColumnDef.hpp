// /////////////////////////////////////
// COOMatrixTmplConvertToSparseCompressedColumnDef.h

#ifndef COO_MATRIX_TMPL_CONVERT_TO_SPARSE_COMPRESSED_COLUMN_DEF_H
#define COO_MATRIX_TMPL_CONVERT_TO_SPARSE_COMPRESSED_COLUMN_DEF_H

#include "COOMatrixTmplConvertToSparseCompressedColumnDecl.h"

namespace SparseLinAlgPack {

template<class T_COOM>
size_type COOM_num_in_column(
	  const T_COOM&						m
	, BLAS_Cpp::Transp					trans
	, size_type							col_offset
	, const IVector::value_type*		col_perm
	, size_type*						num_in_col	)
{
	if(!m.nz()) return 0;
	if( trans == BLAS_Cpp::no_trans ) {
		// non transposed
		T_COOM::difference_type loc_co = m.col_offset(); 
		for( T_COOM::const_iterator itr = m.begin(); itr != m.end(); ++itr )
			num_in_col[ col_perm[ col_offset + loc_co + itr->col_j()  -1 ]  -1 ]++;
	}
	else {
		// transposed
		T_COOM::difference_type loc_ro = m.row_offset(); 
		for( T_COOM::const_iterator itr = m.begin(); itr != m.end(); ++itr ) {
			const size_type i = itr->row_i();
			num_in_col[ col_perm[ col_offset + loc_ro + i - 1 ]  - 1 ]++;
		}
	}
	return m.nz();
}

template<class T_COOM>
void COOM_insert_nonzeros(
	  const T_COOM&						m
	, BLAS_Cpp::Transp					trans
	, value_type						alpha
	, size_type							row_offset
	, size_type							col_offset
	, const IVector::value_type*		row_perm
	, const IVector::value_type*		col_perm
	, size_type*						next_nz_in_col
	, FortranTypes::f_dbl_prec*			D_val
	, FortranTypes::f_int*				D_row_i			)
{
	if(!m.nz()) return;
	T_COOM::difference_type
		loc_ro = m.row_offset(),
		loc_co = m.col_offset();
	if( trans == BLAS_Cpp::no_trans ) {
		// non transposed
		for( T_COOM::const_iterator itr = m.begin(); itr != m.end(); ++itr ) {
			const size_type
				i = loc_ro + itr->row_i(),
				j = loc_co + itr->col_j();
			const size_type
				ele = next_nz_in_col[ col_perm[ col_offset + j - 1 ] - 1 ]++;
			D_val[ ele - 1 ] = alpha * itr->value();
			if(D_row_i)
				D_row_i[ ele - 1 ] = row_perm[ row_offset + i - 1 ];
		}
	}
	else {
		// transposed
		for( T_COOM::const_iterator itr = m.begin(); itr != m.end(); ++itr ) {
			const size_type
				i = loc_co + itr->col_j(),
				j = loc_ro + itr->row_i();
			const size_type
				ele = next_nz_in_col[ col_perm[ col_offset + j - 1 ] - 1 ]++;
			D_val[ ele - 1 ] = alpha * itr->value();
			if(D_row_i)
				D_row_i[ ele - 1 ] = row_perm[ row_offset + i - 1 ];
		}
	}
}

template<class T_COOM>
value_type COOM_insert_scaled_nonzeros(
	  const T_COOM&						m
	, BLAS_Cpp::Transp					trans
	, value_type						scaled_max_ele
	, size_type							row_offset
	, size_type							col_offset
	, const IVector::value_type*		row_perm
	, const IVector::value_type*		col_perm
	, size_type*						next_nz_in_col
	, FortranTypes::f_dbl_prec*			D_val
	, FortranTypes::f_int*				D_row_i			)
{
	value_type alpha = 0;
	for( T_COOM::const_iterator itr = m.begin(); itr != m.end(); ++itr ) {
		register const value_type val = ::fabs( itr->value() );
		if( val > alpha ) alpha = val;
	}
	// scaled_max_ele = max|alpha*A| = alpha * max|A| 
	alpha = scaled_max_ele / alpha;
	COOM_insert_nonzeros( m, trans, alpha, row_offset
		, col_offset, row_perm, col_perm, next_nz_in_col, D_val, D_row_i );
	return alpha;
}

} // end namespace SparseLinAlgPack

#endif	// COO_MATRIX_TMPL_CONVERT_TO_SPARSE_COMPRESSED_COLUMN_DEF_H