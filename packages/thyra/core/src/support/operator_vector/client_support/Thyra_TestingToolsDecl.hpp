// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
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

#ifndef THYRA_TESTING_TOOLS_DECL_HPP
#define THYRA_TESTING_TOOLS_DECL_HPP

#include "Thyra_OperatorVectorTypes.hpp"
#include "Teuchos_VerbosityLevel.hpp"
#include "Teuchos_TestingHelpers.hpp"

namespace Thyra {


/** \brief .
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
inline const std::string passfail(const bool result)
{
  return Teuchos::passfail(result);
}


/** \brief .
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template <class Scalar>
inline
typename Teuchos::ScalarTraits<Scalar>::magnitudeType
relErr( const Scalar &s1, const Scalar &s2 )
{
  return Teuchos::relErr<Scalar>(s1, s2);
}

/** \brief Return relative error of two vectors.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template <class Scalar>
typename Teuchos::ScalarTraits<Scalar>::magnitudeType
relVectorErr( const VectorBase<Scalar> &v1, const VectorBase<Scalar> &v2 );

/** \brief Compute, check and optionally print the relative error in two scalars.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
inline
bool testRelErr(
  const std::string &v1_name
  ,const Scalar &v1
  ,const std::string &v2_name
  ,const Scalar &v2
  ,const std::string &maxRelErr_error_name
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType &maxRelErr_error
  ,const std::string &maxRelErr_warning_name
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType &maxRelErr_warning
  ,std::ostream *out
  ,const std::string &leadingIndent = std::string("")
  )
{
  // ToDo: Indent correctly!
  return Teuchos::testRelErr(v1_name, v1, v2_name, v2,
    maxRelErr_error_name, maxRelErr_error,
    maxRelErr_warning_name, maxRelErr_warning,
    Teuchos::ptr(out) );
}

/** \brief Compute, check and optionally print the relative errors in two
 * scalar arays.
 *
 * \param v1  [in] Array (length <tt>num_scalars</tt>).
 * \param v2  [in] Array (length <tt>num_scalars</tt>).
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar1, class Scalar2, class ScalarMag>
bool testRelErrors(
  const int                                                     num_scalars
  ,const std::string                                            &v1_name
  ,const Scalar1                                                v1[]
  ,const std::string                                            &v2_name
  ,const Scalar2                                                v2[]
  ,const std::string                                            &maxRelErr_error_name
  ,const ScalarMag                                              &maxRelErr_error
  ,const std::string                                            &maxRelErr_warning_name
  ,const ScalarMag                                              &maxRelErr_warning
  ,std::ostream                                                 *out
  ,const std::string                                            &leadingIndent = std::string("")
  );

/** \brief Compute, check and optionally print the relative errors in two vectors.
 *
 * This function only looks at the difference in the relative errors in the
 * natural norm of the difference between two vectors.  This does not perform
 * a component-by-component check.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
bool testRelNormDiffErr(
  const std::string &v1_name,
  const VectorBase<Scalar> &v1,
  const std::string &v2_name,
  const VectorBase<Scalar> &v2,
  const std::string &maxRelErr_error_name,
  const typename Teuchos::ScalarTraits<Scalar>::magnitudeType &maxRelErr_error,
  const std::string &maxRelErr_warning_name,
  const typename Teuchos::ScalarTraits<Scalar>::magnitudeType &maxRelErr_warning,
  std::ostream *out,
  const Teuchos::EVerbosityLevel verbLevel = Teuchos::VERB_LOW,
  const std::string &leadingIndent = std::string("")
  );

/** \brief Check that an error is less than some error tolerence.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
bool testMaxErr(
  const std::string                                             &error_name
  ,const Scalar                                                 &error
  ,const std::string                                            &max_error_name
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType  &max_error
  ,const std::string                                            &max_warning_name
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType  &max_warning
  ,std::ostream                                                 *out
  ,const std::string                                            &leadingIndent = std::string("")
  );

/** \brief Check that an array of errors is less than some error tolerence.
 *
 * \param error  [in] Array (length <tt>num_scalars</tt>).
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
bool testMaxErrors(
  const int                                                     num_scalars
  ,const std::string                                            &error_name
  ,const Scalar                                                 error[]
  ,const std::string                                            &max_error_name
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType  &max_error
  ,const std::string                                            &max_warning_name
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType  &max_warning
  ,std::ostream                                                 *out
  ,const std::string                                            &leadingIndent = std::string("")
  );

/** \brief Check a boolean result against expected result.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
bool testBoolExpr(
  const std::string    &boolExprName
  ,const bool          &boolExpr
  ,const bool          &boolExpected
  ,std::ostream        *out
  ,const std::string   &leadingIndent = std::string("")
  );

/** \brief Print summary outputting for a test or just <tt>passed</tt> or
 * <tt>failed</tt>.
 *
 * @param  result          [in] Bool for of the test was successful or unsuccessful.
 * @param  test_summary    [in] The summary of the test that was just completed.
 * @param  show_all_tests  [in] Bool for if the test summary should be shown even if
 *                         the test passed.
 * @param  success         [out] Update of the success bool.
 * @param  out             [out] Stream where output will be sent if <tt>*out!=NULL</tt>.
 *
 * Preconditions:<ul>
 * <li><tt>success!=NULL</tt>
 * </ul>
 *
 * Preconditions:<ul>
 * <li><tt>*success==false</tt> if <tt>result==false</tt>
 * </ul>
 * 
 * Just look at the definition of this function to see what it does.
 */
void printTestResults(
  const bool              result
  ,const std::string      &test_summary
  ,const bool             show_all_tests
  ,bool                   *success
  ,std::ostream           *out
  );

/** \brief Output operator to pretty print any <tt>Thyra::VectorBase</tt> object.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
std::ostream& operator<<( std::ostream& o, const VectorBase<Scalar>& v );

/** \brief Output operator to pretty print any <tt>Thyra::LinearOpBase</tt> object.
 *
 * Calls <tt>M.describe(o,Teuchos::VERB_EXTREME);</tt>
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_grp
 */
template<class Scalar>
std::ostream& operator<<( std::ostream& o, const LinearOpBase<Scalar>& M );

} // namespace Thyra

// //////////////////////////
// Inline functions                        

inline
void Thyra::printTestResults(
  const bool              result
  ,const std::string      &test_summary
  ,const bool             show_all_tests
  ,bool                   *success
  ,std::ostream           *out
  )
{
  if(!result) *success = false;
  if(out) {
    if( !result || show_all_tests )
      *out << std::endl << test_summary;
    else
      *out << "passed!\n";
  }
}

#endif // THYRA_TESTING_TOOLS_DECL_HPP
