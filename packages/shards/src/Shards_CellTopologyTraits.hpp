/*------------------------------------------------------------------------*/
/*                  shards : Shared Discretization Tools                  */
/*                Copyright (2008) Sandia Corporation                     */
/*                                                                        */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*                                                                        */
/*  This library is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU Lesser General Public License as        */
/*  published by the Free Software Foundation; either version 2.1 of the  */
/*  License, or (at your option) any later version.                       */
/*                                                                        */
/*  This library is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     */
/*  Lesser General Public License for more details.                       */
/*                                                                        */
/*  You should have received a copy of the GNU Lesser General Public      */
/*  License along with this library; if not, write to the Free Software   */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   */
/*  USA                                                                   */
/*------------------------------------------------------------------------*/

#ifndef Shards_CellTopologyTraits_hpp
#define Shards_CellTopologyTraits_hpp

#include <Shards_TypeList.hpp>
#include <Shards_IndexList.hpp>
#include <Shards_CellTopologyData.h>

namespace shards {

/** \addtogroup shards_package_cell_topology
 *  \{
 */

/**
 *  \brief  Return a CellTopology singleton for the given cell topology traits.
 */
template< class Traits >
const CellTopologyData * cell_topology();

template< unsigned Dimension ,
          unsigned Number_Vertex ,
          unsigned Number_Node ,
          class    EdgeList = TypeListEnd ,
          class    EdgeMaps = TypeListEnd ,
          class    FaceList = TypeListEnd ,
          class    FaceMaps = TypeListEnd >
struct CellTopologyTraits ;

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Implementation details for a much of this file ...

#ifndef DOXYGEN_COMPILE

template< class CellTop , class CellMap , unsigned Index , bool Good >
struct SubcellNodeIndex ;

template< class CellTop , class CellMap , unsigned Index >
struct SubcellNodeIndex< CellTop , CellMap , Index , false >
{ enum { value = -1 }; };

template< class CellTop , class CellMap , unsigned Index >
struct SubcellNodeIndex< CellTop , CellMap , Index , true >
{
  enum { value = Index < CellTop::template subcell<0>::count
               ? IndexListAt< CellMap , Index >::value : -1 };
};

//----------------------------------------------------------------------

template< unsigned SubcellDim , unsigned SubcellOrd , unsigned NodeIndex ,
          unsigned Dimension ,
          unsigned Number_Vertex , unsigned Number_Node ,
          class EdgeList , class EdgeMaps ,
          class FaceList , class FaceMaps >
struct SubcellTopologyTraits ;

//----------------------------------------------------------------------
// Self-subcell reference

template< unsigned NodeIndex ,
          unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits<0,0,NodeIndex, 0,NV,NN,EList,EMaps,FList,FMaps>
{
  typedef CellTopologyTraits<0,NV,NN,EList,EMaps,FList,FMaps> topology ;
  enum { count = 1 };
  enum { node = NodeIndex < NN ? (int) NodeIndex : -1 };
};

template< unsigned NodeIndex ,
          unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits<1,0,NodeIndex, 1,NV,NN,EList,EMaps,FList,FMaps>
{
  typedef CellTopologyTraits<1,NV,NN,EList,EMaps,FList,FMaps> topology ;
  enum { count = 1 };
  enum { node = NodeIndex < NN ? (int) NodeIndex : -1 };
};

template< unsigned NodeIndex ,
          unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits<2,0,NodeIndex, 2,NV,NN,EList,EMaps,FList,FMaps>
{
  typedef CellTopologyTraits<2,NV,NN,EList,EMaps,FList,FMaps> topology ;
  enum { count = 1 };
  enum { node = NodeIndex < NN ? (int) NodeIndex : -1 };
};

template< unsigned NodeIndex ,
          unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits<3,0,NodeIndex, 3,NV,NN,EList,EMaps,FList,FMaps>
{
  typedef CellTopologyTraits<3,NV,NN,EList,EMaps,FList,FMaps> topology ;
  enum { count = 1 };
  enum { node = NodeIndex < NN ? (int) NodeIndex : -1 };
};

//----------------------------------------------------------------------
// Node-subcell reference:

template< unsigned SubcellOrd ,
          unsigned D , unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits<0,SubcellOrd,0, D,NV,NN,EList,EMaps,FList,FMaps>
{
  typedef CellTopologyTraits<0,0,0> topology ;
  enum { count = NN };
  enum { node = SubcellOrd < NN ? (int) SubcellOrd : -1 };
};

// Edge-subcell reference:

template< unsigned SubcellOrd , unsigned NodeIndex ,
          unsigned D , unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits<1,SubcellOrd,NodeIndex,
                             D,NV,NN,EList,EMaps,FList,FMaps>
{
private:
  typedef typename TypeListAt<EMaps,SubcellOrd>::type node_map ;
public:

  typedef typename TypeListAt<EList,SubcellOrd>::type topology ;

  enum { count = TypeListLength<EList>::value };

  enum { node = SubcellNodeIndex< topology , node_map , NodeIndex ,
                                  SubcellOrd < count >::value };
};

// Face-subcell reference:

template< unsigned SubcellOrd , unsigned NodeIndex ,
          unsigned D , unsigned NV , unsigned NN ,
          class EList , class EMaps ,
          class FList , class FMaps >
struct SubcellTopologyTraits< 2, SubcellOrd, NodeIndex,
                              D,NV,NN,EList,EMaps,FList,FMaps>
{
private:
  typedef typename TypeListAt<FMaps,SubcellOrd>::type node_map ;
public:

  typedef typename TypeListAt<FList,SubcellOrd>::type topology ;

  enum { count = TypeListLength<FList>::value };

  enum { node = SubcellNodeIndex< topology , node_map , NodeIndex ,
                                  SubcellOrd < count >::value };
};

//----------------------------------------------------------------------
// Only partially specialized subcell references are valid.

template< unsigned SubcellDim , unsigned SubcellOrd , unsigned NodeIndex ,
          unsigned Dimension ,
          unsigned Number_Vertex , unsigned Number_Node ,
          class EdgeList , class EdgeMaps ,
          class FaceList , class FaceMaps >
struct SubcellTopologyTraits
{
  typedef void topology ;
  enum { count = 0 };
  enum { node = -1 };
};

//----------------------------------------------------------------------

template< class ListType > struct TypeListHomogeneous ;

template<>
struct TypeListHomogeneous<TypeListEnd> {
  enum { value = true };
};

template< class T >
struct TypeListHomogeneous< TypeList<T,TypeListEnd> > {
  enum { value = true };
};

template< class T , class Tail >
struct TypeListHomogeneous< TypeList< T, TypeList< T , Tail > > > {
  enum { value = TypeListHomogeneous< TypeList<T,Tail> >::value };
};

template< class ListType >
struct TypeListHomogeneous 
{
  enum { value = false };
};

//----------------------------------------------------------------------

template< int I , int J > struct AssertEqual ;

template< int I > struct AssertEqual<I,I> { enum { OK = true }; };

#endif /* DOXYGEN_COMPILE */

//----------------------------------------------------------------------
/**  \brief  Compile-time traits for a cell topology. */
template< unsigned Dimension , unsigned Number_Vertex , unsigned Number_Node ,
          class EdgeList , class EdgeMaps ,
          class FaceList , class FaceMaps >
struct CellTopologyTraits
{
  /** \brief  The <em> self </em> type for the traits */
  typedef CellTopologyTraits< Dimension, Number_Vertex, Number_Node,
                              EdgeList, EdgeMaps, FaceList, FaceMaps > Traits ;

