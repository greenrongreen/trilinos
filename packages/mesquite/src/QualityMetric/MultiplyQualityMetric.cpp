/*!
  \file   MultiplyQualityMetric.cpp
  \brief 

  \author Michael Brewer
  \date   2002-05-09
*/

#include "MultiplyQualityMetric.hpp"
#include "QualityMetric.hpp"
#include "Vector3D.hpp"
#include "MsqMessage.hpp"

using namespace Mesquite;
#undef __FUNC__
#define __FUNC__ "MultiplyQualityMetric::MultiplyQualityMetric"

MultiplyQualityMetric::MultiplyQualityMetric(QualityMetric* qm1, QualityMetric* qm2, MsqError &err){
  if(qm1 == NULL || qm2 == NULL){
    err.set_msg("MultiplyQualityMetric constructor passed NULL pointer.");
  }
  set_qmetric2(qm2);
  set_qmetric1(qm1);
  feasible=qm1->get_feasible_constraint();
  if(qm2->get_feasible_constraint())
    feasible=qm2->get_feasible_constraint();
  int n_flag=qm1->get_negate_flag();
  if(n_flag!=qm2->get_negate_flag()){
    PRINT_WARNING("MultiplyQualityMetric is being used to compose a metric that should be minimized\n with a metric that should be maximized.");
    set_negate_flag(1);
  }
  else{
    set_negate_flag(n_flag);
  }

  // Checks that metrics are of the same type 
  if ( qm1->get_metric_type() != qm2->get_metric_type() ) {
    err.set_msg("Cannot multiply a vertex-based QM with an element-based QM.");
  } else {
    set_metric_type(qm1->get_metric_type());
  }
  
  set_name("Composite Multiply");
}

#undef __FUNC__
#define __FUNC__ "MultiplyQualityMetric::evaluate_element"

/*! Returns qMetric1->evaluate_element(element, err) multiplied by
  qMetric2-evaluate_element(element, err)*/
double MultiplyQualityMetric::evaluate_element(PatchData& pd,
                                               MsqMeshEntity *element,
                                               MsqError &err)
{
  double metric1, metric2, total_metric;  
  get_qmetric1()->evaluate_element(pd, element, metric1, err);
  get_qmetric2()->evaluate_element(pd, element, metric2, err); 
  total_metric = metric1*metric2;
  return total_metric;
}

#undef __FUNC__
#define __FUNC__ "MultiplyQualityMetric::evaluate_vertex"
/*! Returns qMetric1->evaluate_vertex(...) multiplied by
  qMetric2-evaluate_vertex(...)*/
double MultiplyQualityMetric::evaluate_vertex(PatchData& pd,
                                              MsqVertex* vert,
                                              MsqError& err)
{
  double metric1, metric2, total_metric;  
  get_qmetric1()->evaluate_vertex(pd, vert, metric1, err);
  get_qmetric2()->evaluate_vertex(pd, vert, metric2, err);
  total_metric = metric1*metric2;
  return total_metric;
}



