// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-

/*! \file MeanRatioQualityMetric.hpp

Header file for the Mesquite::MeanRatioQualityMetric class

  \author Michael Brewer
  \date   2002-06-19
 */


#ifndef MeanRatioQualityMetric_hpp
#define MeanRatioQualityMetric_hpp
#include "MsqMeshEntity.hpp"
#include "Mesquite.hpp"
#include "MesquiteError.hpp"
#include "ShapeQualityMetric.hpp"
#include "Vector3D.hpp"
#include "PatchData.hpp"
//Michael delete
#include "MsqMessage.hpp"

namespace Mesquite
{
     /*! \class MeanRatioQualityMetric
       \brief Computes the mean ratio of given element.
       
     */
   class MeanRatioQualityMetric : public ShapeQualityMetric
   {
  public:
 
       /*!Returns a pointer to a ShapeQualityMetric.  The metric
         does not use the sample point functionality or the
         compute_weighted_jacobian.  It evaluates the metric at
         the element vertices, and uses the isotropic ideal element.
         It does require a feasible region, and the metric needs
         to be minimized.
       */
     static ShapeQualityMetric* create_new(){
       
       ShapeQualityMetric* m = new MeanRatioQualityMetric();
       return m;
     }
     
       //! virtual destructor ensures use of polymorphism during destruction
     virtual ~MeanRatioQualityMetric()
        {}
     
       //! evaluate using mesquite objects 
     double evaluate_element(PatchData &pd, MsqMeshEntity *element,
                             MsqError &err); 
          
  protected:     
     double mean_ratio_2d(Vector3D temp_vec[],MsqError &err);
     double mean_ratio_3d(Vector3D temp_vec[],MsqError &err);
  private:
     
     MeanRatioQualityMetric();
    
  };
     //BEGIN INLINE FUNCITONS
   inline double MeanRatioQualityMetric::mean_ratio_2d(Vector3D temp_vec[],
                                                       MsqError &err)
   {
       //NOTE:  We take absolute value here
     double area_val=fabs((temp_vec[0]*temp_vec[1]).length()*2.0);
     double return_val=(temp_vec[0].length_squared()+
                        temp_vec[1].length_squared());
     if(area_val>MSQ_MIN){
       return_val/=area_val;
     }
     else{
       return_val=MSQ_MAX_CAP;
     }
     return return_val;
   }

   inline double MeanRatioQualityMetric::mean_ratio_3d(Vector3D temp_vec[],
                                                       MsqError &err)
   {   
     double term1=temp_vec[0]%temp_vec[0]+
        temp_vec[1]%temp_vec[1]+
        temp_vec[2]%temp_vec[2];
       //det of J
     double temp_vol_sqr=temp_vec[0]%(temp_vec[1]*temp_vec[2]);
     temp_vol_sqr*=temp_vol_sqr;
     double return_val=MSQ_MAX_CAP;
     if(temp_vol_sqr>MSQ_MIN){
         //if not degenerate or inverted???
       return_val=term1/(3*(pow(temp_vol_sqr,(1.0/3.0))));
     }
     return return_val;
   }
   
   

} //namespace


#endif // MeanRatioQualityMetric_hpp


