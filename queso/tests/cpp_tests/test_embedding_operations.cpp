// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

#define BOOST_TEST_DYN_LINK

//// External includes
#include <boost/test/unit_test.hpp>
//// Project includes
#include "utilities/mapping_utilities.h"
#include "io/io_utilities.h"
#include "QuESo_main.h"

namespace queso {
namespace Testing {

BOOST_AUTO_TEST_SUITE( EmbeddingOperationsTestSuite )

BOOST_AUTO_TEST_CASE(Intersection) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Intersected Knot Span" << std::endl;

    std::string filename = "queso/tests/cpp_tests/data/cylinder.stl";
    Parameters parameters( {Component("input_filename", filename),
                            Component("lower_bound_xyz", PointType(0.0, 0.0, 0.0)),
                            Component("upper_bound_xyz", PointType(2.0, 2.0, 1.0)),
                            Component("lower_bound_uvw", PointType(0.0, 0.0, 0.0)),
                            Component("upper_bound_uvw", PointType(4.0, 5.0, 3.0)),
                            Component("number_of_elements", Vector3i(1, 1, 1)),
                            Component("polynomial_order", Vector3i(2, 2, 2)),
                            Component("integration_method", IntegrationMethod::Gauss),
                            Component("init_point_distribution_factor", 3UL),
                            Component("min_num_boundary_triangles", 5000UL),
                            Component("moment_fitting_residual", 1e-8) });
    QuESo queso(parameters);
    queso.Run();

    const auto& elements = queso.GetElements();

    BOOST_CHECK_EQUAL(elements.size(), 1);

    const auto& points_reduced = (*elements.begin())->GetIntegrationPoints();
    BOOST_CHECK_LT(points_reduced.size(), 28);

    const auto& r_triangle_mesh = (*elements.begin())->pGetTrimmedDomain()->GetTriangleMesh();
    const std::size_t num_triangles = r_triangle_mesh.NumOfTriangles();

    BOOST_CHECK_GT(num_triangles, parameters.MinimumNumberOfTriangles());

