/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2009 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    (2009) kraftche@cae.wisc.edu    

  ***************************************************************** */


/** \file SizeAdaptShapeWrapper.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "SizeAdaptShapeWrapper.hpp"

#include "TerminationCriterion.hpp"
#include "InstructionQueue.hpp"
#include "QualityAssessor.hpp"
#include "MeshImpl.hpp"
#include "PlanarDomain.hpp"

#include "PMeanPTemplate.hpp"
#include "TrustRegion.hpp"
#include "TRelQualityMetric.hpp"
#include "IdealShapeTarget.hpp"
#include "TRel2DShapeSizeBarrier.hpp"
#include "TRel3DShapeSizeBarrier.hpp"
#include "RefMeshTargetCalculator.hpp"
#include "ReferenceMesh.hpp"
#include "TagVertexMesh.hpp"
#include "LambdaTarget.hpp"
#include "EdgeLengthMetric.hpp"

namespace MESQUITE_NS {

void SizeAdaptShapeWrapper::run_wrapper( Mesh* mesh, 
                                         ParallelMesh* pmesh,
                                         MeshDomain* domain, 
                                         Settings* settings,
                                         QualityAssessor* qa,
                                         MsqError& err )
{
  InstructionQueue q;
 
    // calculate average lambda for mesh
  TagVertexMesh init_mesh( err, mesh );  MSQ_ERRRTN(err);
  ReferenceMesh ref_mesh( &init_mesh );
  RefMeshTargetCalculator W_0( &ref_mesh );
  q.add_tag_vertex_mesh( &init_mesh, err );  MSQ_ERRRTN(err);
  
    // create objective function
  IdealShapeTarget W_i;
  LambdaTarget W( &W_0, &W_i );
  TRel2DShapeSizeBarrier tm2;
  TRel3DShapeSizeBarrier tm3;
  TRelQualityMetric mu( &W, &tm2, &tm3 );
  PMeanPTemplate of( 1.0, &mu );
  
    // create quality assessor
  EdgeLengthMetric len(0.0);
  qa->add_quality_assessment( &mu );
  qa->add_quality_assessment( &len );
  q.add_quality_assessor( qa, err );
  
    // create solver
  TrustRegion solver( &of );
  TerminationCriterion tc, ptc;
  tc.add_absolute_vertex_movement( maxVtxMovement );
  tc.add_iteration_limit( iterationLimit );
  ptc.add_iteration_limit( pmesh ? parallelIterations : 1 );
  solver.set_inner_termination_criterion( &tc );
  solver.set_outer_termination_criterion( &ptc );
  q.set_master_quality_improver( &solver, err ); MSQ_ERRRTN(err);
  q.add_quality_assessor( qa, err );

  // Optimize mesh
  q.run_common( mesh, pmesh, domain, settings, err ); MSQ_CHKERR(err);  
}

} // namespace MESQUITE_NS
