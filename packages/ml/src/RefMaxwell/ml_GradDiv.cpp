/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */        
/* ******************************************************************** */
#include "ml_config.h"
#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_EPETRAEXT) 
#include <string.h>
#include "ml_GradDiv.h"
#include "ml_epetra.h"
#include "ml_epetra_utils.h"
#include "ml_MultiLevelPreconditioner.h"
#include "ml_EdgeMatrixFreePreconditioner.h"
#include "ml_FaceMatrixFreePreconditioner.h"
#include "ml_ValidateParameters.h"

#include "EpetraExt_RowMatrixOut.h"
using namespace std;
using Teuchos::rcp;
using Teuchos::RCP;
using Teuchos::ArrayRCP;

#ifdef HAVE_ML_IFPACK
#include "Ifpack.h"
#endif

// ================================================ ====== ==== ==== == = 
extern double cms_compute_residual(const Epetra_Operator * op,const Epetra_MultiVector& rhs, const Epetra_MultiVector& lhs);

// ================================================ ====== ==== ==== == = 
ML_Epetra::GradDivPreconditioner::GradDivPreconditioner(const Epetra_CrsMatrix & K2_Matrix,
							const Epetra_CrsMatrix & FaceNode_Matrix,  
							const Epetra_CrsMatrix & D1_Clean_Matrix,        
							const Epetra_CrsMatrix & D0_Clean_Matrix,                  
							const Epetra_CrsMatrix & TMT_Matrix,                 
							const Teuchos::ParameterList & List,
							const bool ComputePrec):
  ML_Preconditioner(),
  K2_Matrix_(&K2_Matrix), FaceNode_Matrix_(&FaceNode_Matrix), 
  D1_Clean_Matrix_(&D1_Clean_Matrix),D0_Clean_Matrix_(&D0_Clean_Matrix), TMT_Matrix_(&TMT_Matrix),
#ifdef HAVE_ML_IFPACK
  IfSmoother(0),
#endif
  verbose_(false),very_verbose_(false)
{

  /* Set the Epetra Goodies */
  Comm_ = &(K2_Matrix_->Comm());
  DomainMap_ = &(K2_Matrix_->DomainMap());
  RangeMap_ = &(K2_Matrix_->RangeMap());
  EdgeMap_ = &(D1_Clean_Matrix_->DomainMap());

  Label_=new char [80];
  strcpy(Label_,"ML face-element grad-div preconditioner");
  List_=List;
  SetDefaultsGradDiv(List_,false);
  
#ifdef ML_TIMING
  /* Internal Timings */
  NumApplications_ = 0;
  ApplicationTime_ = 0.0;
  FirstApplication_ = true;
  FirstApplicationTime_ = 0.0;
  NumConstructions_ = 0;
  ConstructionTime_ = 0.0;
#endif

  if(ComputePrec) ML_CHK_ERRV(ComputePreconditioner());
}/*end constructor*/


// ================================================ ====== ==== ==== == = 
ML_Epetra::GradDivPreconditioner::~GradDivPreconditioner()
{
  if (IsComputePreconditionerOK_) 
    DestroyPreconditioner(); 
}/*end destructor*/


// ================================================ ====== ==== ==== == = 
// Print the individual operators in the multigrid hierarchy.
void ML_Epetra::GradDivPreconditioner::Print(int whichHierarchy){
  //HAQ
  //  if(IsComputePreconditionerOK_ && FacePC && whichHierarchy==11) FacePC->Print(-1);
  //  if(IsComputePreconditionerOK_ && EdgePC && whichHierarchy==22) EdgePC->Print(-1);  
}/*end Print*/


