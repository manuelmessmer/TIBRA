// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

#ifndef TRIMMED_DOMAIN_INCLUDE_H
#define TRIMMED_DOMAIN_INCLUDE_H

/// STL includes
#include <memory>
/// Project includes
#include "embedding/trimmed_domain_base.h"
#include "embedding/aabb_tree.h"

namespace tibra {

///@name TIBRA Classes
///@{

/**
 * @class  TrimmedDomain
 * @author Manuel Messmer
 * @brief  Provides geometrical operations for clipped Brep models (clipped triangle meshes).
 * @details Uses AABB Tree for fast search.
*/
class TrimmedDomain : public TrimmedDomainBase {

public:
    ///@name Type Definitions
    ///@{

    ///@}
    ///@name Life Cycle
    ///@{

    /// Constructor
    ///@brief Builds AABB tree for given mesh.
    ///@param pClippedTriangleMesh
    ///@note mpTriangleMesh must be passed to mTree() and not mpTriangleMesh(), since ptr is moved!
    TrimmedDomain(TriangleMeshPtrType pTriangleMesh, const PointType& rLowerBound, const PointType& rUpperBound )
        : TrimmedDomainBase(std::move(pTriangleMesh), rLowerBound, rUpperBound), mTree(GetTriangleMesh())
    {
    }

    ///@}
    ///@name Operations
    ///@{

    ///@brief Returns true if point is inside TrimmedDomain. Expects point to be inside AABB. Check is omitted.
    ///@brief Performs ray tracing in direction of the first triangle. Search for all intersection of ray. Inside/Outside is detected
    ///       based on the orientation of the closest intersected triangle (forward or backward facing).
    ///@param rPoint
    ///@return bool
    bool IsInsideTrimmedDomain(const PointType& rPoint) const override;

    ///@brief Prototpye of: Clips triangle mesh by AABB and computes boundary edges.
    ///@note This is just a protope and not ready to be used!
    ///@details Projects trimmed domain onto plane at z=lower_bound and z=upper_bound and constructs integration points for trimmed domain.
    ///@param rLowerBound of AABB.
    ///@param rUpperBound of AABB.
    ///@return BoundaryIPVectorPtrType. Boundary integration points to be used for ConstantTerms::Compute.
    BoundaryIPVectorPtrType pGetBoundaryIps() const;

    /// @brief Returns bounding box of trimmed domain. (Might be smaller than the actual domain of element.)
    /// @return BoundingBox (std::pair: first - lower_bound, second - upper_bound)
    const BoundingBox GetBoundingBoxOfTrimmedDomain() const override;

    const TriangleMesh& GetClippedMesh() const {
        return *mpTriangleMesh.get();
    }
    ///@}
private:
    ///@name Private Operations
    ///@{



    ///@brief Returns true if point is inside AABB.
    ///@param rPoint Query point.
    ///@param rLowerBound of AABB.
    ///@param rUpperBound of AABB.
    ///@return bool
    inline bool IsContained(const PointType& rPoint, const PointType& rLowerBound, const PointType& rUpperBound) const{
        if(    rPoint[0] < rLowerBound[0]
            || rPoint[0] > rUpperBound[0]
            || rPoint[1] < rLowerBound[1]
            || rPoint[1] > rUpperBound[1]
            || rPoint[2] < rLowerBound[2]
            || rPoint[2] > rUpperBound[2] )
        {
            return false;
        }

        return true;
    }

    ///@}
    ///@name Private Members
    ///@{

    AABB_tree mTree;
    ///@}
};

///@}

} // End namespace tibra

#endif // TRIMMED_DOMAIN_INCLUDE_H