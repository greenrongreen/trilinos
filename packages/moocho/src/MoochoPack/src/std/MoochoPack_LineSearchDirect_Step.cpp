// ////////////////////////////////////////////////////////////////////////////
// LineSearchDirect_Step.cpp
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.

#include <ostream>
#include <typeinfo>

#include "ReducedSpaceSQPPack/include/std/LineSearchDirect_Step.h"
#include "ReducedSpaceSQPPack/include/ReducedSpaceSQPPackExceptions.h"
#include "ReducedSpaceSQPPack/include/rsqp_algo_conversion.h"
#include "GeneralIterationPack/include/print_algorithm_step.h"
#include "ConstrainedOptimizationPack/include/MeritFuncCalc1DQuadratic.h"
#include "ConstrainedOptimizationPack/include/MeritFuncCalcNLP.h"
#include "AbstractLinAlgPack/include/VectorWithOpMutable.h"
#include "AbstractLinAlgPack/include/VectorStdOps.h"
#include "AbstractLinAlgPack/include/VectorWithOpOut.h"
#include "AbstractLinAlgPack/include/assert_print_nan_inf.h"
#include "AbstractLinAlgPack/include/LinAlgOpPack.h"
#include "ThrowException.h"

namespace ReducedSpaceSQPPack {

LineSearchDirect_Step::LineSearchDirect_Step(
	const direct_line_search_ptr_t& direct_line_search
	)
	:direct_line_search_(direct_line_search)
{}

bool LineSearchDirect_Step::do_step(
	Algorithm& _algo, poss_type step_poss, GeneralIterationPack::EDoStepType type
	,poss_type assoc_step_poss
	)
{
	using AbstractLinAlgPack::Vp_StV;
	using LinAlgOpPack::V_VpV;

	rSQPAlgo	&algo	= rsqp_algo(_algo);
	rSQPState	&s		= algo.rsqp_state();
	NLP			&nlp	= algo.nlp();

	EJournalOutputLevel olevel = algo.algo_cntr().journal_output_level();
	std::ostream& out = algo.track().journal_out();

	// print step header.
	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
		using GeneralIterationPack::print_algorithm_step;
		print_algorithm_step( algo, step_poss, type, assoc_step_poss, out );
	}

	const size_type
		n  = nlp.n(),
		m  = nlp.m(),
		mI = nlp.mI();

	// /////////////////////////////////////////
	// Set references to iteration quantities
	//
	// Set k+1 first then go back to get k+0 to ensure
	// we have backward storage!

	IterQuantityAccess<value_type>
		&f_iq     = s.f(),
		&alpha_iq = s.alpha(),
		&phi_iq   = s.phi();
	IterQuantityAccess<VectorWithOpMutable>
		&x_iq    = s.x(),
		&d_iq    = s.d(),
		&c_iq    = s.c(),
		&h_iq    = s.h();
	
	VectorWithOpMutable        &x_kp1   = x_iq.get_k(+1);
	const VectorWithOp         &x_k     = x_iq.get_k(0);
	value_type                 &f_kp1   = f_iq.get_k(+1);
	const value_type           &f_k     = f_iq.get_k(0);
	VectorWithOpMutable        *c_kp1   = m  ? &c_iq.get_k(+1) : NULL;
	const VectorWithOp         *c_k     = m  ? &c_iq.get_k(0)  : NULL;
	VectorWithOpMutable        *h_kp1   = mI ? &h_iq.get_k(+1) : NULL;
	const VectorWithOp         *h_k     = mI ? &h_iq.get_k(0)  : NULL;
	const VectorWithOp         &d_k     = d_iq.get_k(0);
	value_type                 &alpha_k = alpha_iq.get_k(0);

	// /////////////////////////////////////
	// Compute Dphi_k, phi_kp1 and phi_k

	const MeritFuncNLP
		&merit_func_nlp_k = s.merit_func_nlp().get_k(0);
	if( (int)olevel >= (int)PRINT_ALGORITHM_STEPS ) {
		out << "\nBegin definition of NLP merit function phi.value(f(x),c(x)):\n";
		merit_func_nlp_k.print_merit_func( out, "    " );
		out	<< "end definition of the NLP merit funciton\n";
	}
	// Dphi_k
	const value_type
		Dphi_k = merit_func_nlp_k.deriv();
	if( (int)olevel >= (int)PRINT_ALGORITHM_STEPS ) {
		out	<< "\nDphi_k = "	<< Dphi_k << std::endl;
	}
	THROW_EXCEPTION(
		Dphi_k >= 0, LineSearchFailure
		,"LineSearch2ndOrderCorrect_Step::do_step(...) : " 
		"Error, d_k is not a descent direction for the merit function "
		"since Dphi_k = " << Dphi_k << " >= 0" );
	// ph_kp1
	value_type
		&phi_kp1 = phi_iq.set_k(+1) = merit_func_nlp_k.value(
			f_kp1
			,c_kp1
			,h_kp1
			,mI ? &nlp.hl() : NULL
			,mI ? &nlp.hu() : NULL
			);
	// Must compute phi(x) at the base point x_k since the penalty parameter may have changed.
	const value_type
		&phi_k = phi_iq.set_k(0) = merit_func_nlp_k.value(
			f_k
			,c_k
			,h_k
			,mI ? &nlp.hl() : NULL
			,mI ? &nlp.hu() : NULL
			);
	