  enum {
    /** \brief  Topological dimension */
    dimension    = Dimension ,

    /** \brief  Number of vertices */
    vertex_count = Number_Vertex ,

    /** \brief  Number of nodes (a.k.a. Cell^0 subcells). */
    node_count   = Number_Node ,

    /** \brief  Number of edges (a.k.a. Cell^1 subcells). */
    edge_count   = TypeListLength<EdgeList>::value ,

#ifndef DOXYGEN_COMPILE
    face_count   = TypeListLength<FaceList>::value ,
#endif

    /** \brief  Number of sides (a.k.a. Cell^(D-1) subcells). */
    side_count   = Dimension == 3 ? face_count : (
                   Dimension == 2 ? edge_count : 0 ),

    /** \brief  Unique key for this topology */
    key          = ( dimension    << 28 /*  4 bits, max    7 */ ) |
                   ( face_count   << 22 /*  6 bits, max   63 */ ) |
                   ( edge_count   << 16 /*  6 bits, max   63 */ ) |
                   ( vertex_count << 10 /*  6 bits, max   63 */ ) |
                   ( node_count         /* 10 bits, max 1023 */ ) };

  /** \brief Flag if the subcells of a given dimension are homogeneous */
  enum { subcell_homogeneity = TypeListHomogeneous<EdgeList>::value &&
                               TypeListHomogeneous<FaceList>::value };