// ================================================ ====== ==== ==== == = 
// Computes the preconditioner
int ML_Epetra::GradDivPreconditioner::ComputePreconditioner(const bool CheckFiltering)
{
#ifdef ML_TIMING
  double t_time_start, t_time_curr, t_diff[5];
  StartTimer(&t_time_start);
  t_time_curr=t_time_start;
#endif

  int output_level=List_.get("ML output",0);
  output_level=List_.get("output",output_level);
  
  Teuchos::ParameterList dummy;

  /* Validate List */
  Teuchos::ParameterList newList;
  ML_CreateSublists(List_,newList,0);
  List_ = newList;
  // TODO: Re-add validation
  //  ValidateGradDivParameters(List_);
  
  /* Pull Solver Mode, verbosity, matrix output */
  print_hierarchy= List_.get("print hierarchy",-2);  
  int vb_level=List_.get("ML output",0);
  if(vb_level >= 15) {very_verbose_=true;verbose_=true;}
  else if (vb_level >= 5) {very_verbose_=false;verbose_=true;}
  else very_verbose_=verbose_=false;
  
  /* Nuke everything if we've done this already */
  if(IsComputePreconditionerOK_) DestroyPreconditioner();

  /* Output Header */
  if(verbose_ && !Comm_->MyPID()) {
    printf("------------------------------------------------------------------------------\n");
    printf("***\n");
    printf("*** ML_Epetra::GradDivPreconditioner [%s]\n",Label());
    printf("***\n");
  }

  /* Find the Dirichlet Rows of K2 */
  int numBCfaces,*BCfaces;
  BCfaces=FindLocalDiricheltRowsFromOnesAndZeros(*K2_Matrix_,numBCfaces);
  Epetra_IntVector* BCEdgeList=FindLocalDirichletColumnsFromRows(BCfaces,numBCfaces,*D1_Clean_Matrix_);
  ArrayRCP<int> BCfaces_(BCfaces,0,numBCfaces,true);

  /* Do the Nuking for D1_Matrix_ */ 
  D1_Matrix_ = rcp(new Epetra_CrsMatrix(*D1_Clean_Matrix_));
  Apply_BCsToMatrixRows(BCfaces,numBCfaces,*D1_Matrix_);
  Apply_BCsToMatrixColumns(*BCEdgeList,*D1_Matrix_);   
  D1_Matrix_->OptimizeStorage();

  /* Setup Edge boundary conditions */
  int numBCedges=0,*BCedges;
  for (int i=0; i < D1_Matrix_->NumMyRows(); i++)
    if((*BCEdgeList)[i]==1) numBCedges++;
  BCedges=new int[numBCedges];
  for (int i=0,edgeid=0; i < D1_Matrix_->NumMyRows(); i++)
    if((*BCEdgeList)[i]==1) BCedges[edgeid++]=i;
  delete BCEdgeList;
  ArrayRCP<int> BCedges_(BCedges,0,numBCedges,true);

  /* Do the Nuking for the D0 Matrix */
  Epetra_IntVector * BCnodes=FindLocalDirichletColumnsFromRows(BCedges,numBCedges,*D0_Clean_Matrix_);   
  int Nn=BCnodes->MyLength();
  int numBCnodes=0;
  for(int i=0;i<Nn;i++){
    if((*BCnodes)[i]) numBCnodes++;
  }
  D0_Matrix_ = rcp(new Epetra_CrsMatrix(*D0_Clean_Matrix_));
  Apply_BCsToMatrixRows(BCedges,numBCedges,*D0_Matrix_);
  Apply_BCsToMatrixColumns(*BCnodes,*D0_Matrix_);   
  D0_Matrix_->OptimizeStorage();
  delete BCnodes;


#ifdef ML_TIMING
  StopTimer(&t_time_curr,&(t_diff[0]));
#endif

  /* Generate the K1 Matrix */
  Epetra_CrsMatrix *K1;
  ML_Epetra_PtAP(*K2_Matrix_,*D1_Matrix_,K1);
  K1_Matrix_=rcp(K1);

#ifdef ML_TIMING
  StopTimer(&t_time_curr,&(t_diff[1]));
#endif
  
#ifdef HAVE_ML_IFPACK
  /* Build smoother if needed */
  string smoother=List_.get("smoother: type","IFPACK");
  if(smoother=="IFPACK" || smoother=="Chebyshev") {
    if(smoother=="IFPACK") smoother=List_.get("smoother: ifpack type","point relaxation stand-alone");

    Ifpack Factory;
    IfSmoother = Factory.Create(smoother,const_cast<Epetra_CrsMatrix*>(&*K2_Matrix_),0);
    if(!IfSmoother) ML_CHK_ERR(-6);
    IfSmoother->SetParameters(List_.sublist("smoother: ifpack list"));
    ML_CHK_ERR(IfSmoother->Compute());
    if(!Comm_->MyPID()) cout<<*IfSmoother<<endl;
  }
#endif
#ifdef ML_TIMING
  StopTimer(&t_time_curr,&(t_diff[2]));
#endif
  /* Build the (1,1) Block Preconditioner */ 
  Teuchos::ParameterList & List11=List_.sublist("graddiv: 11list");
  if (List11.name() == "ANONYMOUS") List11.setName("graddiv: 11list");
  FacePC=new FaceMatrixFreePreconditioner(rcp(K2_Matrix_,false),Teuchos::null,rcp(FaceNode_Matrix_,false),rcp(TMT_Matrix_,false),BCfaces_,List11,true);
#ifdef ML_TIMING
  StopTimer(&t_time_curr,&(t_diff[3]));
#endif
  if(print_hierarchy) FacePC->Print();

  /* Build the (2,2) Block Preconditioner */ 
  Teuchos::ParameterList & List22=List_.sublist("graddiv: 22list");
  if (List22.name() == "ANONYMOUS") List11.setName("graddiv: 22list");
  EdgePC=new EdgeMatrixFreePreconditioner(K1_Matrix_,Teuchos::null,D0_Matrix_,rcp(D0_Clean_Matrix_,false),rcp(TMT_Matrix_,false),BCedges_,List22,true);
  if(print_hierarchy) EdgePC->Print();

#ifdef ML_TIMING
  StopTimer(&t_time_curr,&(t_diff[4]));
  /* Output */
  ML_Comm *comm_;
  ML_Comm_Create(&comm_);
  int printl=ML_Get_PrintLevel();
  ML_Set_PrintLevel(output_level);  
  ReportTimer(t_diff[0],"ML_RMP::ComputePreconditioner (apply BCs     )",comm_);
  ReportTimer(t_diff[1],"ML_RMP::ComputePreconditioner (form K1       )",comm_);
  ReportTimer(t_diff[2],"ML_RMP::ComputePreconditioner (smoother      )",comm_);
  ReportTimer(t_diff[3],"ML_RMP::ComputePreconditioner (Face prec bld )",comm_);
  ReportTimer(t_diff[4],"ML_RMP::ComputePreconditioner (Edge prec bld )",comm_);
  ReportTimer(t_time_curr-t_time_start,"ML_RMP::ComputePreconditioner (total         )",comm_);
  ML_Set_PrintLevel(printl);
  ML_Comm_Destroy(&comm_);

  NumConstructions_++;
  ConstructionTime_+=t_time_curr-t_time_start;
#endif  
  
  /* Output footer */
  if(verbose_ && !Comm_->MyPID()) {
    printf("------------------------------------------------------------------------------\n");
  }

  IsComputePreconditionerOK_=true;
  return 0;
}/*end ComputePreconditioner*/






