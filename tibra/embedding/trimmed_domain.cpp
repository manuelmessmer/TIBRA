// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

//// STL includes
#include <random>
//// Project includes
#include "define.hpp"
#include "embedding/trimmed_domain.h"
#include "embedding/ray_aabb_primitive.h"
#include "utilities/mesh_utilities.h"
#include "io/io_utilities.h"

namespace tibra {

typedef TrimmedDomainBase::BoundaryIPVectorPtrType BoundaryIPVectorPtrType;
typedef TrimmedDomainBase::BoundingBox BoundingBox;

bool TrimmedDomain::IsInsideTrimmedDomain(const PointType& rPoint) const {

    const IndexType num_triangles = mClippedMesh.NumOfTriangles();
    if( num_triangles == 0){
        return true;
    }
    bool try_next_triangle = true;
    bool is_inside = false;
    IndexType current_id = 0;
    while( try_next_triangle ){
        // Return false if all triangles are tested, but non valid (all are parallel or on_boundary)
        if( current_id >= num_triangles ){
            return false;
        }
        // Get direction
        const auto center_triangle = mClippedMesh.Center(current_id);
        Vector3d direction = center_triangle - rPoint;

        // Normalize
        double norm_direction = direction.Norm();
        direction /= norm_direction;

        // Construct ray
        Ray_AABB_primitive ray(rPoint, direction);

        // Get vertices of current triangle
        const auto& p1 = mClippedMesh.P1(current_id);
        const auto& p2 = mClippedMesh.P2(current_id);
        const auto& p3 = mClippedMesh.P3(current_id);

        // Make sure target triangle is not parallel
        if( !ray.is_parallel(p1, p2, p3) ) {
            // Get potential ray intersections from AABB tree.
            auto potential_intersections = mTree.Query(ray);

            if( potential_intersections.size() == 0){
                throw std::runtime_error("TrimmedDomain :: IsInsideTrimmedDomain :: No potential intersections found.");
            }
            // Test if potential intersections actually intersect.
            // If intersection lies on boundary cast a new ray.
            // @todo Use symbolic perturbations: http://dl.acm.org/citation.cfm?id=77639
            try_next_triangle = false;
            double min_distance = MAXD;
            is_inside = false;
            IndexType invalid_count = 0;
            IndexType count_intersect = 0;
            for( auto r : potential_intersections){
                const auto& p1 = mClippedMesh.P1(r);
                const auto& p2 = mClippedMesh.P2(r);
                const auto& p3 = mClippedMesh.P3(r);
                double t, u, v;
                bool back_facing, parallel;
                if( ray.intersect(p1, p2, p3, t, u, v, back_facing, parallel) ) {
                    if( !parallel ){
                        double sum_u_v = u+v;
                        if( t < EPS2 ){ // origin lies on boundary
                            return false;
                        }
                        // Ray shoots through boundary.
                        if( u < 0.0+EPS3 || v < 0.0+EPS3 || sum_u_v > 1.0-EPS3 ){
                            try_next_triangle = true;
                            break;
                        }
                        if( t < min_distance ){
                            is_inside = back_facing;
                            min_distance = t;
                        }
                    }
                }
            }
        }
        current_id++;
    }
    return is_inside;
}

const BoundingBox TrimmedDomain::GetBoundingBoxOfTrimmedDomain() const {
    // Initialize bounding box
    BoundingBox bounding_box = { {MAXD, MAXD, MAXD},
                                 {LOWESTD, LOWESTD, LOWESTD} };

    // Loop over all vertices
    const auto& vertices = mpTriangleMesh->GetVertices();
    for( auto& v : vertices ){
        // Loop over all 3 dimensions
        for( IndexType i = 0; i < 3; ++i){
            if( v[i] < bounding_box.first[i] ){ // Find min values
                bounding_box.first[i] = v[i];
            }
            if( v[i] > bounding_box.second[i] ){ // Find max values
                bounding_box.second[i] = v[i];
            }
        }
    }

    return bounding_box;
}

BoundaryIPVectorPtrType TrimmedDomain::pGetBoundaryIps() const{
    // Pointer to boundary integration points
    auto p_boundary_ips = MakeUnique<BoundaryIPVectorType>();

    p_boundary_ips->reserve(mpTriangleMesh->NumOfTriangles()*6UL);
    for( IndexType triangle_id = 0; triangle_id < mpTriangleMesh->NumOfTriangles(); ++triangle_id ){
        IndexType method = 3; // Creates 6 points per triangle.
        auto p_new_points = mpTriangleMesh->pGetIPsGlobal(triangle_id, method);
        p_boundary_ips->insert(p_boundary_ips->end(), p_new_points->begin(), p_new_points->end());
    }

    return std::move(p_boundary_ips);
}

} // End namespace tibra