  /** \brief Subcell information
   *
   *  - <b> subcell<Dim>::count        </b> Number of subcells of this dimension
   *  - <b> subcell<Dim,Ord>::topology </b> topology of the subcell
   *  - <b> subcell<Dim,Ord,J>::node   </b> node ordinal of subcell's node J
   */
  template< unsigned Dim, unsigned Ord = 0, unsigned J = 0 >
  struct subcell :
    public SubcellTopologyTraits< Dim , Ord , J ,
                                  dimension , vertex_count , node_count ,
                                  EdgeList , EdgeMaps ,
                                  FaceList , FaceMaps > {};

  /** \brief Side subcell information
   *
   *  - <b> side<>::count       </b> Number of sides
   *  - <b> side<Ord>::topology </b> topology of the side
   *  - <b> side<Ord,J>::node   </b> node ordinal of side's node J
   */
  template< unsigned Ord = 0 , unsigned J = 0 >
  struct side :
    public SubcellTopologyTraits< ( 1 < dimension ? dimension - 1 : 4 ) ,
                                  Ord , J ,
                                  dimension , vertex_count , node_count ,
                                  EdgeList , EdgeMaps ,
                                  FaceList , FaceMaps > {};

  /** \brief Edge subcell information
   *
   *  - <b> edge<>::count       </b> Number of edge
   *  - <b> edge<Ord>::topology </b> topology of the edge
   *  - <b> edge<Ord,J>::node   </b> node ordinal of edge's node J
   */
  template< unsigned Ord = 0 , unsigned J = 0 >
  struct edge :
    public SubcellTopologyTraits< ( 1 < dimension ? 1 : 4 ) , Ord , J ,
                                  dimension , vertex_count , node_count ,
                                  EdgeList , EdgeMaps ,
                                  TypeListEnd , TypeListEnd > {};

private:

#ifndef DOXYGEN_COMPILE

  enum { nedge_map = TypeListLength<EdgeMaps>::value ,
         nface_map = TypeListLength<FaceMaps>::value };

  enum { OK_edge  = AssertEqual< edge_count , nedge_map >::OK };
  enum { OK_face  = AssertEqual< face_count , nface_map >::OK };
  enum { OK_dimen = AssertEqual< 0 , dimension    >>  3 >::OK };
  enum { OK_faceN = AssertEqual< 0 , face_count   >>  6 >::OK };
  enum { OK_edgeN = AssertEqual< 0 , edge_count   >>  6 >::OK };
  enum { OK_vertN = AssertEqual< 0 , vertex_count >>  6 >::OK };
  enum { OK_nodeN = AssertEqual< 0 , node_count   >> 10 >::OK };

#endif

};

/** \} */

} // namespace shards

#endif // Shards_CellTopologyTraits_hpp

