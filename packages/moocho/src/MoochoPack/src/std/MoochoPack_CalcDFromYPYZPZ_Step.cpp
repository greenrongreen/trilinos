// ////////////////////////////////////////////////////////////////////////////
// CalcDFromYPYZPZ_Step.cpp

// disable VC 5.0 warnings about debugger limitations
#pragma warning(disable : 4786)	

#include <ostream>

#include "../../include/std/CalcDFromYPYZPZ_Step.h"
#include "../../include/rsqp_algo_conversion.h"
#include "GeneralIterationPack/include/print_algorithm_step.h"
#include "ConstrainedOptimizationPack/include/print_vector_change_stats.h"
#include "SparseLinAlgPack/include/MatrixWithOp.h"
#include "LinAlgPack/include/VectorOp.h"
#include "LinAlgPack/include/VectorOut.h"
#include "LinAlgPack/include/LinAlgOpPack.h"

namespace LinAlgOpPack {
	using SparseLinAlgPack::Vp_StMtV;
}

bool ReducedSpaceSQPPack::CalcDFromYPYZPZ_Step::do_step(Algorithm& _algo
	, poss_type step_poss, GeneralIterationPack::EDoStepType type, poss_type assoc_step_poss)
{
	using LinAlgPack::V_VpV;
	using LinAlgPack::norm_inf;

	rSQPAlgo	&algo	= rsqp_algo(_algo);
	rSQPState	&s		= algo.rsqp_state();
	Range1D		indep	= s.con_indep();

	EIterationInfoOutput olevel = s.iteration_info_output();
	std::ostream& out = algo.track().journal_out();

	// print step header.
	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
		using GeneralIterationPack::print_algorithm_step;
		print_algorithm_step( algo, step_poss, type, assoc_step_poss, out );
	}

	// d = Ypy + Zpz
	V_VpV( &s.d().set_k(0), s.Ypy().get_k(0), s.Zpz().get_k(0) );

	if( (int)olevel >= (int)PRINT_ALGORITHM_STEPS ) {
		out	<< "\n||d||inf = " << ( s.norm_inf_d().set_k(0) = norm_inf(s.d().get_k(0)) )
			<< std::endl;
		ConstrainedOptimizationPack::print_vector_change_stats(
			s.x().get_k(0), "x", s.d().get_k(0), "d", out );
	}

	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_VECTORS) ) {
		out	<< "\nd_k = \n" << s.d().get_k(0);
	}

	return true;
}

void ReducedSpaceSQPPack::CalcDFromYPYZPZ_Step::print_step( const Algorithm& algo
	, poss_type step_poss, GeneralIterationPack::EDoStepType type, poss_type assoc_step_poss
	, std::ostream& out, const std::string& L ) const
{
	out
		<< L << "*** Calculates the search direction d from Ypy and Zpz\n"
		<< L << "d_k = Ypy_k + Zpz_k \n";
}
