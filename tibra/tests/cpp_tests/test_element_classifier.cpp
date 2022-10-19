// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <chrono>

#include "geometries/triangle_mesh.h"
#include "io/io_utilities.h"
#include "embedding/geometrical_entity_classifier.h"

namespace Testing{

BOOST_AUTO_TEST_SUITE( ElementClassifierTestSuite )


BOOST_AUTO_TEST_CASE(CylinderElementClassifierTest) {
    std::cout << "Testing :: Test Classify Elements :: Cylinder" << std::endl;

    // Read mesh from STL file
    TriangleMesh triangle_mesh{};
    IO::ReadMeshFromSTL(triangle_mesh, "tibra/tests/cpp_tests/data/cylinder.stl");

    GeometricalEntityClassifier classifier(triangle_mesh);

    const double delta_x = 0.1;
    const double delta_y = 0.1;
    const double delta_z = 0.1;

    std::vector<IndexType> result{};
    double tolerance = 0.0;
    result.reserve(117900);
    for(double x = -1.5; x <= 1.5; x += delta_x){
        for(double y = -1.5; y <= 1.5; y += delta_y){
            for(double z = -1; z <= 12; z += delta_z){

                TriangleMesh::Vector3d lower_bound = {x, y, z};
                TriangleMesh::Vector3d upper_bound = {x+delta_x, y+delta_y, z+delta_z};
                result.push_back( classifier.GetIntersectionState(lower_bound, upper_bound, tolerance) );
            }
        }
    }

    BOOST_CHECK_EQUAL( result.size(), 117900);
    std::ifstream myfile("tibra/tests/cpp_tests/results/element_classifier_cylinder.txt");
    std::string line;
    for( int i = 0; i < result.size(); ++i){
        getline (myfile, line);
        BOOST_CHECK_EQUAL( result[i], std::stoi(line) );
    }
    myfile.close();
}

BOOST_AUTO_TEST_CASE(CubeElementClassifierTest) {
    std::cout << "Testing :: Test Classify Elements :: Cube with cavity" << std::endl;

    // Read mesh from STL file
    TriangleMesh triangle_mesh{};
    IO::ReadMeshFromSTL(triangle_mesh, "tibra/tests/cpp_tests/data/cube_with_cavity.stl");

    // Instatiate classifier
    GeometricalEntityClassifier classifier(triangle_mesh);

    const double delta_x = 0.15;
    const double delta_y = 0.15;
    const double delta_z = 0.15;

    std::vector<IndexType> result{};
    double tolerance = 0.0;
    result.reserve(9261);
    for(double x = -1.5001; x <= 1.5; x += delta_x){
        for(double y = -1.5001; y <= 1.5; y += delta_y){
            for(double z = -1.5001; z <= 1.5; z += delta_z){

                TriangleMesh::Vector3d lower_bound = {x, y, z};
                TriangleMesh::Vector3d upper_bound = {x+delta_x, y+delta_y, z+delta_z};
                result.push_back( classifier.GetIntersectionState(lower_bound, upper_bound, tolerance) );
            }
        }
    }

    BOOST_CHECK_EQUAL( result.size(), 9261);
    std::ifstream myfile("tibra/tests/cpp_tests/results/element_classifier_cube.txt");
    std::string line;
    for( int i = 0; i < result.size(); ++i){
        getline (myfile, line);
        BOOST_CHECK_EQUAL( result[i], std::stoi(line) );
        //myfile << result[i] << "\n";
    }
    myfile.close();
}

BOOST_AUTO_TEST_CASE(ElephantElementClassifierTest) {
    std::cout << "Testing :: Test Classify Elements :: Elephant" << std::endl;

    // Read mesh from STL file
    TriangleMesh triangle_mesh{};
    IO::ReadMeshFromSTL(triangle_mesh, "tibra/tests/cpp_tests/data/elephant.stl");

    GeometricalEntityClassifier classifier(triangle_mesh);

    const double delta_x = 0.05;
    const double delta_y = 0.05;
    const double delta_z = 0.05;

    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<IndexType> result{};
    double tolerance = 0.0;
    result.reserve(6375);
    for(double x = -0.4; x <= 0.4; x += delta_x){
        for(double y = -0.6; y <= 0.6; y += delta_y){
            for(double z = -0.35; z <= 0.35; z += delta_x){

                TriangleMesh::Vector3d lower_bound = {x, y, z};
                TriangleMesh::Vector3d upper_bound = {x+delta_x, y+delta_y, z+delta_z};
                result.push_back( classifier.GetIntersectionState(lower_bound, upper_bound, tolerance) );
            }
        }
    }

    BOOST_CHECK_EQUAL( result.size(), 6375);
    std::ifstream myfile("tibra/tests/cpp_tests/results/element_classifier_elephant.txt");
    std::string line;
    for( int i = 0; i < result.size(); ++i){
        getline (myfile, line);
        BOOST_CHECK_EQUAL( result[i], std::stoi(line) );
    }
    myfile.close();
}

BOOST_AUTO_TEST_CASE(BunnyElementClassifierTest) {
    std::cout << "Testing :: Test Classify Elements :: Bunny" << std::endl;

    // Read mesh from STL file
    TriangleMesh triangle_mesh{};
    IO::ReadMeshFromSTL(triangle_mesh, "tibra/tests/cpp_tests/data/stanford_bunny.stl");

    GeometricalEntityClassifier classifier(triangle_mesh);

    const double delta_x = 3;
    const double delta_y = 3;
    const double delta_z = 3;

    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<IndexType> result{};
    double tolerance = 0.0;
    result.reserve(41070);
    for(double x = -24; x <= 85; x += delta_x){
        for(double y = -43; y <= 46; y += delta_y){
            for(double z = 5; z <= 115; z += delta_z){

                TriangleMesh::Vector3d lower_bound = {x, y, z};
                TriangleMesh::Vector3d upper_bound = {x+delta_x, y+delta_y, z+delta_z};
                result.push_back( classifier.GetIntersectionState(lower_bound, upper_bound, tolerance) );
            }
        }
    }

    BOOST_CHECK_EQUAL( result.size(), 41070);
    std::ifstream myfile("tibra/tests/cpp_tests/results/element_classifier_bunny.txt");
    std::string line;
    for( int i = 0; i < result.size(); ++i){
        getline (myfile, line);
        BOOST_CHECK_EQUAL( result[i], std::stoi(line) );
    }
    myfile.close();
}

BOOST_AUTO_TEST_SUITE_END()

} // End namespace Testing