// ================================================ ====== ==== ==== == = 
// Destroys all structures allocated in \c ComputePreconditioner() if the preconditioner has been computed.
int ML_Epetra::GradDivPreconditioner::DestroyPreconditioner(){
  // NTS: Make sure this gets everything
  int printl=ML_Get_PrintLevel();
  int output_level=List_.get("ML output",0);
  output_level=List_.get("output",output_level);

  ML_Set_PrintLevel(output_level);
  if(FacePC) {delete FacePC; FacePC=0;}
  if(EdgePC) {delete EdgePC; EdgePC=0;}
  ML_Set_PrintLevel(printl);  
  D0_Matrix_=Teuchos::null;
  D1_Matrix_=Teuchos::null;
  ML_Set_PrintLevel(output_level);  
#ifdef HAVE_ML_IFPACK
  if(IfSmoother) {delete IfSmoother;IfSmoother=0;}
#endif

#ifdef ML_TIMING
  ML_Comm *comm_;
  ML_Comm_Create(&comm_);
  ReportTimer(ConstructionTime_ ,   "ML_RMP::ComputePreconditioner (construction  )",comm_);  
  ReportTimer(FirstApplicationTime_,"ML_RMP::ComputePreconditioner (1st iter time )",comm_);  
  ReportTimer(ApplicationTime_ ,    "ML_RMP::ComputePreconditioner (total itr cost)",comm_);
  ML_Set_PrintLevel(printl);  
  ML_Comm_Destroy(&comm_);
#else
  ML_Set_PrintLevel(printl);  
#endif

  return 0;
}/*end DestroyPreconditioner*/




