// @HEADER
// ***********************************************************************
//
//                    Teuchos: Common Tools Package
//                 Copyright (2004) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER

#ifndef TEUCHOS_DEFAULT_COMM_HPP
#define TEUCHOS_DEFAULT_COMM_HPP

#include "Teuchos_RCP.hpp"
#include "Teuchos_DefaultSerialComm.hpp"
#ifdef HAVE_MPI
#  include "Teuchos_DefaultMpiComm.hpp"
#endif

namespace Teuchos {

#ifdef HAVE_MPI
namespace Details {

template<class OrdinalType>
int
mpiFreeDefaultComm (MPI_Comm, int, void*, void*);

template<class OrdinalType>
int
mpiFreeDefaultSerialComm (MPI_Comm, int, void*, void*);

} // namespace Details
#endif // HAVE_MPI

/// \class DefaultComm
/// \brief Return a default global communicator appropriate for the build.
///
/// Use this class to get a Comm instance representing the default
/// global communicator.  If Teuchos was built with MPI (i.e., if the
/// HAVE_MPI macro is defined), then the default communicator wraps
/// MPI_COMM_WORLD.  Otherwise, it is a "serial" communicator
/// (containing one process, whose rank is zero).
///
/// \tparam OrdinalType The ordinal type for the Comm communicator
///   wrapper template class.  Comm uses \c OrdinalType to represent
///   things like array lengths and indices.
///
/// \note (mfh 19 Jul 2011, 22 Dec 2011) \c OrdinalType is called
///   <tt>OrdinalType</tt> and not \c Ordinal, because of a bug in
///   Intel's C++ compiler (version 11.1).  This compiler confuses the
///   \c Ordinal template parameter of DefaultComm with the
///   Teuchos::Ordinal typedef.  The \c Ordinal template parameter
///   should actually shadow the typedef in Teuchos, and it does with
///   GCC 4.5.1, but does not with Intel's compiler.  This may be the
///   case with other compilers as well, but I haven't tested them
///   yet.  If this class' template parameter were named \c Ordinal,
///   then the following line of code would result in a compile error:
///   \code
///   RCP<const Comm<int> pComm = DefaultComm<int>::getDefaultSerialComm (null);
///   \endcode
template<typename OrdinalType>
class DefaultComm {
public:
  /// \brief Return the default global communicator.
  ///
  /// \warning When running with MPI, do not call this function until
  ///   after MPI_Init has been called.  You can use GlobalMPISesssion
  ///   to initialize MPI without explicitly depending on the MPI
  ///   interface or the mpi.h header file.  (If Trilinos was not
  ///   built with MPI, GlobalMPISession will do the right thing, so
  ///   you can use it unconditionally.)
  ///
  /// \warning Do not use or refer to the returned object after
  ///   MPI_Finalize has been called.  In a non-MPI build, do not use
  ///   or refer to the returned object after main() has returned.
  static Teuchos::RCP<const Comm<OrdinalType> > getComm ();

  /// \brief Return a serial Comm if the input Comm is null.
  ///
  /// If the input communicator \c comm is null, return the default
  /// serial communicator.  Otherwise, just return the input.
  ///
  /// \warning The same warnings as for getComm() apply here.
  static Teuchos::RCP<const Comm<OrdinalType> >
  getDefaultSerialComm (const Teuchos::RCP<const Comm<OrdinalType> >& comm);

private:
  /// \brief The default global communicator.
  ///
  /// If Teuchos was built with MPI, this is a wrapper for
  /// MPI_COMM_WORLD.  Otherwise, this is a "serial" communicator
  /// (containing one process, whose rank is zero).
  static const Comm<OrdinalType>* comm_;

  //! A "serial" communicator (containing one process, whose rank is zero).
  static const Comm<OrdinalType>* defaultSerialComm_;

#ifdef HAVE_MPI
  //! MPI_Finalize hook that calls freeDefaultComm().
  template<class OT>
  friend int
  Details::mpiFreeDefaultComm (MPI_Comm, int, void*, void*);

  //! MPI_Finalize hook that calls freeDefaultSerialComm().
  template<class OT>
  friend int
  Details::mpiFreeDefaultSerialComm (MPI_Comm, int, void*, void*);
#endif // HAVE_MPI

  //! Free the default Comm object.
  static void freeDefaultComm () {
    if (comm_ != NULL) {
      delete comm_;
      comm_ = NULL;
    }
  }