    double area = 0.0;
    for( std::size_t triangle_id = 0; triangle_id < num_triangles; ++triangle_id){
        const PointType coordinates = r_triangle_mesh.Center(triangle_id);

        BOOST_CHECK_GT(coordinates[2], -1e-6);
        BOOST_CHECK_LT(coordinates[2], 1.0+1e-6);

        BOOST_CHECK_GT(coordinates[0], -1e-6);
        BOOST_CHECK_GT(coordinates[1], -1e-6);

        if( coordinates[0] > 1e-6 && coordinates[1] > 1e-6
            && coordinates[2] > 1e-6 && coordinates[2] < 1-1e-6){ // check in z richtung fehlt noch
            double radius = std::sqrt( std::pow(coordinates[0],2) + std::pow(coordinates[1], 2) );
            BOOST_CHECK_GT(radius, 0.998);
        }
        area += r_triangle_mesh.Area(triangle_id);
    }
    BOOST_CHECK_LT(area, 5.141592654);
    BOOST_CHECK_GT(area, 5.135);
}

void TestElephantLarge( IntegrationMethodType IntegrationMethod, IndexType p, IndexType NumPointsInside, double Tolerance,
                        bool BSplineMesh, const BoundingBoxType& rBoundsUVW){

    std::string filename = "queso/tests/cpp_tests/data/elephant.stl";
    Parameters parameters( {Component("input_filename", filename),
                            Component("lower_bound_xyz", PointType(-0.37, -0.55, -0.31)),
                            Component("upper_bound_xyz", PointType(0.37, 0.55, 0.31)),
                            Component("b_spline_mesh", BSplineMesh),
                            Component("lower_bound_uvw", rBoundsUVW.first),
                            Component("upper_bound_uvw", rBoundsUVW.second),
                            Component("number_of_elements", Vector3i(14, 22, 12)),
                            Component("polynomial_order", Vector3i(p, p, p)),
                            Component("integration_method", IntegrationMethod) });
    QuESo queso(parameters);
    queso.Run();

    const auto& elements = queso.GetElements();

    const PointType delta_uvw = rBoundsUVW.second - rBoundsUVW.first;
    // Compute total weight
    double weigth_trimmed = 0.0;
    double weigth_inside = 0.0;
    const auto el_it_begin = elements.begin();
    int num_elements_inside = 0;
    int num_elements_trimmed = 0;
    int num_points_inside = 0;
    for( std::size_t i = 0; i < elements.size(); ++i){
        auto el_it = *(el_it_begin+i);
        const double det_j = el_it->DetJ();
        if( el_it->IsTrimmed() ){
            const auto& points_trimmed = el_it->GetIntegrationPoints();
            BOOST_CHECK_GT(points_trimmed.size(), 0);
            BOOST_CHECK_LT(points_trimmed.size(), (p+1)*(p+1)*(p+1)+1);
            for( auto point : points_trimmed ){
                weigth_trimmed += point.GetWeight()*det_j;
            }
            num_elements_trimmed++;
        } else {
            const auto& points_inside = el_it->GetIntegrationPoints();
            //BOOST_CHECK_GT(points_inside.size(), 0);
            for( auto point : points_inside ){
                weigth_inside += point.GetWeight()*det_j;
                num_points_inside++;
            }
            num_elements_inside++;
        }
    }
    BOOST_REQUIRE_EQUAL(num_elements_inside, 108);
    //BOOST_CHECK_EQUAL(num_elements_trimmed, 604);
    BOOST_CHECK_EQUAL(num_points_inside, NumPointsInside);

    // Check volume inside
    const double jacobian = 0.74*1.1*0.62;
    const double ref_volume_inside = jacobian/(14*22*12)*num_elements_inside;
    const double volume_inside = weigth_inside;
    const double rel_error_volume_inside = std::abs(volume_inside-ref_volume_inside)/ref_volume_inside;
    BOOST_CHECK_LT(rel_error_volume_inside, 1e-13);

    // Check volume inside + trimmed
    const double volume_tot = (weigth_trimmed+weigth_inside);
    const double ref_volume_tot = 0.0462012;
    const double rel_error_tot = std::abs(volume_tot-ref_volume_tot)/ref_volume_tot;
    BOOST_CHECK_LT(rel_error_tot, Tolerance);
}

void TestElephantSmall( IntegrationMethodType IntegrationMethod, IndexType p, IndexType NumPointsInside, double Tolerance,
                        bool BSplineMesh, const BoundingBoxType& rBoundsUVW){

    std::string filename = "queso/tests/cpp_tests/data/elephant.stl";
    Parameters parameters( {Component("input_filename", filename),
                            Component("lower_bound_xyz", PointType(-0.37, -0.55, -0.31)),
                            Component("upper_bound_xyz", PointType(0.37, 0.55, 0.31)),
                            Component("b_spline_mesh", BSplineMesh),
                            Component("lower_bound_uvw", rBoundsUVW.first),
                            Component("upper_bound_uvw", rBoundsUVW.second),
                            Component("number_of_elements", Vector3i(7, 11, 6)),
                            Component("polynomial_order", Vector3i(p, p, p)),
                            Component("integration_method", IntegrationMethod) });

    QuESo queso(parameters);
    queso.Run();

    const auto& elements = queso.GetElements();

    // Compute total weight
    double weigth_trimmed = 0.0;
    double weigth_inside = 0.0;
    const auto el_it_begin = elements.begin();
    int num_elements_inside = 0;
    int num_elements_trimmed = 0;
    int num_points_inside = 0;
    for( std::size_t i = 0; i < elements.size(); ++i){
        auto el_it = *(el_it_begin+i);
        const double det_j = el_it->DetJ();
        if( el_it->IsTrimmed() ){
            const auto& points_trimmed = el_it->GetIntegrationPoints();
            BOOST_CHECK_GT(points_trimmed.size(), 0);
            BOOST_CHECK_LT(points_trimmed.size(), (p+1)*(p+1)*(p+1)+1);
            for( auto point : points_trimmed ){
                weigth_trimmed += point.GetWeight()*det_j;
            }
            num_elements_trimmed++;
        } else {
            const auto& points_inside = el_it->GetIntegrationPoints();
            //BOOST_CHECK_GT(points_inside.size(), 0);
            for( auto point : points_inside ){
                weigth_inside += point.GetWeight()*det_j;
                num_points_inside++;
            }
            num_elements_inside++;
        }
    }
    BOOST_CHECK_EQUAL(num_elements_inside, 5);
    // BOOST_CHECK_EQUAL(num_elements_trimmed, 143);
    BOOST_CHECK_EQUAL(num_points_inside, NumPointsInside);

    // Check volume inside
    const double jacobian = 0.74*1.1*0.62;
    const double ref_volume_inside = jacobian/(7*11*6)*num_elements_inside;
    const double volume_inside = weigth_inside;
    const double rel_error_volume_inside = std::abs(volume_inside-ref_volume_inside)/ref_volume_inside;
    BOOST_CHECK_LT(rel_error_volume_inside, 1e-13);

    // Check volume inside + trimmed
    const double volume_tot = (weigth_trimmed+weigth_inside);
    const double ref_volume_tot = 0.0462012;
    const double rel_error_tot = std::abs(volume_tot-ref_volume_tot)/ref_volume_tot;
    BOOST_CHECK_LT(rel_error_tot, Tolerance);

}

// p=2
BOOST_AUTO_TEST_CASE(VolumeElephant1) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant Gauss (p=2)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantLarge(IntegrationMethod::Gauss, 2, 2916, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantLarge(IntegrationMethod::Gauss, 2, 2916, 0.0002, use_b_spline_mesh, MakeBox({-1.0, -5.5, -2.2}, {44.0, 1.12, 2.0}));
    TestElephantLarge(IntegrationMethod::Gauss, 2, 2916, 0.0002, false, MakeBox({-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}));
}

BOOST_AUTO_TEST_CASE(VolumeElephant2) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant Optimal (p=2)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantLarge(IntegrationMethod::GGQ_Optimal, 2, 1786, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantLarge(IntegrationMethod::GGQ_Optimal, 2, 1786, 0.0002, use_b_spline_mesh, MakeBox({-1.0, -5.5, -2.2}, {44.0, 1.12, 2.0}));
}

BOOST_AUTO_TEST_CASE(VolumeElephant3) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant GGQ_Reduced1 (p=2)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantLarge(IntegrationMethod::GGQ_Reduced1, 2, 673, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantLarge(IntegrationMethod::GGQ_Reduced1, 2, 673, 0.0002, use_b_spline_mesh, MakeBox({-6.0, -7.5, -1.2}, {22.0, 1.82, 2.8}));
}

BOOST_AUTO_TEST_CASE(VolumeElephant4) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant GGQ_Reduced2 (p=2)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantLarge(IntegrationMethod::GGQ_Reduced2, 2, 406, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantLarge(IntegrationMethod::GGQ_Reduced2, 2, 406, 0.0002, use_b_spline_mesh, MakeBox({-0.37, -0.55, -0.31}, {0.37, 0.55, 0.31}));
}

// p=3
BOOST_AUTO_TEST_CASE(VolumeElephant5) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant Gauss (p=3)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantSmall(IntegrationMethod::Gauss, 3, 320, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantSmall(IntegrationMethod::Gauss, 3, 320, 0.0002, use_b_spline_mesh, MakeBox({-0.37, -0.55, -0.31}, {0.37, 0.55, 0.31}));
    TestElephantSmall(IntegrationMethod::Gauss, 3, 320, 0.0002, false, MakeBox({-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}));
}

// p=3
BOOST_AUTO_TEST_CASE(VolumeElephant6) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant GGQ_Optimal (p=3)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantSmall(IntegrationMethod::GGQ_Optimal, 3, 256, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantSmall(IntegrationMethod::GGQ_Optimal, 3, 256, 0.0002, use_b_spline_mesh, MakeBox({-0.37, -0.55, -0.31}, {0.37, 0.55, 0.31}));
}

// p=4
BOOST_AUTO_TEST_CASE(VolumeElephant7) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant Gauss (p=4)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantSmall(IntegrationMethod::Gauss, 4, 625, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
    TestElephantSmall(IntegrationMethod::Gauss, 4, 625, 0.0002, use_b_spline_mesh, MakeBox({-0.37, -0.55, -0.31}, {0.37, 0.55, 0.31}));
    TestElephantSmall(IntegrationMethod::Gauss, 4, 625, 0.0002, false, MakeBox({-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}));
}

// p=4
BOOST_AUTO_TEST_CASE(VolumeElephant8) {
    QuESo_INFO << "Testing :: Test Embedding Operations :: Volume Elephant GGQ_Optimal (p=4)" << std::endl;
    const bool use_b_spline_mesh = true;
    TestElephantSmall(IntegrationMethod::GGQ_Optimal, 4, 525, 0.0002, use_b_spline_mesh, MakeBox({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}));
}

BOOST_AUTO_TEST_SUITE_END()

} // End namespace Testing
} // End namespace queso