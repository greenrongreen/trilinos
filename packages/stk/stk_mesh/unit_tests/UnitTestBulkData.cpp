/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <stddef.h>                     // for size_t
#include <stdlib.h>                     // for exit
#include <exception>                    // for exception
#include <iostream>                     // for basic_ostream::operator<<, etc
#include <iterator>                     // for distance
#include <map>                          // for _Rb_tree_const_iterator
#include <stdexcept>                    // for logic_error, runtime_error
#include <stk_mesh/base/BulkData.hpp>   // for BulkData, etc
#include <stk_mesh/base/FieldParallel.hpp>  // for communicate_field_data, etc
#include <stk_mesh/base/GetEntities.hpp>  // for count_entities, etc
#include <stk_mesh/fixtures/BoxFixture.hpp>  // for BoxFixture
#include <stk_mesh/fixtures/HexFixture.hpp>  // for HexFixture, etc
#include <stk_mesh/fixtures/QuadFixture.hpp>  // for QuadFixture
#include <stk_mesh/fixtures/RingFixture.hpp>  // for RingFixture
#include <stk_util/parallel/Parallel.hpp>  // for ParallelMachine, etc
#include <stk_util/unit_test_support/stk_utest_macros.hpp>
#include <string>                       // for string, basic_string, etc
#include <unit_tests/UnitTestModificationEndWrapper.hpp>
#include <unit_tests/UnitTestRingFixture.hpp>  // for test_shift_ring
#include <utility>                      // for pair
#include <vector>                       // for vector, etc
#include "gtest/gtest.h"                // for AssertHelper
#include "mpi.h"                        // for MPI_Barrier, MPI_COMM_WORLD, etc
#include "stk_mesh/base/Bucket.hpp"     // for Bucket, has_superset
#include "stk_mesh/base/Entity.hpp"     // for Entity
#include "stk_mesh/base/EntityKey.hpp"  // for EntityKey
#include "stk_mesh/base/Field.hpp"      // for Field
#include "stk_mesh/base/FieldBase.hpp"  // for field_data, etc
#include "stk_mesh/base/Ghosting.hpp"   // for Ghosting
#include "stk_mesh/base/MetaData.hpp"   // for MetaData, entity_rank_names, etc
#include "stk_mesh/base/Part.hpp"       // for Part
#include "stk_mesh/base/Relation.hpp"
#include "stk_mesh/base/Selector.hpp"   // for Selector, operator|
#include "stk_mesh/base/Types.hpp"      // for EntityProc, EntityVector, etc
#include "stk_topology/topology.hpp"    // for topology, etc
#include "stk_util/util/PairIter.hpp"   // for PairIter
namespace stk { namespace mesh { class FieldBase; } }







using stk::mesh::Part;
using stk::mesh::MetaData;
using stk::mesh::BulkData;
using stk::mesh::Selector;
using stk::mesh::PartVector;
using stk::mesh::BaseEntityRank;
using stk::mesh::PairIterRelation;
using stk::mesh::EntityProc;
using stk::mesh::Entity;
using stk::mesh::EntityId;
using stk::mesh::EntityKey;
using stk::mesh::EntityVector;
using stk::mesh::EntityRank;
using stk::mesh::fixtures::RingFixture;
using stk::mesh::fixtures::BoxFixture;

namespace {

const EntityRank NODE_RANK = stk::topology::NODE_RANK;

void donate_one_element( BulkData & mesh , bool aura )
{
  const int p_rank = mesh.parallel_rank();

  Selector select_owned( MetaData::get(mesh).locally_owned_part() );

  std::vector<unsigned> before_count ;
  std::vector<unsigned> after_count ;

  count_entities( select_owned , mesh , before_count );

  // Change owner of an element on a process boundary
  // from P0 to P1, and then recount to confirm ownership change

  std::vector<EntityProc> change ;

  // A shared node:
  EntityKey node_key;
  Entity elem = Entity();

  for ( stk::mesh::EntityCommListInfoVector::const_iterator
        i =  mesh.comm_list().begin() ;
        i != mesh.comm_list().end() ; ++i ) {
    if ( mesh.in_shared( i->key ) && i->key.rank() == BaseEntityRank ) {
      node_key = i->key;
      break;
    }
  }

  STKUNIT_ASSERT( node_key.is_valid() );

  Entity node = mesh.get_entity(node_key);
  STKUNIT_ASSERT( mesh.is_valid(node) );

  Entity const *node_elems_i = mesh.begin_elements(node);
  Entity const *node_elems_e = mesh.end_elements(node);
  for ( ; (node_elems_i != node_elems_e) && !mesh.is_valid(elem); ++node_elems_i)
  {
    elem = *node_elems_i;
    if ( mesh.parallel_owner_rank(elem) != p_rank ) { elem = Entity(); }
  }

  STKUNIT_ASSERT( mesh.is_valid(elem) );

  unsigned donated_nodes = 0 ;

  // Only process #0 donates an element and its owned nodes:
  if ( 0 == p_rank ) {
    EntityProc entry ;
    entry.first = elem ;
    entry.second = mesh.entity_comm_sharing(mesh.entity_key(node))[0].proc;
    change.push_back( entry );

    Entity const *elem_nodes_i = mesh.begin_nodes(elem);
    Entity const *elem_nodes_e = mesh.end_nodes(elem);
    for ( ; elem_nodes_i != elem_nodes_e; ++elem_nodes_i)
    {
      if ( mesh.parallel_owner_rank(*elem_nodes_i) == p_rank ) {
        entry.first = *elem_nodes_i;
        change.push_back( entry );
        ++donated_nodes ;
      }
    }
  }

  STKUNIT_ASSERT( mesh.modification_begin() );
  mesh.change_entity_owner( change );
  STKUNIT_ASSERT( stk::unit_test::modification_end_wrapper( mesh , aura ) );

  count_entities( select_owned , mesh , after_count );

  if ( 0 == p_rank ) {
    STKUNIT_ASSERT_EQUAL( before_count[3] - 1 , after_count[3] );
    STKUNIT_ASSERT_EQUAL( before_count[0] - donated_nodes, after_count[0] );
  }
}

void donate_all_shared_nodes( BulkData & mesh , bool aura )
{
  const int p_rank = mesh.parallel_rank();

  const Selector select_used = MetaData::get(mesh).locally_owned_part() |
                               MetaData::get(mesh).globally_shared_part() ;

  std::vector<unsigned> before_count ;
  std::vector<unsigned> after_count ;

  count_entities( select_used , mesh , before_count );

  // Donate owned shared nodes to first sharing process.

  const stk::mesh::EntityCommListInfoVector & entity_comm = mesh.comm_list();

  STKUNIT_ASSERT( ! entity_comm.empty() );

  std::vector<EntityProc> change ;

  for ( stk::mesh::EntityCommListInfoVector::const_iterator
        i =  entity_comm.begin() ;
        i != entity_comm.end() &&
        i->key.rank() == BaseEntityRank ; ++i ) {
    Entity const node = i->entity;
    const stk::mesh::PairIterEntityComm ec = mesh.entity_comm_sharing(i->key);

    if ( mesh.parallel_owner_rank(node) == p_rank && ! ec.empty() ) {
      change.push_back( EntityProc( node , ec->proc ) );
    }
  }

  STKUNIT_ASSERT( mesh.modification_begin() );
  mesh.change_entity_owner( change );
  STKUNIT_ASSERT( stk::unit_test::modification_end_wrapper( mesh , aura ) );

  count_entities( select_used , mesh , after_count );

  STKUNIT_ASSERT( 3 <= after_count.size() );
  STKUNIT_ASSERT_EQUAL( before_count[0] , after_count[0] );
  STKUNIT_ASSERT_EQUAL( before_count[1] , after_count[1] );
  STKUNIT_ASSERT_EQUAL( before_count[2] , after_count[2] );
  STKUNIT_ASSERT_EQUAL( before_count[3] , after_count[3] );
}

} // empty namespace

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testBulkData)
{
  // Unit test the Part functionality in isolation:

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  std::vector<std::string> entity_names(5);
  for ( size_t i = 0 ; i < 5 ; ++i ) {
    std::ostringstream name ;
    name << "EntityRank" << i ;
    entity_names[i] = name.str();
  }

  unsigned spatial_dim = 3;
  MetaData meta( spatial_dim, entity_names );

  meta.commit();

  BulkData bulk( meta , pm , 100 );

  for ( size_t i = 0 ; i < 4 ; ++i ) {
    STKUNIT_ASSERT( bulk.modification_begin() );
    STKUNIT_ASSERT_EQUAL( i , bulk.synchronized_count() );
    STKUNIT_ASSERT( bulk.modification_end() );
  }

  std::vector<Part*> no_parts ;

  Entity e[5] ;

  const unsigned id = bulk.parallel_rank() + 1 ;

  STKUNIT_ASSERT( bulk.modification_begin() );
  for ( size_t i = 0 ; i < 5 ; ++i ) {
    e[i] = bulk.declare_entity(  static_cast<EntityRank>(i) , id , no_parts );
  }
  STKUNIT_ASSERT( bulk.modification_end() );

  for ( size_t i = 0 ; i < 5 ; ++i ) {
    STKUNIT_ASSERT( e[i] == bulk.get_entity( static_cast<EntityRank>(i) , id ) );
  }

  STKUNIT_ASSERT( bulk.modification_begin() );
  STKUNIT_ASSERT_THROW( bulk.declare_entity( static_cast<EntityRank>(11) , id , no_parts ),
                        std::logic_error );
  STKUNIT_ASSERT( bulk.modification_end() );

  // Catch not-ok-to-modify
  STKUNIT_ASSERT_THROW( bulk.declare_entity( stk::topology::NODE_RANK , id + 1 , no_parts ),
                        std::logic_error );
}

