/*
//@HEADER
// ************************************************************************
//
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions?  Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_SERIAL_HPP
#define KOKKOS_SERIAL_HPP

#include <cstddef>
#include <iosfwd>
#include <Kokkos_Parallel.hpp>
#include <Kokkos_Layout.hpp>
#include <Kokkos_HostSpace.hpp>
#include <Kokkos_MemoryTraits.hpp>

/*--------------------------------------------------------------------------*/

namespace Kokkos {

/** \brief Kokkos device for non-parallel execution */
class Serial {
public:
  //! \name Type declarations that all Kokkos devices must provide.
  //@{

  typedef Serial                device_type ;
  typedef HostSpace::size_type  size_type ;
  typedef HostSpace             memory_space ;
  typedef LayoutRight           array_layout ;
  typedef Serial                host_mirror_device_type ;

  //@}
  //! \name Functions that all Kokkos devices must implement.
  //@{

  static bool in_parallel() { return false ; }

  /** \brief  Set the device in a "sleep" state.
   *
   * This function sets the device in a "sleep" state in which it is
   * not ready for work.  This may consume less resources than if the
   * device were in an "awake" state, but it may also take time to
   * bring the device from a sleep state to be ready for work.
   *
   * \return True if the device is in the "sleep" state, else false if
   *   the device is actively working and could not enter the "sleep"
   *   state.
   */
  static bool sleep();

  /// \brief Wake the device from the 'sleep' state so it is ready for work.
  ///
  /// \return True if the device is in the "ready" state, else "false"
  ///  if the device is actively working (which also means that it's
  ///  awake).
  static bool wake();

  /// \brief Wait until all dispatched functors complete.
  ///
  /// The parallel_for or parallel_reduce dispatch of a functor may
  /// return asynchronously, before the functor completes.  This
  /// method does not return until all dispatched functors on this
  /// device have completed.
  static void fence() {}

  /// \brief Free any resources being consumed by the device.
  ///
  /// For the Serial device, this terminates spawned worker threads.
  static void finalize();

  /** \brief  Print Serial configuration information */
  static void print_configuration( std::ostream & );

  //----------------------------------------
  /** \name Function for the functor device interface */
  /**@{ */

  inline int league_rank() const { return 0 ; }
  inline int league_size() const { return 1 ; }
  inline int team_rank() const { return 0 ; }
  inline int team_size() const { return 1 ; }

  inline void team_barrier() {}

  inline std::pair<size_t,size_t> work_range( size_t n ) const 
    { return std::pair<size_t,size_t>(0,n); }

  template< typename T >
  inline T * get_shmem( const int count );

  /**@} */
  //----------------------------------------

  static void * resize_reduce_scratch( const unsigned );
};

} // namespace Kokkos

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace Kokkos {
namespace Impl {

//----------------------------------------------------------------------------
//TODO: Needs constructor for Kokkos::ParallelWorkRequest CRT

template< class FunctorType , class WorkSpec >
class ParallelFor< FunctorType , WorkSpec , Serial > {
public:

  ParallelFor( const FunctorType & functor , const size_t work_count )
    {
      for ( size_t iwork = 0 ; iwork < work_count ; ++iwork ) {
        functor( iwork );
      }
    }
};

template< class FunctorType , class WorkSpec >
class ParallelReduce< FunctorType , WorkSpec , Serial > {
public:

  typedef ReduceAdapter< FunctorType >  Reduce ;
  typedef typename Reduce::pointer_type pointer_type ;

  ParallelReduce( const FunctorType  & functor ,
                  const size_t         work_count ,
                  pointer_type         result = 0 )
    {
      if ( 0 == result ) {
        result = (pointer_type ) Serial::resize_reduce_scratch( Reduce::value_size( functor ) );
      }

      functor.init( Reduce::reference( result ) );

      for ( size_t iwork = 0 ; iwork < work_count ; ++iwork ) {
        functor( iwork , Reduce::reference( result ) );
      }

      Reduce::final( functor , result );
    }

  void wait() {}
};

//----------------------------------------------------------------------------

} // namespace Impl
} // namespace Kokkos

#endif /* #define KOKKOS_SERIAL_HPP */

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

