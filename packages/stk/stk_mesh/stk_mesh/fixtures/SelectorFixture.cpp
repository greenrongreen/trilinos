/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <stk_mesh/fixtures/SelectorFixture.hpp>
#include <sstream>                      // for ostringstream, etc
#include <string>                       // for string
#include "mpi.h"                        // for MPI_COMM_WORLD
#include "stk_mesh/base/BulkData.hpp"   // for BulkData
#include "stk_mesh/base/MetaData.hpp"   // for MetaData, put_field
#include "stk_mesh/base/Types.hpp"      // for EntityRank, EntityId, etc
#include "stk_topology/topology.hpp"    // for topology, etc
namespace stk { namespace mesh { class Part; } }

namespace stk {
namespace mesh {
namespace fixtures {

SelectorFixture::~SelectorFixture() {}

SelectorFixture::SelectorFixture()
  : m_meta_data( 0 /*dim*/ )
  , m_bulk_data( m_meta_data , MPI_COMM_WORLD )
  , m_partA( m_meta_data.declare_part( "PartA" , 0 ) )
  , m_partB( m_meta_data.declare_part( "PartB" , 0 ) )
  , m_partC( m_meta_data.declare_part( "PartC" , 0 ) )
  , m_partD( m_meta_data.declare_part( "PartD" , 0 ) )
  , m_entity1( )
  , m_entity2( )
  , m_entity3( )
  , m_entity4( )
  , m_entity5( )
  , m_fieldA(m_meta_data.declare_field<stk::mesh::Field<double> >(stk::topology::NODE_RANK, "FieldA"))
  , m_fieldABC(m_meta_data.declare_field<stk::mesh::Field<double> >(stk::topology::NODE_RANK, "FieldABC"))
{
  stk::mesh::put_field(m_fieldA, m_partA);

  stk::mesh::put_field(m_fieldABC, m_partA);
  stk::mesh::put_field(m_fieldABC, m_partB);
  stk::mesh::put_field(m_fieldABC, m_partC);
}

void SelectorFixture::generate_mesh()
{
  const unsigned entity_count = 5 ;

  stk::mesh::EntityRank ent_type = stk::topology::NODE_RANK;

  // Create Entities and assign to parts:
  stk::mesh::EntityId ent_id =
    1 + entity_count * m_bulk_data.parallel_rank(); // Unique ID

  std::vector<stk::mesh::Part*> partMembership;

  // Entity1 is contained in PartA
  partMembership.clear();
  partMembership.push_back( & m_partA );
  m_entity1 = m_bulk_data.declare_entity(ent_type, ent_id, partMembership);
  ++ent_id;

  // Entity2 is contained in PartA and PartB
  partMembership.clear();
  partMembership.push_back( & m_partA );
  partMembership.push_back( & m_partB );
  m_entity2 = m_bulk_data.declare_entity(ent_type, ent_id, partMembership);
  ++ent_id;

  // Entity3 is contained in PartB and PartC
  partMembership.clear();
  partMembership.push_back( & m_partB );
  partMembership.push_back( & m_partC );
  m_entity3 = m_bulk_data.declare_entity(ent_type, ent_id, partMembership);
  ++ent_id;

  // Entity4 is contained in PartC
  partMembership.clear();
  partMembership.push_back( & m_partC );
  m_entity4 = m_bulk_data.declare_entity(ent_type, ent_id, partMembership);
  ++ent_id;

  // Entity5 is not contained in any Part
  partMembership.clear();
  m_entity5 = m_bulk_data.declare_entity(ent_type, ent_id, partMembership);
}

//--------------------------------------------------------------------------

VariableSelectorFixture::~VariableSelectorFixture() {}

VariableSelectorFixture::VariableSelectorFixture(int NumParts)
  : m_MetaData( 0 /*dim*/, std::vector<std::string>(4, std::string("MyEntityRank")) )
  , m_BulkData( m_MetaData , MPI_COMM_WORLD )
  , m_declared_part_vector()
{
  // Create Parts and commit:
  std::string myPartName;
  stk::mesh::EntityRank myRank = stk::topology::NODE_RANK;

  std::string partName = "Part_";
  for (int part_i=0 ; part_i<NumParts; ++part_i) {
    std::ostringstream localPartName(partName);
    localPartName << part_i;
    stk::mesh::Part * part =
      & m_MetaData.declare_part(localPartName.str(),myRank);
    m_declared_part_vector.push_back( part );
  }

  m_MetaData.commit();

  // Create Entities and assign to parts:

  m_BulkData.modification_begin();

  stk::mesh::EntityRank ent_type = stk::topology::NODE_RANK;
  stk::mesh::EntityId ent_id =
    1 + NumParts * m_BulkData.parallel_rank(); // Unique ID

  for (int part_i = 0 ; part_i < NumParts ; ++part_i) {
    std::vector<stk::mesh::Part*> partMembership;
    partMembership.push_back(m_declared_part_vector[part_i]);
    stk::mesh::Entity e = m_BulkData.declare_entity(ent_type, ent_id, partMembership);
    m_entities.push_back( e );
    ++ent_id;
  }

  m_BulkData.modification_end();
}

} // fixtures
} // mesh
} // stk