//----------------------------------------------------------------------
// Testing for mesh entities without relations

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeOwner_nodes)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  enum { nPerProc = 10 };
  const int p_rank = stk::parallel_machine_rank( pm );
  const int p_size = stk::parallel_machine_size( pm );
  const unsigned id_total = nPerProc * p_size ;
  const unsigned id_begin = nPerProc * p_rank ;
  const unsigned id_end   = nPerProc * ( p_rank + 1 );

  const int spatial_dimension = 3;
  MetaData meta( spatial_dimension );
  BulkData bulk( meta , pm , 100 );

  const PartVector no_parts ;

  meta.commit();
  bulk.modification_begin();

  // Ids for all entities (all entities have type 0):

  std::vector<EntityId> ids( id_total );

  for ( unsigned i = 0 ; i < id_total ; ++i ) {
    ids[i] = i + 1;
  }

  // Declare just those entities in my range of ids:

  for ( unsigned i = id_begin ; i < id_end ; ++i ) {
    bulk.declare_entity( stk::topology::NODE_RANK , ids[i] , no_parts );
  }

  STKUNIT_ASSERT( bulk.modification_end() );

  // Verify that I only have entities in my range:

  for ( unsigned i = 0 ; i < id_total ; ++i ) {
    Entity e = bulk.get_entity( stk::topology::NODE_RANK , ids[ i ] );
    if ( id_begin <= i && i < id_end ) {
      STKUNIT_ASSERT( bulk.is_valid(e) );
    }
    else {
      STKUNIT_ASSERT( !bulk.is_valid(e) );
    }
  }

  // Test change owner no-op first:

  std::vector<EntityProc> change ;

  STKUNIT_ASSERT( bulk.modification_begin() );
  bulk.change_entity_owner( change );
  STKUNIT_ASSERT( bulk.modification_end() );

  for ( unsigned i = 0 ; i < id_total ; ++i ) {
    Entity e = bulk.get_entity( stk::topology::NODE_RANK , ids[ i ] );
    if ( id_begin <= i && i < id_end ) {
      STKUNIT_ASSERT( bulk.is_valid(e) );
    }
    else {
      STKUNIT_ASSERT( !bulk.is_valid(e) );
    }
  }

  // Can only test changing owner in parallel.

  if ( 1 < p_size ) {
    // Give my last two ids to the next process
    // Get the previous process' last two ids

    const int p_give = ( p_rank + 1 ) % p_size ;
    const unsigned id_give = id_end - 2 ;
    const unsigned id_get  = ( id_begin + id_total - 2 ) % id_total ;

    STKUNIT_ASSERT( bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_give] )) );
    STKUNIT_ASSERT( bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_give+1] )) );
    STKUNIT_ASSERT( !bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_get] )) );
    STKUNIT_ASSERT( !bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_get+1] )) );

    change.resize(2);
    change[0].first = bulk.get_entity( stk::topology::NODE_RANK , ids[id_give] );
    change[0].second = p_give ;
    change[1].first = bulk.get_entity( stk::topology::NODE_RANK , ids[id_give+1] );
    change[1].second = p_give ;

    STKUNIT_ASSERT( bulk.modification_begin() );
    bulk.change_entity_owner( change );
    STKUNIT_ASSERT( bulk.modification_end() );

    STKUNIT_ASSERT( bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_get] )) );
    STKUNIT_ASSERT( bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_get+1] )) );

    // Entities given away are destroyed until the next modification cycle
    {
      Entity const e0 = bulk.get_entity( stk::topology::NODE_RANK , ids[id_give] );
      Entity const e1 = bulk.get_entity( stk::topology::NODE_RANK , ids[id_give+1] );
      STKUNIT_ASSERT( !bulk.is_valid(e0) );
      STKUNIT_ASSERT( !bulk.is_valid(e1) );
    }

    STKUNIT_ASSERT( bulk.modification_begin() );
    STKUNIT_ASSERT( bulk.modification_end() );

    STKUNIT_ASSERT( !bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_give] )) );
    STKUNIT_ASSERT( !bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_give+1] )) );
  }
}

//----------------------------------------------------------------------
// Testing for creating existing mesh entities without relations

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testCreateMore)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  enum { nPerProc = 10 };

  const int p_size = stk::parallel_machine_size( pm );
  const int p_rank = stk::parallel_machine_rank( pm );

  if ( 1 < p_size ) {

    const unsigned id_total = nPerProc * p_size ;
    const unsigned id_begin = nPerProc * p_rank ;
    const unsigned id_end   = nPerProc * ( p_rank + 1 );

    const int spatial_dimension = 3;
    MetaData meta( spatial_dimension );

    const PartVector no_parts ;

    meta.commit();

    BulkData bulk( meta , pm , 100 );

    bulk.modification_begin();

    // Ids for all entities (all entities have type 0):

    std::vector<EntityId> ids( id_total );

    for ( unsigned i = 0 ; i < id_total ; ++i ) { ids[i] = i + 1; }

    // Declare just those entities in my range of ids:

    for ( unsigned i = id_begin ; i < id_end ; ++i ) {
      bulk.declare_entity( stk::topology::NODE_RANK , ids[i] , no_parts );
    }

    STKUNIT_ASSERT( bulk.modification_end() );

    // Only one process create entities with previous process' last two ids

    const unsigned id_get  = ( id_begin + id_total - 2 ) % id_total ;

    STKUNIT_ASSERT( !bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_get] )) );
    STKUNIT_ASSERT( !bulk.is_valid(bulk.get_entity( stk::topology::NODE_RANK , ids[id_get+1] )) );

    STKUNIT_ASSERT( bulk.modification_begin() );

    if ( 1 == p_rank ) {
      // These declarations create entities that already exist,
      // which will be an error.  Must create an owned entity
      // to use them, thus they become shared.

      Entity e0 = bulk.declare_entity( stk::topology::NODE_RANK , ids[ id_get ] , no_parts );
      Entity e1 = bulk.declare_entity( stk::topology::NODE_RANK , ids[ id_get + 1 ] , no_parts );

      Entity eU = bulk.declare_entity( stk::topology::EDGE_RANK , 1 , no_parts );

      bulk.declare_relation( eU , e0 , 0 );
      bulk.declare_relation( eU , e1 , 1 );
    }

    bulk.modification_end();

    if ( 1 == p_rank ) {
      Entity e0 = bulk.get_entity( stk::topology::NODE_RANK , ids[id_get] );
      Entity e1 = bulk.get_entity( stk::topology::NODE_RANK , ids[id_get+1] );
      STKUNIT_ASSERT( bulk.is_valid(e0) );
      STKUNIT_ASSERT( bulk.is_valid(e1) );
      STKUNIT_ASSERT( 0 == bulk.parallel_owner_rank(e0) );
      STKUNIT_ASSERT( 0 == bulk.parallel_owner_rank(e1) );
    }

    // Now test tripping the error condition

    bulk.modification_begin();

    if ( 0 == p_rank ) {
      bulk.declare_entity( stk::topology::NODE_RANK , ids[ id_get ] , no_parts );
      bulk.declare_entity( stk::topology::NODE_RANK , ids[ id_get + 1 ] , no_parts );
    }

    STKUNIT_ASSERT_THROW( bulk.modification_end() , std::runtime_error );
  }
}