  //! Free the default serial Comm object.
  static void freeDefaultSerialComm () {
    if (defaultSerialComm_ != NULL) {
      delete defaultSerialComm_;
      defaultSerialComm_ = NULL;
    }
  }
};

#ifdef HAVE_MPI
namespace Details {

template<class OrdinalType>
int
mpiFreeDefaultComm (MPI_Comm, int, void*, void*)
{
  try {
    ::Teuchos::DefaultComm<OrdinalType>::freeDefaultComm ();
  } catch (...) {
    // Destructors must not throw exceptions, so we must accept the
    // possible memory leak and move on.
    std::cerr << "Teuchos::DefaultComm: Failed to free default Comm!  We can't "
      "throw an exception here because this is a singleton destructor that "
      "should only be called at MPI_Finalize or (if not building with MPI) at "
      "exit from main()." << std::endl;
    // FIXME (mfh 16 Nov 2014) There might be some way to create a
    // custom return code with MPI error reporting.  For now, we just
    // pick some error code not equal to MPI_SUCCESS.  It could
    // perhaps overlap with some existing error code.
    return (MPI_SUCCESS == 0) ? -1 : 0;
  }
  return MPI_SUCCESS;
}

template<class OrdinalType>
int
mpiFreeDefaultSerialComm (MPI_Comm, int, void*, void*)
{
  try {
    ::Teuchos::DefaultComm<OrdinalType>::freeDefaultSerialComm ();
  } catch (...) {
    // Destructors must not throw exceptions, so we must accept the
    // possible memory leak and move on.
    std::cerr << "Teuchos::DefaultComm: Failed to free default serial Comm!  "
      "We can't throw an exception here because this is a singleton destructor "
      "that should only be called at MPI_Finalize or (if not building with MPI)"
      " at exit from main()." << std::endl;
    // FIXME (mfh 16 Nov 2014) There might be some way to create a
    // custom return code with MPI error reporting.  For now, we just
    // pick some error code not equal to MPI_SUCCESS.  It could
    // perhaps overlap with some existing error code.
    return (MPI_SUCCESS == 0) ? -1 : 0;
  }
  return MPI_SUCCESS;
}

} // namespace Details
#endif // HAVE_MPI


template<typename OrdinalType>
Teuchos::RCP<const Teuchos::Comm<OrdinalType> >
DefaultComm<OrdinalType>::getComm ()
{
  if (comm_ == NULL) {
#ifdef HAVE_MPI
    comm_ = new MpiComm<OrdinalType> (MPI_COMM_WORLD);

    // We want comm_ to be deallocated when MPI_Finalize is called.
    // The standard idiom for this (look in the MPI standard) is to
    // register an attribute ((key,value) pair) with MPI_COMM_SELF,
    // with a custom "destructor" to be called at MPI_Finalize.

    // Key is an output argument of MPI_Comm_create_keyval.  If we
    // ever wanted to call MPI_Comm_free_keyval, we would need to save
    // the key.  MPI_Finalize will free the (key,value) pair
    // automatically, so we never need to call MPI_Comm_free_keyval.
    int key = MPI_KEYVAL_INVALID;
    int err =
      MPI_Comm_create_keyval (MPI_COMM_NULL_COPY_FN,
                              Details::mpiFreeDefaultComm<OrdinalType>,
                              &key,
                              NULL); // no extra state
    if (err != MPI_SUCCESS) {
      if (comm_ != NULL) { // clean up if MPI call fails
        delete comm_;
        comm_ = NULL;
      }
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "Teuchos::DefaultComm::getComm: MPI_Comm_create_keyval failed!");
    }
    int val = key; // doesn't matter

    // Attach the attribute to MPI_COMM_SELF.
    err = MPI_Comm_set_attr (MPI_COMM_SELF, key, &val);
    if (err != MPI_SUCCESS) {
      if (comm_ != NULL) { // clean up if MPI call fails
        delete comm_;
        comm_ = NULL;
      }
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "Teuchos::DefaultComm::getComm: MPI_Comm_set_attr failed!");
    }
#else // NOT HAVE_MPI
    comm_ = new SerialComm<OrdinalType> ();
    // We want comm_ to be deallocated when main exits, so register
    // its deallocation function as an atexit handler.
    //
    // The POSIX standard allows atexit to fail, in particular if it
    // lacks space for registering more functions.  "[T]he application
    // should call sysconf() to obtain the value of {ATEXIT_MAX}, the
    // [maximum] number of functions that can be registered. There is
    // no way for an application to tell how many functions have
    // already been registered with atexit()."
    //
    // We don't do this here.  Instead, we just check atexit's return
    // code.  If it fails, we throw.
    int err = atexit (freeDefaultComm);
    if (err != 0) {
      if (comm_ != NULL) { // clean up if atexit fails
        delete comm_;
        comm_ = NULL;
      }
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "Teuchos::DefaultComm::getComm: atexit failed!");
    }
