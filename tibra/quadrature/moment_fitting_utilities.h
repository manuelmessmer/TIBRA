// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

#ifndef MOMENT_FITTING_UTILITIES_INCLUDE_H
#define MOMENT_FITTING_UTILITIES_INCLUDE_H

//// STL includes
#include <boost/numeric/ublas/matrix.hpp>
#include <vector>
#include <array>
#include <variant>
//// Project includes
#include "embedding/octree.h"
#include "containers/element.hpp"
#include "containers/boundary_integration_point.hpp"
#include "utilities/parameters.h"

namespace tibra {

///@name TIBRA Classes
///@{

/**
 * @class  MomentFitting.
 * @author Manuel Messmer
 * @brief  Provides functions to create integration rules for trimmed domains.
 * @todo   Refactor and rename to moment fitting or point elimination.
**/
class MomentFitting{
public:
    ///@name Type Definition
    ///@{
    typedef Element::IntegrationPointVectorType IntegrationPointVectorType;
    typedef std::vector<BoundaryIntegrationPoint> BoundaryIPsVectorType;
    typedef Unique<BoundaryIPsVectorType> BoundaryIPsVectorPtrType;
    typedef boost::numeric::ublas::vector<double> VectorType;

    ///@}
    ///@name Operations
    ///@{

    ///@brief Creates integration points for trimmed domain.
    ///@details 1. Distributes initial integration points uniformly in trimmed domain.
    ///         2. Computes constant terms of moment fitting equation.
    ///         3. Solves moment fitting equation in iterative point elimination algorithm.
    /// See: M. Meßmer et. al: Efficient CAD-integrated isogeometric analysis of trimmed solids,
    ///      Comput. Methods Appl. Mech. Engrg. 400 (2022) 115584, https://doi.org/10.1016/j.cma.2022.115584.
    ///@param rElement
    ///@param rParam
    static void CreateIntegrationPointsTrimmed(Element& rElement, const Parameters& rParam);

    static void ComputeConstantTerms(const Element& rElement, const BoundaryIPsVectorPtrType& rBoundaryIps, VectorType& rConstantTerms, const Parameters& rParam);

    static double MomentFitting1(const VectorType& rConstantTerms, IntegrationPointVectorType& rIntegrationPoint, const Element& rElement, const Parameters& rParam);

    ///@}
private:
    ///@name Private Operations
    ///@{

    ///@todo Clean this up.
    static double PointElimination(const VectorType& rConstantTerms, IntegrationPointVectorType& rIntegrationPoint, Element& rElement, const Parameters& rParam);

    static void DistributeIntegrationPoints(Octree<TrimmedDomainBase>& rOctree, IntegrationPointVectorType& rIntegrationPoint, SizeType MinNumPoints, Element& rElement, const Parameters& rParam);

}; // End Class

} // End namespace tibra

#endif // MOMENT_FITTING_UTILITIES_INCLUDE_H