//----------------------------------------------------------------------
STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testBulkDataRankBeginEnd)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  const int p_size = stk::parallel_machine_size( pm );
  if (p_size != 1) {
    return;
  }

  const size_t spatial_dim = 3;
  MetaData meta(spatial_dim, stk::mesh::entity_rank_names());
  BulkData bulk(meta, pm);
  bulk.modification_begin();
  BulkData::const_entity_iterator iter = bulk.begin_entities(stk::topology::NODE_RANK);
  BulkData::const_entity_iterator end = bulk.end_entities(stk::topology::NODE_RANK);

  STKUNIT_ASSERT(iter == end);

  EntityId node_id = 1;
  bulk.declare_entity(stk::topology::NODE_RANK, node_id);

  iter = bulk.begin_entities(stk::topology::NODE_RANK);
  end = bulk.end_entities(stk::topology::NODE_RANK);

  //insist that there is 1 node:
  STKUNIT_ASSERT(iter != end);
  STKUNIT_ASSERT(std::distance(iter,end) == 1u);

  //now declare an edge...
  EntityId edge_id = 1;
  bulk.declare_entity(stk::topology::EDGE_RANK, edge_id);

  iter = bulk.begin_entities(stk::topology::NODE_RANK);
  end = bulk.end_entities(stk::topology::NODE_RANK);

  //insist that there is still 1 node:
  STKUNIT_ASSERT(iter != end);
  STKUNIT_ASSERT(std::distance(iter,end) == 1u);

  iter = bulk.begin_entities(stk::topology::EDGE_RANK);
  end = bulk.end_entities(stk::topology::EDGE_RANK);

  //insist that there is 1 edge:
  STKUNIT_ASSERT(iter != end);
  STKUNIT_ASSERT(std::distance(iter,end) == 1u);

  node_id = 2;
  bulk.declare_entity(stk::topology::NODE_RANK, node_id);

  iter = bulk.begin_entities(stk::topology::NODE_RANK);
  end = bulk.end_entities(stk::topology::NODE_RANK);

  //insist that there are 2 nodes:
  STKUNIT_ASSERT(iter != end);
  STKUNIT_ASSERT(std::distance(iter,end) == 2u);

  iter = bulk.begin_entities(stk::topology::EDGE_RANK);
  end = bulk.end_entities(stk::topology::EDGE_RANK);

  //insist that there is still 1 edge:
  STKUNIT_ASSERT(iter != end);
  STKUNIT_ASSERT(std::distance(iter,end) == 1u);

  iter = bulk.begin_entities(stk::topology::FACE_RANK);
  end = bulk.end_entities(stk::topology::FACE_RANK);

  //insist that there are no faces:
  STKUNIT_ASSERT(iter == end);

  bulk.modification_end();
}
//----------------------------------------------------------------------

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeOwner_ring)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  enum { nPerProc = 10 };
  const int p_rank = stk::parallel_machine_rank( pm );
  const int p_size = stk::parallel_machine_size( pm );
  const unsigned nLocalNode = nPerProc + ( 1 < p_size ? 1 : 0 );
  const unsigned nLocalElement = nPerProc ;

  std::vector<unsigned> local_count ;

  //------------------------------
  {
    bool aura = false;
    RingFixture ring_mesh( pm , nPerProc , false /* no element parts */ );
    BulkData & bulk = ring_mesh.m_bulk_data;
    ring_mesh.m_meta_data.commit();

    bulk.modification_begin();
    ring_mesh.generate_mesh( );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk, aura));

    bulk.modification_begin();
    ring_mesh.fixup_node_ownership( );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk, aura));

    const Selector select_used = ring_mesh.m_meta_data.locally_owned_part() |
                                 ring_mesh.m_meta_data.globally_shared_part() ;
    const Selector select_all = ring_mesh.m_meta_data.universal_part() ;

    count_entities( select_used , ring_mesh.m_bulk_data , local_count );
    STKUNIT_ASSERT_EQUAL( local_count[stk::topology::NODE_RANK] , nLocalNode );
    STKUNIT_ASSERT_EQUAL( local_count[stk::topology::ELEMENT_RANK] , nLocalElement );

    count_entities( select_all , ring_mesh.m_bulk_data , local_count );
    STKUNIT_ASSERT_EQUAL( local_count[stk::topology::NODE_RANK] , nLocalNode );
    STKUNIT_ASSERT_EQUAL( local_count[stk::topology::ELEMENT_RANK] , nLocalElement );

    if ( 1 < p_size ) {
      // Shift ring by two nodes and elements.

      stk::unit_test::test_shift_ring( ring_mesh, false /* no aura */ );

      count_entities( select_used , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement );

      count_entities( select_all , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement );
    }
  }

  //------------------------------
  // Test shift starting with ghosting but not regenerated ghosting.
  {
    RingFixture ring_mesh( pm , nPerProc , false /* no element parts */ );
    BulkData& bulk = ring_mesh.m_bulk_data;
    ring_mesh.m_meta_data.commit();

    bulk.modification_begin();
    ring_mesh.generate_mesh( );
    STKUNIT_ASSERT(bulk.modification_end());

    bulk.modification_begin();
    ring_mesh.fixup_node_ownership( );
    STKUNIT_ASSERT(bulk.modification_end());

    const Selector select_owned( ring_mesh.m_meta_data.locally_owned_part() );
    const Selector select_used = ring_mesh.m_meta_data.locally_owned_part() |
                                 ring_mesh.m_meta_data.globally_shared_part() ;
    const Selector select_all(   ring_mesh.m_meta_data.universal_part() );

    count_entities( select_used , ring_mesh.m_bulk_data , local_count );
    STKUNIT_ASSERT_EQUAL( local_count[stk::topology::NODE_RANK] , nLocalNode );
    STKUNIT_ASSERT_EQUAL( local_count[stk::topology::ELEMENT_RANK] , nLocalElement );

    count_entities( select_all , ring_mesh.m_bulk_data , local_count );
    const unsigned n_extra = 1 < p_size ? 2 : 0 ;
    STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode + n_extra );
    STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement + n_extra );

    if ( 1 < p_size ) {
      stk::unit_test::test_shift_ring( ring_mesh, false /* no aura */ );

      count_entities( select_owned , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nPerProc );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nPerProc );

      count_entities( select_used , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement );

      // All of my ghosts were disrupted and therefore deleted:
      count_entities( select_all , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT_EQUAL( nLocalElement , local_count[stk::topology::ELEMENT_RANK] );
      STKUNIT_ASSERT_EQUAL( nLocalNode , local_count[stk::topology::NODE_RANK] );
    }
  }
  //------------------------------
  // Test shift starting with ghosting and regenerating ghosting.
  {
    RingFixture ring_mesh( pm , nPerProc , false /* no element parts */ );
    BulkData& bulk = ring_mesh.m_bulk_data;
    ring_mesh.m_meta_data.commit();

    bulk.modification_begin();
    ring_mesh.generate_mesh( );
    STKUNIT_ASSERT(bulk.modification_end());

    bulk.modification_begin();
    ring_mesh.fixup_node_ownership( );
    STKUNIT_ASSERT(bulk.modification_end());

    const Selector select_owned( ring_mesh.m_meta_data.locally_owned_part() );
    const Selector select_used = ring_mesh.m_meta_data.locally_owned_part() |
                                 ring_mesh.m_meta_data.globally_shared_part() ;
    const Selector select_all(   ring_mesh.m_meta_data.universal_part() );

    count_entities( select_used , ring_mesh.m_bulk_data , local_count );
    STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode );
    STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement );

    count_entities( select_all , ring_mesh.m_bulk_data , local_count );
    const unsigned n_extra = 1 < p_size ? 2 : 0 ;
    STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode + n_extra );
    STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement + n_extra );

    if ( 1 < p_size ) {
      stk::unit_test::test_shift_ring( ring_mesh, true /* with aura */ );

      count_entities( select_owned , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nPerProc );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nPerProc );

      count_entities( select_used , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement );

      // All of my ghosts were regenerated:
      count_entities( select_all , ring_mesh.m_bulk_data , local_count );
      STKUNIT_ASSERT( local_count[stk::topology::NODE_RANK] == nLocalNode + n_extra );
      STKUNIT_ASSERT( local_count[stk::topology::ELEMENT_RANK] == nLocalElement + n_extra );
    }
  }
  //------------------------------
  // Test bad owner change catching:
  if ( 1 < p_size ) {
    RingFixture ring_mesh( pm , nPerProc , false /* no element parts */ );
    BulkData& bulk = ring_mesh.m_bulk_data;
    ring_mesh.m_meta_data.commit();

    bulk.modification_begin();
    ring_mesh.generate_mesh( );
    STKUNIT_ASSERT(bulk.modification_end());

    bulk.modification_begin();
    ring_mesh.fixup_node_ownership( );
    STKUNIT_ASSERT(bulk.modification_end());

    std::vector<EntityProc> change ;

    if ( 0 == p_rank ) {
      change.resize(4);
      // Error to change to bad owner:
      change[0].first = ring_mesh.m_bulk_data.get_entity( stk::topology::NODE_RANK , ring_mesh.m_node_ids[1] );
      change[0].second = p_size ;
      // Error to change a ghost:
      for ( stk::mesh::EntityCommListInfoVector::const_iterator
            ec =  ring_mesh.m_bulk_data.comm_list().begin() ;
            ec != ring_mesh.m_bulk_data.comm_list().end() ; ++ec ) {
        if ( bulk.in_receive_ghost( ec->key ) ) {
          change[1].first = ec->entity;
          break ;
        }
      }
      change[1].second = p_rank ;
      // Error to change to multiple owners:
      change[2].first = ring_mesh.m_bulk_data.get_entity( stk::topology::NODE_RANK , ring_mesh.m_node_ids[1] );
      change[2].second = ( p_rank + 1 ) % p_size ;
      change[3].first = change[2].first ;
      change[3].second = ( p_rank + 2 ) % p_size ;
    }

    STKUNIT_ASSERT( ring_mesh.m_bulk_data.modification_begin() );

    STKUNIT_ASSERT_THROW( ring_mesh.m_bulk_data.change_entity_owner( change ),
                          std::runtime_error );
  }
  //------------------------------
  // Test move one element with initial ghosting but not regenerated ghosting:
  // last processor give its shared node to P0
  if ( 1 < p_size ) {
    RingFixture ring_mesh( pm , nPerProc , false /* no element parts */ );
    BulkData& bulk = ring_mesh.m_bulk_data;
    ring_mesh.m_meta_data.commit();

    bulk.modification_begin();
    ring_mesh.generate_mesh( );
    STKUNIT_ASSERT(bulk.modification_end());

    bulk.modification_begin();
    ring_mesh.fixup_node_ownership( );
    STKUNIT_ASSERT(bulk.modification_end());

    const Selector select_owned( ring_mesh.m_meta_data.locally_owned_part() );
    const Selector select_used = ring_mesh.m_meta_data.locally_owned_part() |
                                 ring_mesh.m_meta_data.globally_shared_part() ;
    const Selector select_all(   ring_mesh.m_meta_data.universal_part() );

    std::vector<EntityProc> change ;

    if ( p_rank + 1 == p_size ) {
      EntityProc entry ;
      entry.first = ring_mesh.m_bulk_data.get_entity( stk::topology::NODE_RANK , ring_mesh.m_node_ids[0] );
      entry.second = 0 ;
      STKUNIT_ASSERT_EQUAL( p_rank , bulk.parallel_owner_rank(entry.first) );
      change.push_back( entry );
    }

    STKUNIT_ASSERT( ring_mesh.m_bulk_data.modification_begin() );
    ring_mesh.m_bulk_data.change_entity_owner( change );
    STKUNIT_ASSERT( stk::unit_test::modification_end_wrapper( ring_mesh.m_bulk_data , false ) );

    count_entities( select_owned , ring_mesh.m_bulk_data , local_count );
    const unsigned n_node = p_rank == 0          ? nPerProc + 1 : (
                            p_rank + 1 == p_size ? nPerProc - 1 :
                                                   nPerProc );

    STKUNIT_ASSERT_EQUAL( n_node , local_count[stk::topology::NODE_RANK] );
    STKUNIT_ASSERT_EQUAL( static_cast<unsigned>(nPerProc) , local_count[stk::topology::ELEMENT_RANK] );

    count_entities( select_used , ring_mesh.m_bulk_data , local_count );
    STKUNIT_ASSERT_EQUAL( nLocalNode , local_count[stk::topology::NODE_RANK] );
    STKUNIT_ASSERT_EQUAL( nLocalElement , local_count[stk::topology::ELEMENT_RANK] );

    // Moving the node disrupted ghosting on first and last process
    count_entities( select_all , ring_mesh.m_bulk_data , local_count );
    const unsigned n_extra = p_rank + 1 == p_size || p_rank == 0 ? 1 : 2 ;
    STKUNIT_ASSERT_EQUAL( nLocalNode + n_extra , local_count[stk::topology::NODE_RANK] );
    STKUNIT_ASSERT_EQUAL( nLocalElement + n_extra , local_count[stk::topology::ELEMENT_RANK] );
  }
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Testing for collection of boxes

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeOwner_box)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  const int root_box[3][2] = { { 0 , 4 } , { 0 , 5 } , { 0 , 6 } };

  const int p_size = stk::parallel_machine_size( pm );

  const int spatial_dimension = 3;
  MetaData meta( spatial_dimension );

  meta.commit();

  //------------------------------
  {
    bool aura = false;
    BoxFixture fixture( pm, 100 );
    fixture.fem_meta().commit();
    BulkData & bulk = fixture.bulk_data();
    int local_box[3][2] = { { 0 , 0 } , { 0 , 0 } , { 0 , 0 } };

    bulk.modification_begin();
    fixture.generate_boxes( root_box, local_box );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk, aura));

    if ( 1 < p_size ) {
      donate_one_element( bulk , aura );
    }
  }

  if ( 1 < p_size ) {
    bool aura = false;
    BoxFixture fixture( pm, 100 );
    fixture.fem_meta().commit();
    BulkData & bulk = fixture.bulk_data();
    int local_box[3][2] = { { 0 , 0 } , { 0 , 0 } , { 0 , 0 } };

    bulk.modification_begin();
    fixture.generate_boxes( root_box, local_box );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk, aura));

    donate_all_shared_nodes( bulk , aura );
  }
  //------------------------------
  if ( 1 < p_size ) {
    bool aura = false;
    BoxFixture fixture( pm, 100 );
    fixture.fem_meta().commit();
    BulkData & bulk = fixture.bulk_data();
    int local_box[3][2] = { { 0 , 0 } , { 0 , 0 } , { 0 , 0 } };

    bulk.modification_begin();
    fixture.generate_boxes( root_box, local_box );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk, aura));

    donate_one_element( bulk , false /* no aura */ );
  }
  //------------------------------
  // Introduce ghosts:
  if ( 1 < p_size ) {
    BoxFixture fixture( pm, 100 );
    BulkData & bulk = fixture.bulk_data();
    MetaData & box_meta = fixture.fem_meta();
    box_meta.commit();
    int local_box[3][2] = { { 0 , 0 } , { 0 , 0 } , { 0 , 0 } };

    bulk.modification_begin();
    fixture.generate_boxes( root_box, local_box );
    STKUNIT_ASSERT(bulk.modification_end());

    std::vector<unsigned> used_count ;
    std::vector<unsigned> all_count ;

    const Selector select_owned( box_meta.locally_owned_part() );
    const Selector select_used = box_meta.locally_owned_part() |
                                 box_meta.globally_shared_part() ;
    const Selector select_all(   box_meta.universal_part() );

    count_entities( select_all , bulk , all_count );
    count_entities( select_used , bulk , used_count );

    STKUNIT_ASSERT( used_count[0] < all_count[0] );
    STKUNIT_ASSERT( used_count[3] < all_count[3] );

    donate_all_shared_nodes( bulk , false /* don't regenerate aura */ );

    count_entities( select_all , bulk , all_count );
    count_entities( select_used , bulk , used_count );

    STKUNIT_ASSERT_EQUAL( used_count[0] , all_count[0] );
    STKUNIT_ASSERT_EQUAL( used_count[3] , all_count[3] );
  }
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testModifyPropagation)
{
  // Our new modification model makes it so the modified status
  // of an entity is propagated up to higher-ranked entities
  // that have relations to the modified entity. We test this
  // by grabbing a node off of a ring mesh, modifying it, and
  // checking that its element also gets marked as modified.

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  const unsigned nPerProc = 2;
  const int p_size = stk::parallel_machine_size( pm );

  // this test only needs to be run w/ one processor
  if (p_size > 1) return;

  // Make a ring_mesh and add an extra part
  RingFixture ring_mesh( pm , nPerProc, false /* don't use element parts */);
  stk::mesh::Part& special_part =
    ring_mesh.m_meta_data.declare_part("special_node_part", stk::mesh::BaseEntityRank );
  ring_mesh.m_meta_data.commit();
  BulkData& bulk = ring_mesh.m_bulk_data;

  bulk.modification_begin();
  ring_mesh.generate_mesh( );
  STKUNIT_ASSERT(bulk.modification_end());

  bulk.modification_begin();
  ring_mesh.fixup_node_ownership( );
  STKUNIT_ASSERT(bulk.modification_end());

  // grab the first element
  EntityVector elements;
  const stk::mesh::EntityRank element_rank = stk::topology::ELEMENT_RANK;
  stk::mesh::get_entities( ring_mesh.m_bulk_data, element_rank, elements );
  stk::mesh::Entity element = elements.front();

  // get one of the nodes related to this element
  STKUNIT_ASSERT(bulk.num_nodes(element) > 0);
  stk::mesh::Entity node = *bulk.begin_nodes(element);
  STKUNIT_ASSERT_EQUAL( bulk.entity_rank(node), static_cast<unsigned>(stk::mesh::BaseEntityRank) );

  // make a modification to the node by changing its parts
  ring_mesh.m_bulk_data.modification_begin();
  stk::mesh::PartVector parts;
  parts.push_back( &special_part );
  bulk.change_entity_parts( node, parts );

  // check that the node AND it's element are marked as modified
  STKUNIT_ASSERT_EQUAL ( bulk.state(node), stk::mesh::Modified );
  STKUNIT_ASSERT_EQUAL ( bulk.state(element), stk::mesh::Modified );

  STKUNIT_ASSERT ( bulk.modification_end() );
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeEntityOwnerFromSelfToSelf)
{
  // It should be legal to "change" entity ownership from yourself to yourself.
  //
  // 1---3---5
  // | 1 | 2 |
  // 2---4---6
  //
  // To test this, we use the mesh above, with elem 1 going on rank 0 and
  // elem 2 going on rank 1. Nodes 3,4 are shared. After the mesh is set up
  // we change the ownership of a few nodes to the same proc that already
  // owns them.

  stk::ParallelMachine pm = MPI_COMM_WORLD;

  // Set up meta and bulk data
  const unsigned spatial_dim = 2;
  MetaData meta_data(spatial_dim);
  meta_data.commit();
  BulkData mesh(meta_data, pm);
  int p_rank = mesh.parallel_rank();
  int p_size = mesh.parallel_size();

  // Bail if we only have one proc
  if (p_size == 1) {
    return;
  }

  // Begin modification cycle so we can create the entities and relations
  mesh.modification_begin();

  EntityVector nodes;
  const unsigned nodes_per_elem = 4, nodes_per_side = 2;

  if (p_rank < 2) {
    // We're just going to add everything to the universal part
    stk::mesh::PartVector empty_parts;

    // Create element
    const EntityRank elem_rank = stk::topology::ELEMENT_RANK;
    Entity elem = mesh.declare_entity(elem_rank,
                                        p_rank+1, //elem_id
                                        empty_parts);

    // Create nodes
    const unsigned starting_node_id = p_rank * nodes_per_side + 1;
    for (unsigned id = starting_node_id; id < starting_node_id + nodes_per_elem; ++id) {
      nodes.push_back(mesh.declare_entity(NODE_RANK, id, empty_parts));
    }

    // Add relations to nodes
    unsigned rel_id = 0;
    for (EntityVector::iterator itr = nodes.begin(); itr != nodes.end(); ++itr, ++rel_id) {
      mesh.declare_relation( elem, *itr, rel_id );
    }
  }

  mesh.modification_end();

  mesh.modification_begin();

  std::vector<EntityProc> change ;
  if (p_rank < 2) {
    // Change ownership of some nodes to the same proc that owns them

    // Add a non-shared node to change list
    if ( p_rank == 0 ) {
      EntityProc entry( nodes.front(), p_rank ) ;
      change.push_back( entry );
    }
    else {
      EntityProc entry( nodes.back(), p_rank ) ;
      change.push_back( entry );
    }

    // Add a shared node to change list
    Entity shared_node = nodes[p_rank == 0 ? nodes_per_side : 0];
    EntityId expected_id = 3;
    Part& shared_part = meta_data.globally_shared_part();
    STKUNIT_ASSERT( has_superset(mesh.bucket(shared_node), shared_part) );
    STKUNIT_ASSERT_EQUAL(mesh.identifier(shared_node), expected_id);
    if (mesh.parallel_owner_rank(shared_node) == p_rank) {
      EntityProc entry( shared_node, p_rank );
      change.push_back( entry );
    }
  }

  mesh.change_entity_owner(change);

  mesh.modification_end();
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeEntityOwnerOfShared)
{
  // This unit-test is designed to test the conditions that results that
  // resulted in the difficult-to-fix rebalance use-case bug. Specifically,
  // it will test the changing-of-ownership of a shared edge to a proc that
  // either ghosted it or did not know about it.
  //
  // 1---3---5---7
  // | 1 | 2 | 3 | ...
  // 2---4---6---8
  //
  // To test this, we use the mesh above, with each elem going on a separate
  // proc, one elem per proc. We will take the edge shared by the last
  // two (rightmost) elements and change the ownership to proc 0.

  stk::ParallelMachine pm = MPI_COMM_WORLD;

  // Set up meta and bulk data
  const unsigned spatial_dim = 2;
  MetaData meta_data(spatial_dim);
  meta_data.commit();
  BulkData mesh(meta_data, pm);
  int p_rank = mesh.parallel_rank();
  int p_size = mesh.parallel_size();
  const EntityRank edge_rank = stk::topology::EDGE_RANK;
  const EntityRank elem_rank = stk::topology::ELEMENT_RANK;

  // Bail if we have fewer than 3 procs
  if (p_size < 3) {
    return;
  }

  // Begin modification cycle so we can create the entities and relations
  mesh.modification_begin();

  const unsigned nodes_per_elem = 4, nodes_per_side = 2;
  EntityKey elem_key_chg_own(elem_rank, p_size - 1 /*id*/);
  EntityKey edge_key_chg_own(edge_rank, 1 /*id*/);

  // We're just going to add everything to the universal part
  stk::mesh::PartVector empty_parts;

  // Create element
  Entity elem = mesh.declare_entity(elem_rank,
                                      p_rank+1, //elem_id
                                      empty_parts);

  // If it is 2nd to last element, it is the one changing
  if (p_rank == p_size - 2) {
    STKUNIT_ASSERT(elem_key_chg_own == mesh.entity_key(elem));
  }

  // Create nodes
  EntityVector nodes;
  const unsigned starting_node_id = p_rank * nodes_per_side + 1;
  for (unsigned id = starting_node_id; id < starting_node_id + nodes_per_elem; ++id) {
    nodes.push_back(mesh.declare_entity(NODE_RANK, id, empty_parts));
  }

  // Add relations to nodes
  unsigned rel_id = 0;
  for (EntityVector::iterator itr = nodes.begin(); itr != nodes.end(); ++itr, ++rel_id) {
    mesh.declare_relation( elem, *itr, rel_id );
  }

  // Create edge on last two procs

  if (p_rank >= p_size - 2) {
    Entity edge = mesh.declare_entity(edge_rank,
                                       1, // id
                                       empty_parts);
    STKUNIT_ASSERT(mesh.entity_key(edge) == edge_key_chg_own);

    // Add relation from elem to edge
    mesh.declare_relation( elem, edge, 1 /*rel-id*/);

    // Add relations from edge to nodes
    unsigned start_idx = p_rank == p_size - 1 ? 0 : nodes_per_side;
    unsigned end_idx = start_idx + nodes_per_side;
    rel_id = 0;
    for (unsigned idx = start_idx ;
         start_idx < end_idx;
         ++start_idx, ++rel_id) {
      mesh.declare_relation( edge, nodes[idx], rel_id );
    }
  }

  mesh.modification_end();

  // Changing elem and edge should be ghosted or unknown on proc 0
  if (p_rank == 0) {
    // Get the two entities
    Entity changing_elem = mesh.get_entity(elem_key_chg_own);
    Entity changing_edge = mesh.get_entity(edge_key_chg_own);
    if (p_size == 3) {
      // Should be ghosted
      STKUNIT_ASSERT(mesh.is_valid(changing_elem));
      STKUNIT_ASSERT(mesh.is_valid(changing_edge));

      // Verify that the entities are ghosted
      Part& owned = meta_data.locally_owned_part();
      Part& shared = meta_data.globally_shared_part();
      STKUNIT_ASSERT(!(mesh.bucket(changing_elem).member(owned) ||
                       mesh.bucket(changing_elem).member(shared)));
      STKUNIT_ASSERT(!(mesh.bucket(changing_edge).member(owned) ||
                       mesh.bucket(changing_edge).member(shared)));
    }
    else {
      // Should be invalid
      STKUNIT_ASSERT(!mesh.is_valid(changing_elem));
      STKUNIT_ASSERT(!mesh.is_valid(changing_edge));
    }
  }

  mesh.modification_begin();

  std::vector<EntityProc> change ;
  if (p_rank >= p_size - 2) {
    // Change ownership of changing elem and all entities in it's closure that
    // we own to proc 0.

    Entity changing_elem = mesh.get_entity(elem_key_chg_own);
    if (p_rank == p_size - 2) {
      EntityProc eproc(changing_elem, 0 /*new owner*/);
      change.push_back(eproc);
    }

    const stk::mesh::EntityRank end_rank = static_cast<stk::mesh::EntityRank>(mesh.mesh_meta_data().entity_rank_count());
    for (stk::mesh::EntityRank irank = stk::topology::BEGIN_RANK; irank < end_rank; ++irank)
    {
      stk::mesh::Entity const *to_i = mesh.begin(changing_elem, irank);
      stk::mesh::Entity const *to_e = mesh.end(changing_elem, irank);
      for ( ; to_i != to_e; ++to_i)
      {
        if (mesh.parallel_owner_rank(*to_i) == p_rank)
        {
          EntityProc eproc(*to_i, 0 /*new owner*/);
          change.push_back(eproc);
        }
      }
    }
  }

  mesh.change_entity_owner(change);

  mesh.modification_end();

  // Changing elem and edge should now be owned by proc 0
  if (p_rank == 0) {
    // Get the two ghosted entities, check that they were found
    Entity changing_elem = mesh.get_entity(elem_key_chg_own);
    Entity changing_edge = mesh.get_entity(edge_key_chg_own);
    STKUNIT_ASSERT(mesh.is_valid(changing_elem));
    STKUNIT_ASSERT(mesh.is_valid(changing_edge));

    // Verify that the entities are ghosted
    Part& owned = meta_data.locally_owned_part();
    STKUNIT_ASSERT( mesh.bucket(changing_elem).member(owned) );
    STKUNIT_ASSERT( mesh.bucket(changing_edge).member(owned) );
  }
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testFamilyTreeGhosting)
{
  // A family tree is a higher-rank entity (rank = element_rank() + 1) that
  // has down-relations to elements used, for example, to hold parent/child
  // relations in an adapted mesh.
  //
  // 1---3---5---7
  // | 1 | 2 | 3 | ...
  // 2---4---6---8
  //
  // To test this, we use the mesh above, with each elem going on a separate
  // proc, one elem per proc.
  // After the mesh is set up we add rank-3 (family tree) entities and have them point down to
  // just the single rank-2 elements.  Then we check that they are properly
  // ghosted after modification_end.

  stk::ParallelMachine pm = MPI_COMM_WORLD;

  // Set up meta and bulk data
  const unsigned spatial_dim = 2;

  std::vector<std::string> entity_rank_names = stk::mesh::entity_rank_names();
  entity_rank_names.push_back("FAMILY_TREE");

  MetaData meta_data(spatial_dim, entity_rank_names);
  meta_data.commit();
  BulkData mesh(meta_data, pm);
  int p_rank = mesh.parallel_rank();
  int p_size = mesh.parallel_size();

  Part& owned  = meta_data.locally_owned_part();
  Part& shared = meta_data.globally_shared_part();

  //
  // Begin modification cycle so we can create the entities and relations
  //

  mesh.modification_begin();

  EntityVector nodes;
  const unsigned nodes_per_elem = 4, nodes_per_side = 2;
  const EntityRank family_tree_rank = static_cast<EntityRank>(stk::topology::ELEMENT_RANK + 1);
  const EntityId my_family_tree_id = p_rank+1;

  // We're just going to add everything to the universal part
  stk::mesh::PartVector empty_parts;

  // Create element
  const EntityRank elem_rank = stk::topology::ELEMENT_RANK;
  Entity elem = mesh.declare_entity(elem_rank,
                                    p_rank+1, //elem_id
                                    empty_parts);

  // Create nodes
  const unsigned starting_node_id = p_rank * nodes_per_side + 1;
  for (unsigned id = starting_node_id; id < starting_node_id + nodes_per_elem; ++id) {
    nodes.push_back(mesh.declare_entity(NODE_RANK, id, empty_parts));
  }

  // Add relations to nodes
  unsigned rel_id = 0;
  for (EntityVector::iterator itr = nodes.begin(); itr != nodes.end(); ++itr, ++rel_id) {
    mesh.declare_relation( elem, *itr, rel_id );
  }

  // Create family tree
  Entity family_tree = mesh.declare_entity(family_tree_rank,
                                           my_family_tree_id,
                                           empty_parts);
  // Add relation to element
  unsigned downward_ordinal = 0; // we only have 1 down relation, it has ordinal 0
  mesh.declare_relation( family_tree, elem, downward_ordinal);

  mesh.modification_end();

  //
  // Test correctness of ghosting: Check that adjacent family-trees are ghosted on this proc
  //

  // Compute and store ids of adjacent family-trees
  std::vector<EntityId> family_tree_ghost_ids;
  if (p_rank > 0) {
    family_tree_ghost_ids.push_back(my_family_tree_id - 1);
  }
  if (p_rank < p_size - 1) {
    family_tree_ghost_ids.push_back(my_family_tree_id + 1);
  }

  // Check that my_family_tree exists and I own it
  Entity my_family_tree = mesh.get_entity(family_tree_rank, my_family_tree_id);
  STKUNIT_ASSERT(mesh.is_valid(my_family_tree));
  STKUNIT_ASSERT( (p_rank) == mesh.parallel_owner_rank(my_family_tree));

  // Check that adjacent family-trees exist and are ghosted
  for (std::vector<EntityId>::const_iterator
       itr = family_tree_ghost_ids.begin(); itr != family_tree_ghost_ids.end(); ++itr) {
    int expected_ghosted_family_tree_id = *itr;

    Entity expected_ghosted_family_tree = mesh.get_entity(family_tree_rank, expected_ghosted_family_tree_id);
    STKUNIT_ASSERT(mesh.is_valid(expected_ghosted_family_tree));
    STKUNIT_ASSERT(expected_ghosted_family_tree_id - 1 == mesh.parallel_owner_rank(expected_ghosted_family_tree));

    stk::mesh::Bucket& bucket = mesh.bucket(expected_ghosted_family_tree);
    STKUNIT_ASSERT(!bucket.member(owned) && !bucket.member(shared));
  }
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, test_other_ghosting)
{
  //
  // 1---3---5---7
  // | 1 | 2 | 3 | ...
  // 2---4---6---8
  //
  // To test this, we use the mesh above, with each elem going on a separate
  // proc, one elem per proc.

  stk::ParallelMachine pm = MPI_COMM_WORLD;

  // Set up meta and bulk data
  const unsigned spatial_dim = 2;

  std::vector<std::string> entity_rank_names = stk::mesh::entity_rank_names();
  //entity_rank_names.push_back("FAMILY_TREE");

  MetaData meta_data(spatial_dim, entity_rank_names);
  meta_data.commit();
  BulkData mesh(meta_data, pm);
  int p_rank = mesh.parallel_rank();
  int p_size = mesh.parallel_size();

  if (p_size != 3) return;

  //Part& owned  = meta_data.locally_owned_part();
  //Part& shared = meta_data.globally_shared_part();

  //
  // Begin modification cycle so we can create the entities and relations
  //

  mesh.modification_begin();

  EntityVector nodes;
  const unsigned nodes_per_elem = 4, nodes_per_side = 2;

  // We're just going to add everything to the universal part
  stk::mesh::PartVector empty_parts;

  // Create element
  const EntityRank elem_rank = stk::topology::ELEMENT_RANK;
  Entity elem = mesh.declare_entity(elem_rank,
                                      p_rank+1, //elem_id
                                      empty_parts);

  // Create nodes
  const unsigned starting_node_id = p_rank * nodes_per_side + 1;
  for (unsigned id = starting_node_id; id < starting_node_id + nodes_per_elem; ++id) {
    nodes.push_back(mesh.declare_entity(NODE_RANK, id, empty_parts));
  }

  // Add relations to nodes
  unsigned rel_id = 0;
  for (EntityVector::iterator itr = nodes.begin(); itr != nodes.end(); ++itr, ++rel_id) {
    mesh.declare_relation( elem, *itr, rel_id );
  }

  if (1 && p_rank == 2)
    {
       Entity node = mesh.declare_entity(NODE_RANK,
                                          4,
                                          empty_parts);
       //Entity node = mesh.get_entity(NODE_RANK,
       //4);

      mesh.declare_relation(elem, node, 4);
    }

  mesh.modification_end();

  mesh.modification_begin();
  if (p_rank == 1)
    {
      Entity this_elem = mesh.get_entity(stk::topology::ELEMENT_RANK, 2);
      if (!mesh.destroy_entity(this_elem)) exit(2);
    }
  if (p_rank == 2)
    {
      Entity this_elem = mesh.get_entity(stk::topology::ELEMENT_RANK, 3);
      if (!mesh.destroy_entity(this_elem)) exit(2);
    }
  mesh.modification_end();
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testChangeEntityPartsOfShared)
{
  //
  // This unit-test is designed to test what happens when a shared entity
  // is moved on one processor during the same modification cycle in which
  // it was declared.
  //
  // 1---3---5
  // | 1 | 2 |
  // 2---4---6
  //
  // To test this, we use the mesh above, with each elem going on a separate
  // proc, one elem per proc. Node 3 is the node we'll be testing.
  //

  stk::ParallelMachine pm = MPI_COMM_WORLD;

  // Set up meta and bulk data
  const unsigned spatial_dim = 2;
  MetaData meta_data(spatial_dim);
  const EntityRank node_rank = stk::topology::NODE_RANK;
  const EntityRank elem_rank = stk::topology::ELEMENT_RANK;

  stk::mesh::Part& extra_node_part = meta_data.declare_part("extra_node_part", node_rank);
  meta_data.commit();

  BulkData mesh(meta_data, pm);
  int p_rank = mesh.parallel_rank();
  int p_size = mesh.parallel_size();

  // Bail unless in parallel
  if (p_size == 1) {
    return;
  }

  // Begin modification cycle so we can create the entities and relations
  if (p_rank < 2) {
    mesh.modification_begin();

    const unsigned nodes_per_elem = 4, nodes_per_side = 2;
    EntityKey node_key_to_move(node_rank, 3 /*id*/);

    // We're just going to add everything to the universal part
    stk::mesh::PartVector empty_parts;

    // Create element
    Entity elem = mesh.declare_entity(elem_rank,
                                        p_rank+1, //elem_id
                                        empty_parts);

    // Create nodes
    EntityVector nodes;
    const unsigned starting_node_id = p_rank * nodes_per_side + 1;
    for (unsigned id = starting_node_id; id < starting_node_id + nodes_per_elem; ++id) {
      nodes.push_back(mesh.declare_entity(NODE_RANK, id, empty_parts));
    }

    // Add relations to nodes
    unsigned rel_id = 0;
    for (EntityVector::iterator itr = nodes.begin(); itr != nodes.end(); ++itr, ++rel_id) {
      mesh.declare_relation( elem, *itr, rel_id );
    }

    // On the processor that does *not* end up as the owner of the node, change its parts
    Entity changing_node = mesh.get_entity(node_key_to_move);
    if (p_rank == 1) {
      PartVector add_parts(1, &extra_node_part);
      mesh.change_entity_parts(changing_node, add_parts);
    }

    mesh.modification_end();

    // Expect that this is a shared node
    STKUNIT_EXPECT_FALSE(mesh.entity_comm_sharing(mesh.entity_key(changing_node)).empty());

    // Expect that part change had no impact since it was on the proc that did not end
    // up as the owner
    STKUNIT_EXPECT_FALSE(mesh.bucket(changing_node).member(extra_node_part));

    mesh.modification_begin();

    // On the processor that owns the node, change its parts
    if (p_rank == 0) {
      PartVector add_parts(1, &extra_node_part);
      mesh.change_entity_parts(changing_node, add_parts);
    }

    mesh.modification_end();

    // Expect that the part change *did* have an impact
    STKUNIT_EXPECT_TRUE(mesh.bucket(changing_node).member(extra_node_part));
  }
  else {
    // On extra procs, do bare minimum
    mesh.modification_begin();
    mesh.modification_end();
    mesh.modification_begin();
    mesh.modification_end();
  }
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testParallelSideCreation)
{
  //
  // This unit-test is designed to test what happens when a shared sides are created on
  // both processors that share the side with different global ids.  Then synchonization
  // is done as a second step.
  //
  // 1---3---5
  // | 1 | 2 |
  // 2---4---6
  //
  // To test this, we use the mesh above, with each elem going on a separate
  // proc, one elem per proc. Node 3 is the node we'll be testing.
  //

  stk::ParallelMachine pm = MPI_COMM_WORLD;

  // Set up meta and bulk data
  const unsigned spatial_dim = 2;
  MetaData meta_data(spatial_dim);
  const EntityRank node_rank = stk::topology::NODE_RANK;
  const EntityRank elem_rank = stk::topology::ELEMENT_RANK;
  const EntityRank side_rank = stk::topology::EDGE_RANK;

  meta_data.commit();

  BulkData mesh(meta_data, pm);
  int p_rank = mesh.parallel_rank();
  int p_size = mesh.parallel_size();

  // Bail unless in parallel
  if (p_size == 1) {
    return;
  }

  // Begin modification cycle so we can create the entities and relations
  if (p_rank < 2) {
    mesh.modification_begin();

    const unsigned nodes_per_elem = 4, nodes_per_side = 2;
    EntityKey node_key_to_move(node_rank, 3 /*id*/);

    // We're just going to add everything to the universal part
    stk::mesh::PartVector empty_parts;

    // Create nodes
    EntityVector nodes;
    const unsigned starting_node_id = p_rank * nodes_per_side + 1;
    for (unsigned id = starting_node_id; id < starting_node_id + nodes_per_elem; ++id) {
      nodes.push_back(mesh.declare_entity(NODE_RANK, id, empty_parts));
    }

    // Create element
    const EntityId elem_id = p_rank+1;
    Entity elem = mesh.declare_entity(elem_rank, elem_id, empty_parts);

    // Create local version of side (with different, artificial id on each processor)
    const EntityId tmp_side_id = p_rank+51;
    Entity side = mesh.declare_entity(side_rank, tmp_side_id, empty_parts);

    // Add element relations to nodes
    unsigned elem_rel_id = 0;
    for (EntityVector::iterator itr = nodes.begin(); itr != nodes.end(); ++itr, ++elem_rel_id) {
      mesh.declare_relation( elem, *itr, elem_rel_id );
    }

    // Add side relations to nodes and element
    EntityVector side_nodes;
    side_nodes.push_back( mesh.get_entity( NODE_RANK, 3 ) );
    side_nodes.push_back( mesh.get_entity( NODE_RANK, 4 ) );
    unsigned side_rel_id = 0;
    for (EntityVector::iterator itr = side_nodes.begin(); itr != side_nodes.end(); ++itr, ++side_rel_id) {
      mesh.declare_relation( side, *itr, side_rel_id );
    }
    mesh.declare_relation( elem, side, 0 );

    mesh.modification_end();

    // Expect that the side is not shared, but the nodes of side are shared
    STKUNIT_EXPECT_TRUE(mesh.entity_comm_sharing(mesh.entity_key(side)).empty());
    STKUNIT_EXPECT_FALSE(mesh.entity_comm_sharing(mesh.entity_key(side_nodes[0])).empty());
    STKUNIT_EXPECT_FALSE(mesh.entity_comm_sharing(mesh.entity_key(side_nodes[1])).empty());

    // Now "detect" that there is a duplicate aura side using the side nodes
    EntityVector sides;
    get_entities_through_relations(mesh, side_nodes, side_rank, sides);
    STKUNIT_EXPECT_EQUAL(2u, sides.size());

    mesh.modification_begin();

    // Delete the local side and create new, shared side
    mesh.destroy_relation(elem, side, 0);
    bool successfully_destroyed = mesh.destroy_entity(side);
    STKUNIT_EXPECT_TRUE(successfully_destroyed);

    const EntityId side_id = 1;
    side = mesh.declare_entity(side_rank, side_id, empty_parts);

    // Add side relations to nodes and element
    side_rel_id = 0;
    for (EntityVector::iterator itr = side_nodes.begin(); itr != side_nodes.end(); ++itr, ++side_rel_id) {
      mesh.declare_relation( side, *itr, side_rel_id );
    }
    mesh.declare_relation( elem, side, 0 );

    mesh.modification_end();

    // Expect that the side is shared, and nodes of side are shared
    STKUNIT_EXPECT_FALSE(mesh.entity_comm_sharing(mesh.entity_key(side)).empty());
    STKUNIT_EXPECT_FALSE(mesh.entity_comm_sharing(mesh.entity_key(side_nodes[0])).empty());
    STKUNIT_EXPECT_FALSE(mesh.entity_comm_sharing(mesh.entity_key(side_nodes[1])).empty());

    // Check that there is only a single side using the side nodes
    get_entities_through_relations(mesh, side_nodes, side_rank, sides);
    STKUNIT_EXPECT_EQUAL(1u, sides.size());
  }
  else {
    // On extra procs, do bare minimum
    mesh.modification_begin();
    mesh.modification_end();
    mesh.modification_begin();
    mesh.modification_end();
  }
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, test_final_modification_end)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;

  const unsigned spatial_dim = 2;
  MetaData meta_data(spatial_dim);
  meta_data.commit();

  BulkData mesh(meta_data, pm);

  mesh.modification_begin();
  mesh.final_modification_end();

  STKUNIT_ASSERT_THROW(mesh.modification_begin(), std::logic_error );
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Testing of field_data_footprint(.)
STKUNIT_UNIT_TEST( UnitTestingOfBulkData, test_total_field_data_footprint )
{
  // Test 3x1x1 HexFixture structure
  const unsigned NX = 3;
  const unsigned NY = 1;
  const unsigned NZ = 1;
  stk::mesh::fixtures::HexFixture hf(MPI_COMM_WORLD,NX,NY,NZ);
  hf.m_meta.commit();
  hf.generate_mesh();

  const stk::mesh::BulkData &mesh = hf.m_bulk_data;

  // Call function we're testing
  size_t field_data_footprint = mesh.total_field_data_footprint(stk::topology::NODE_RANK);

  // Alternative computation explicitly gathers buckets.
  size_t node_fields_footprint = 0;
  const std::vector<stk::mesh::Bucket *> &node_buckets = mesh.buckets(stk::topology::NODE_RANK);
  for (size_t i = 0; i < node_buckets.size(); ++i)
  {
    node_fields_footprint +=
        node_buckets[i]->capacity() * field_bytes_per_entity(hf.m_coord_field, *node_buckets[i]);
  }

  STKUNIT_EXPECT_EQUAL(node_fields_footprint, field_data_footprint);
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Testing of communicate_field_data

typedef stk::mesh::Field<int>  PressureFieldType ;

static void test_sync_1(stk::mesh::BulkData& eMesh, PressureFieldType& pressure_field, bool sync_shared, bool sync_aura)
{
  unsigned p_rank = eMesh.parallel_rank();
  unsigned p_size = eMesh.parallel_size();
  static_cast<void>(p_size);

  const std::vector<stk::mesh::Bucket*> & buckets = eMesh.buckets( stk::topology::NODE_RANK );

  enum Type{ Owned, Shared, Ghost };

  for ( std::vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
  {
    {
      stk::mesh::Bucket & bucket = **k ;

      const unsigned num_elements_in_bucket = bucket.size();

      for (unsigned iEntity = 0; iEntity < num_elements_in_bucket; iEntity++)
      {
        stk::mesh::Entity entity = bucket[iEntity];
        int * const p = stk::mesh::field_data( pressure_field , entity );
        stk::mesh::EntityId id=eMesh.identifier(entity);

        int type=Owned;
        if (bucket.owned())
        {
          p[0] = (p_rank+1)*100+id;
        }
        else if (bucket.shared())
        {
          p[0] = -((eMesh.parallel_owner_rank(entity)+1)*100+id);
          type=Shared;
        }
        else
        {
          p[0] = ((p_rank+1)*1000 + id);
          type=Ghost;
        }

      }
    }
  }

  {
    std::vector< const stk::mesh::FieldBase *> fields;
    fields.push_back(&pressure_field);

    // only the aura = !locally_owned_part && !globally_shared_part (outer layer)
    if (sync_aura) stk::mesh::communicate_field_data(eMesh.shared_aura(), fields);

    // the shared part (just the shared boundary)
    if (sync_shared) stk::mesh::copy_owned_to_shared(eMesh, fields);
  }

  for ( std::vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
  {
    {
      stk::mesh::Bucket & bucket = **k ;

      const unsigned num_elements_in_bucket = bucket.size();

      for (unsigned iEntity = 0; iEntity < num_elements_in_bucket; iEntity++)
      {
        stk::mesh::Entity entity = bucket[iEntity];
        stk::mesh::EntityId id = eMesh.identifier(entity);
        int * const p = stk::mesh::field_data( pressure_field , entity );
        int type=Owned;
        double p_e = (p_rank+1)*100+id;
        if (bucket.owned())
        {
          STKUNIT_ASSERT_EQUAL(p[0], p_e);
        }
        else if (bucket.shared())
        {
          p_e = ((eMesh.parallel_owner_rank(entity)+1)*100+id);
          type = Shared;
          if (sync_shared)
          {
            STKUNIT_ASSERT_EQUAL(p[0], p_e);
          }
        }
        else
        {
          p_e = ((eMesh.parallel_owner_rank(entity)+1)*100+id);
          type = Ghost;
          if (sync_aura)
            STKUNIT_ASSERT_EQUAL(p[0], p_e);
        }
      }
    }
  }
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testFieldComm)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  // run this with exercise_field_sync_bug = true, and 3 <= nprocs <= 4 to show the possible bug
  bool exercise_field_sync_bug = true;

  const unsigned p_size = stk::parallel_machine_size( pm );

  const int spatial_dimension = 3;
  MetaData meta( spatial_dimension );

  meta.commit();

  //------------------------------
  // 3d seems to be fine...
  if (p_size <= 4)
  {
    const int root_box[3][2] = { { 0 , 2 } , { 0 , 2 } , { 0 , 1 } };  // emulate 2d box

    BoxFixture fixture( pm, 100 );
    PressureFieldType& p_field = fixture.fem_meta().declare_field<PressureFieldType>(stk::topology::NODE_RANK, "p");
    stk::mesh::put_field( p_field , fixture.fem_meta().universal_part());
    fixture.fem_meta().commit();
    BulkData & bulk = fixture.bulk_data();
    int local_box[3][2] = { { 0 , 0 } , { 0 , 0 } , { 0 , 0 } };

    bulk.modification_begin();
    fixture.generate_boxes( root_box, local_box );
    bulk.modification_end();

    {
      bool shared_aura = false;
      bool shared = false;
      test_sync_1(bulk, p_field, shared, shared_aura);
      test_sync_1(bulk, p_field, false, true);
      if (exercise_field_sync_bug || p_size <=2)
        {
          test_sync_1(bulk, p_field, true, false);
          test_sync_1(bulk, p_field, true, true);
        }
    }
  }

  //------------------------------
  // 2d, not so much
  if (p_size <= 4)
  {
    stk::mesh::fixtures::QuadFixture fixture(pm, 2 /*nx*/, 2 /*ny*/);
    PressureFieldType& p_field = fixture.m_meta.declare_field<PressureFieldType>(stk::topology::NODE_RANK, "p");
    stk::mesh::put_field( p_field , fixture.m_meta.universal_part());
    fixture.m_meta.commit();
    fixture.generate_mesh();
    stk::mesh::BulkData & bulk = fixture.m_bulk_data;

    {
      bool shared_aura = false;
      bool shared = false;
      test_sync_1(bulk, p_field, shared, shared_aura);
      test_sync_1(bulk, p_field, false, true);
      if (exercise_field_sync_bug || p_size <=2)
        {
          test_sync_1(bulk, p_field, true, false);
          test_sync_1(bulk, p_field, true, true);
        }
    }
  }
}

// testing comm lists and custom ghosting

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testCommList)
{
  /**
   * This is a boiled-down version of a stk_adapt situation that is failing
   *   where a custom ghosted node is later shared because it becomes part
   *   of an element (this occurs during hanging-node refinement).  It is
   *   a definite edge case, but exposed an assumption in the comm_mesh_verify_parallel_consistency()
   *   function, that a comm list can't have both a shared and custom ghosted node.
   * This test currently just verifies the issue exists.  When comm_mesh_verify_parallel_consistency()
   *   is fixed, this test should be modified to enforce no failure under debug mode.
   *
   *  Mesh
   *    7---8---9  P0 owns nodes 1,2,4,5; P, elem 1
   *    | 3 | 4 |  P1 : 3,6, elem 2
   *    4---5---6  P2 : 7,8, elem 3
   *    | 1 | 2 |  P3 : 9,   elem 4
   *    1---2---3
   *
   *  node 5 ghosted to proc 3, node 9 ghosted to proc 0
   * 
   *  Comm list looks like this after the ghosting operations (obtained from print_comm_list()):
   *    legend: (ghost_id, proc)
   *    P3: NODE[9] owner(3)     (1,0) (1,1) (1,2) (2,0)
   *    P0: NODE[9] owner(3)     (1,3) (2,3)
   *
   *  elem 1 modified to replace relation elem[1] -> node[1] with node[9] (previously ghosted)
   *
   *  This last step induces a comm list like this 
   *
   *    P3: NODE[9] owner(3) mod (0,0) (1,1) (1,2) (2,0)
   *    P0: NODE[9] owner(3) mod (0,3) (2,3)
   *
   *  Note the repetition of proc 3 in both the shared (ghost_id=0) and ghosted (id=2) list
   *  for P0 (and repetition of proc 0 in P3's list).  This causes the
   *  comm_mesh_verify_parallel_consistency() test to fail, although
   *  this should be a valid mesh, e.g., for periodic b.c.'s we might want this situation.
   *
   */

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  const unsigned p_size = stk::parallel_machine_size( pm );
  const unsigned p_rank = stk::parallel_machine_rank( pm );

  //------------------------------
  if (p_size != 4)
    return;

  //------------------------------
  // test begin/end pair
  {
    stk::mesh::fixtures::QuadFixture fixture(pm, 2 /*nx*/, 2 /*ny*/);
    fixture.m_meta.commit();
    fixture.generate_mesh();
    stk::mesh::BulkData & bulk = fixture.m_bulk_data;
    bulk.modification_begin();
    bulk.modification_end();
  }

  //------------------------------
  // test begin/end pair with mesh mods
  {
    stk::mesh::fixtures::QuadFixture fixture(pm, 2 /*nx*/, 2 /*ny*/);
    fixture.m_meta.commit();
    fixture.generate_mesh();
    stk::mesh::BulkData & bulk = fixture.m_bulk_data;
    bulk.modification_begin();
    bulk.modification_end();

    bulk.modification_begin();

    // add some custom ghosting
    stk::mesh::Ghosting & ghosting = bulk.create_ghosting( std::string("new_nodes") );
    std::vector<stk::mesh::EntityKey> receive;
    ghosting.receive_list( receive );

    std::vector<stk::mesh::EntityProc> nodes_to_ghost;

    if (p_rank == 0)
      {
        stk::mesh::Entity node_5 = bulk.get_entity(stk::topology::NODE_RANK, 5);
        STKUNIT_EXPECT_TRUE(bulk.is_valid(node_5));
        nodes_to_ghost.push_back( stk::mesh::EntityProc(node_5, 3) );
      }

    if (p_rank == 3)
      {
        stk::mesh::Entity node = bulk.get_entity(stk::topology::NODE_RANK, 9);
        STKUNIT_EXPECT_TRUE(bulk.is_valid(node));
        nodes_to_ghost.push_back( stk::mesh::EntityProc(node, 0) );
      }

    bulk.change_ghosting( ghosting, nodes_to_ghost, receive);

    bulk.modification_end();

    bulk.modification_begin();

    if (p_rank == 0)
      {
        stk::mesh::Entity node = bulk.get_entity(stk::topology::NODE_RANK, 9);
        STKUNIT_EXPECT_TRUE(bulk.is_valid(node));
        stk::mesh::Entity node1 = bulk.get_entity(stk::topology::NODE_RANK, 1);
        STKUNIT_EXPECT_TRUE(bulk.is_valid(node1));
        stk::mesh::Entity elem = bulk.get_entity(stk::topology::ELEMENT_RANK, 1);
        STKUNIT_EXPECT_TRUE(bulk.is_valid(elem));
        bulk.destroy_relation( elem, node1, 0);
        bulk.declare_relation( elem , node , 0 );
      }

    bool failed=false;
    try {
      bulk.modification_end();
    }
    catch ( const std::exception & X ) {
      failed = true;
    }
    STKUNIT_EXPECT_FALSE(failed);

  }

}