#endif // HAVE_MPI
  }
  // Return a nonowning RCP, because we need to ensure that
  // destruction happens at MPI_Finalize (or at exit of main(), if not
  // building with MPI).
  return rcp (comm_, false);
}

template<typename OrdinalType>
Teuchos::RCP<const Teuchos::Comm<OrdinalType> >
DefaultComm<OrdinalType>::
getDefaultSerialComm (const Teuchos::RCP<const Comm<OrdinalType> >& comm)
{
  if (! comm.is_null ()) {
    return comm;
  } else {
    if (defaultSerialComm_ == NULL) {
#ifdef HAVE_MPI
      //defaultSerialComm_ = new MpiComm<OrdinalType> (MPI_COMM_SELF);
      defaultSerialComm_ = new SerialComm<OrdinalType> ();

      // Register an MPI_Finalize hook to free defaultSerialComm_.
      // (See getComm implementation above in this file for details.)

      int key = MPI_KEYVAL_INVALID;
      int err =
        MPI_Comm_create_keyval (MPI_COMM_NULL_COPY_FN,
                                Details::mpiFreeDefaultSerialComm<OrdinalType>,
                                &key,
                                NULL); // no extra state
      if (err != MPI_SUCCESS) {
        if (defaultSerialComm_ != NULL) { // clean up if MPI call fails
          delete defaultSerialComm_;
          defaultSerialComm_ = NULL;
        }
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::runtime_error, "Teuchos::DefaultComm::getDefaultSerialComm"
          ": MPI_Comm_create_keyval failed!");
      }
      int val = key; // doesn't matter

      // Attach the attribute to MPI_COMM_SELF.
      err = MPI_Comm_set_attr (MPI_COMM_SELF, key, &val);
      if (err != MPI_SUCCESS) {
        if (defaultSerialComm_ != NULL) { // clean up if MPI call fails
          delete defaultSerialComm_;
          defaultSerialComm_ = NULL;
        }
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::runtime_error, "Teuchos::DefaultComm::getDefaultSerialComm"
          ": MPI_Comm_set_attr failed!");
      }
#else // NOT HAVE_MPI
      defaultSerialComm_ = new SerialComm<OrdinalType> ();
      // We want defaultSerialComm_ to be deallocated when main exits,
      // so register its deallocation function as an atexit handler.
      //
      // The POSIX standard allows atexit to fail, in particular if it
      // lacks space for registering more functions.  "[T]he
      // application should call sysconf() to obtain the value of
      // {ATEXIT_MAX}, the [maximum] number of functions that can be
      // registered. There is no way for an application to tell how
      // many functions have already been registered with atexit()."
      //
      // We don't do this here.  Instead, we just check atexit's
      // return code.  If it fails, we throw.
      int err = atexit (freeDefaultComm);
      if (err != 0) {
        if (defaultSerialComm_ != NULL) { // clean up if atexit fails
          delete defaultSerialComm_;
          defaultSerialComm_ = NULL;
        }
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
          "Teuchos::DefaultComm::getDefaultSerialComm: atexit failed!");
      }
#endif // HAVE_MPI
    }
    // Return a nonowning RCP, because we need to ensure that
    // destruction happens at MPI_Finalize (or at exit of main(), if not
    // building with MPI).
    return rcp (defaultSerialComm_, false);
  }
}

template<typename OrdinalType>
const Teuchos::Comm<OrdinalType>*
DefaultComm<OrdinalType>::comm_ = NULL;

template<typename OrdinalType>
const Teuchos::Comm<OrdinalType>*
DefaultComm<OrdinalType>::defaultSerialComm_ = NULL;

} // namespace Teuchos

#endif // TEUCHOS_DEFAULT_COMM_HPP