// ================================================ ====== ==== ==== == = 
// Apply the preconditioner to an Epetra_MultiVector X, puts the result in Y
int ML_Epetra::GradDivPreconditioner::ApplyInverse(const Epetra_MultiVector& B, Epetra_MultiVector& X_) const
{
  /* Sanity Checks */
  if (!B.Map().SameAs(*DomainMap_)) ML_CHK_ERR(-1);
  if (B.NumVectors() != X_.NumVectors()) ML_CHK_ERR(-1);

  /* Check for zero RHS */
  int NumVectors=B.NumVectors();
  bool norm0=true;
  double *norm=new double[NumVectors]; 
  B.Norm2(norm);
  for(int i=0;norm0==true && i<NumVectors;i++) norm0=norm0 && (norm[i]==0);
  delete [] norm;
  if(norm0) return 0;

#ifdef ML_TIMING
  double t_time,t_diff;
  StartTimer(&t_time);
#endif

  /* Build new work vectors */
  Epetra_MultiVector X(X_.Map(),X_.NumVectors());
  X.PutScalar(0);  
  Epetra_MultiVector TempF1(X.Map(),NumVectors,false);
  Epetra_MultiVector TempF2(X.Map(),NumVectors,true);
  Epetra_MultiVector TempE1(*EdgeMap_,NumVectors,false);
  Epetra_MultiVector TempE2(*EdgeMap_,NumVectors,true);
  Epetra_MultiVector Resid(B.Map(),NumVectors);


#ifdef HAVE_ML_IFPACK
  /* Smooth if needed */
  if(IfSmoother) {ML_CHK_ERR(IfSmoother->ApplyInverse(B,X));}
#endif

  /* Build Residual */
  ML_CHK_ERR(K2_Matrix_->Multiply(false,X,TempF1));
  ML_CHK_ERR(Resid.Update(-1.0,TempF1,1.0,B,0.0));  

  /* Precondition face block (additive)*/
  ML_CHK_ERR(FacePC->ApplyInverse(Resid,TempF2));

  /* Precondition (2,2) block (additive)*/
  D1_Matrix_->Multiply(true,Resid,TempE1);
  ML_CHK_ERR(EdgePC->ApplyInverse(TempE1,TempE2));             
  D1_Matrix_->Multiply(false,TempE2,TempF1);

  /* Update X */
  X.Update(1.0,TempF1,1.0,TempF2,1.0);

#ifdef HAVE_ML_IFPACK
  /* Smooth if needed */
  if(IfSmoother) {ML_CHK_ERR(IfSmoother->ApplyInverse(B,X));}
#endif

  /* Copy work vector to output */
  X_=X;
  
  /* Timer Stuff */
#ifdef ML_TIMING
  if(FirstApplication_){
    FirstApplication_=false;
    FirstApplicationTime_=ApplicationTime_;
  }/*end if*/
  NumApplications_++;

  StopTimer(&t_time,&t_diff);

  /* Output */
  ML_Comm *comm_;
  ML_Comm_Create(&comm_);
  this->ApplicationTime_+= t_diff;  
  ML_Comm_Destroy(&comm_);
#endif

  return 0;
}/*end ApplyInverse*/



// ================================================ ====== ==== ==== == = 
int ML_Epetra::SetDefaultsGradDiv(Teuchos::ParameterList & inList,bool OverWrite)
{  
  /* Sublists */
  Teuchos::ParameterList ListRF,List11,List11c,List22,List22c,ListIf;
  Teuchos::ParameterList & List11_=inList.sublist("graddiv: 11list");
  Teuchos::ParameterList & List22_=inList.sublist("graddiv: 22list");
  Teuchos::ParameterList & List11c_=List11_.sublist("face matrix free: coarse");
  Teuchos::ParameterList & List22c_=List22_.sublist("edge matrix free: coarse");
  Teuchos::ParameterList & ListIf_=inList.sublist("smoother: ifpack list");

  /* (1,1) coarse */
  ML_Epetra::SetDefaults("SA",List11c);
  List11c.set("smoother: type","Chebyshev");
  List11c.set("aggregation: threshold",.01);
  List11c.set("smoother: sweeps",2);
  List11c.set("coarse: type","Amesos-KLU");  
  List11c.set("ML label","coarse face block");
  ML_Epetra::UpdateList(List11c,List11c_,OverWrite); 

  /* (2,2) coarse */
  ML_Epetra::SetDefaults("SA",List22c);
  List22c.set("smoother: type","Chebyshev");
  List22c.set("aggregation: threshold",.01);
  List22c.set("smoother: sweeps",2);
  //  List22c.set("coarse: type","symmetric Gauss-Seidel");  
  //  List22c.set("coarse: sweeps",6);
  List22c.set("ML label","coarse edge block");
  ML_Epetra::UpdateList(List22c,List22c_,OverWrite);

  /* (1,1) */
  ML_Epetra::SetDefaults("SA",List11);
  List11.set("smoother: type","do-nothing");
  List11.set("aggregation: type","Uncoupled");
  List11.set("smoother: sweeps",0);
  List11.set("aggregation: threshold",.01);  
  List11.set("ML label","face matrix free");
  ML_Epetra::UpdateList(List11,List11_,OverWrite);
  
  /* (2,2) */
  ML_Epetra::SetDefaults("SA",List22);
  List11.set("smoother: type","Chebyshev");
  List22.set("aggregation: type","Uncoupled");
  List22.set("smoother: sweeps",2);
  List22.set("aggregation: threshold",.01);  
  List22.set("ML label","edge matrix free");
  ML_Epetra::UpdateList(List22,List22_,OverWrite);

  /* Ifpack list for overall */
  ListIf.set("relaxation: type","symmetric Gauss-Seidel");
  ListIf.set("relaxation: sweeps",2);
  ListIf.set("relaxation: zero starting solution",false);
  ML_Epetra::UpdateList(ListIf,ListIf_,OverWrite);

  /* Build Teuchos List: Overall */  
  SetDefaults("SA",ListRF,0,0,false);
  ListRF.set("smoother: ifpack type","point relaxation stand-alone");
  ListRF.set("smoother: type","IFPACK");

  ML_Epetra::UpdateList(ListRF,inList,OverWrite);
  return 0;
}/*end SetDefaultsGradDiv*/


#endif