	// //////////////////////////////////////
	// Setup the calculation merit function

	// Here f_kp1, c_kp1 and h_kp1 are updated at the same time the
	// line search is being performed!
	nlp.set_f( &f_kp1 );
	if(m)  nlp.set_c( c_kp1 );
	if(mI) nlp.set_h( h_kp1 );
	MeritFuncCalcNLP
		phi_calc( &merit_func_nlp_k, &nlp );

	// //////////////////////
	// Do the line search
	
	const VectorWithOp* xd[2] = { &x_k, &d_k };
	MeritFuncCalc1DQuadratic
		phi_calc_1d( phi_calc, 1, xd, &x_kp1 );
	
	if( !direct_line_search().do_line_search(
			phi_calc_1d, phi_k, &alpha_k, &phi_kp1
			,( static_cast<int>(olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS)
			   ? &out : static_cast<std::ostream*>(0)	)
			)
		)
	{
		// The line search failed!
		if( static_cast<int>(olevel) >= static_cast<int>(PRINT_BASIC_ALGORITHM_INFO) )
			out
				<< "\nThe maximum number of linesearch iterations has been exceeded "
				<< "(k = " << algo.state().k() << ")\n"
				<< "(phi_k - phi_kp1)/phi_k = " << ((phi_k - phi_kp1)/phi_k)
				<< "\nso we will reject the step and declare a line search failure.\n";
		THROW_EXCEPTION(
			true, LineSearchFailure
			,"LineSearchDirect_Step::do_step(): Line search failure" );
	}
	
	if( (int)olevel >= (int)PRINT_ALGORITHM_STEPS ) {
		out	<< "\nalpha_k                = " << alpha_k;
		out << "\n||x_kp1||inf           = " << x_kp1.norm_inf();
		out << "\nf_kp1                  = " << f_kp1;
		if(m)
			out << "\n||c_kp1||inf           = " << c_kp1->norm_inf();
		if(mI) {
			assert(0); // Todo: Implement below with a reduction operator!
			out << "\n||min(h_kp1-hl,0)||inf = " << 0.0;
			out << "\n||max(h_kp1-hu,0)||inf = " << 0.0;
		}
		out << "\nphi_kp1                = " << phi_kp1;
		out << std::endl;
	}
	
	if( (int)olevel >= (int)PRINT_VECTORS ) {
		out << "\nx_kp1 =\n" << x_kp1;
		if(m)
			out <<"\nc_kp1 =\n" << *c_kp1;
		if(mI)
			out <<"\nh_kp1 =\n" << *h_kp1;
	}

	return true;
}

void LineSearchDirect_Step::print_step(
	const Algorithm& algo, poss_type step_poss, GeneralIterationPack::EDoStepType type, poss_type assoc_step_poss
	,std::ostream& out, const std::string& L
	) const
{
	out
		<< L << "*** Preform a line search along the full space search direction d_k.\n"
		<< L << "Dphi_k = merit_func_nlp_k.deriv()\n"
		<< L << "if Dphi_k >= 0 then\n"
		<< L << "    throw line_search_failure\n"
		<< L << "end\n"
		<< L << "phi_kp1 = merit_func_nlp_k.value(f_kp1,c_kp1,h_kp1,hl,hu)\n"
		<< L << "phi_k = merit_func_nlp_k.value(f_k,c_k,h_k,hl,hu)\n"
		<< L << "begin direct line search (where phi = merit_func_nlp_k): \"" << typeid(direct_line_search()).name() << "\"\n";
	direct_line_search().print_algorithm( out, L + "    " );
	out
		<< L << "end direct line search\n"
		<< L << "if maximum number of linesearch iterations are exceeded then\n"
		<< L << "    throw line_search_failure\n"
		<< L << "end\n";
}

} // end namespace ReducedSpaceSQPPack
