#include "CLArgs.hpp"
#include "QualityAssessor.hpp"
#include "IdealWeightInverseMeanRatio.hpp"
#include "SizeMetric.hpp"
#include "TRelQualityMetric.hpp"
#include "IdealShapeTarget.hpp"
#include "TRel3DShape.hpp"
#include "TRel2DShape.hpp"
#include "InstructionQueue.hpp"
#include "MsqError.hpp"
#include "MeshImpl.hpp"
#include "QuadLagrangeShape.hpp"
#include "TetLagrangeShape.hpp"
#include "TriLagrangeShape.hpp"
#include "ElementMaxQM.hpp"

using namespace Mesquite;

#include <vector>
#include <algorithm>

int main( int argc, char* argv[] )
{
  int two;
  CLArgs::ToggleArg freeonly;
  CLArgs::IntRangeArg histogram( &two );
  CLArgs args( "msqquality", "Assess mesh quality",
               "Caculate various quality metrics for a mesh," 
               "and optinally export a VTK file for which quality "
               "values are stored as attribute data." );
  args.int_flag( 'H', "ints", "Print histograms with specified number of intervals", &histogram );
  args.toggle_flag( 'f', "Assess quality only for elements with at least one free vertex", &freeonly );
  args.add_required_arg( "input_file" );
  args.add_optional_arg( "output_file" );
  std::vector<std::string> files;
  if (!args.parse_options( argc, argv, files, std::cerr )) {
    args.print_usage( std::cerr );
    return 1;
  }
  
  MsqError err;
  MeshImpl mesh;
  mesh.read_vtk( files.front().c_str(), err );
  if (err) {
    std::cerr << err << std::endl 
                    << "Failed to read file: " << files.front() << std::endl;
    return 2;
  }

  QualityAssessor qa( true, freeonly.value(), "INVERTED" );
  IdealWeightInverseMeanRatio imr;
  SizeMetric size;
  IdealShapeTarget tc;
  TRel3DShape tm_3d;
  TRel2DShape tm_2d;
  TRelQualityMetric tmp( &tc, &tm_2d, &tm_3d );
  ElementMaxQM max_tmp( &tmp );
  
  int intervals = histogram.seen() ? histogram.value() : 0;
  qa.add_quality_assessment( &imr, intervals, 0.0, "InverseMeanRatio" );
  qa.add_quality_assessment( &size, intervals, 0.0, "Size" );
  qa.add_quality_assessment( &max_tmp, intervals, 0.0, "TMP_Shape" );
  qa.tag_fixed_elements( "FIXED_ELEMS" );

  QuadLagrangeShape quad;
  TriLagrangeShape tri;
  TetLagrangeShape tet;
  InstructionQueue q;
  q.set_mapping_function( &quad );
  q.set_mapping_function( &tri );
  q.set_mapping_function( &tet );
  
  q.add_quality_assessor( &qa, err );
  q.run_instructions( &mesh, err );
  if (err) {
    std::cerr << err << std::endl;
    return 3;
  }
  
  if (files.size() > 1) {
    mesh.write_vtk( files[1].c_str(), err );
    if (err) {
      std::cerr << err << std::endl 
                      << "Failed to write file: " << files[1] << std::endl;
      return 2;
    }
  }
  
  return 0;
}
