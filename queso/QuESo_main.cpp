//   ____        ______  _____
//  / __ \      |  ____|/ ____|
// | |  | |_   _| |__  | (___   ___
// | |  | | | | |  __|  \___ \ / _ \'
// | |__| | |_| | |____ ____) | (_) |
//  \___\_\\__,_|______|_____/ \___/
//         Quadrature for Embedded Solids
//
//  License:    BSD 4-Clause License
//              See: https://github.com/manuelmessmer/QuESo/blob/main/LICENSE
//
//  Authors:    Manuel Messmer

//// STL includes
#include <fstream>
#include <thread>
#include <omp.h>

//// Project includes
#include "queso/QuESo_main.h"
#include "queso/io/io_utilities.h"
#include "queso/utilities/mesh_utilities.h"
#include "queso/embedding/brep_operator.h"
#include "queso/quadrature/single_element.hpp"
#include "queso/quadrature/trimmed_element.hpp"
#include "queso/quadrature/multiple_elements.hpp"
#include "queso/quadrature/integration_points_1d/integration_points_factory_1d.h"

namespace queso {

void QuESo::Run()
{
    Timer timer{};
    QuESo_INFO_IF(mParameters.EchoLevel() > 0) << "QuESo ------------------------------------------ START" << std::endl;

    double volume_brep = 0.0;
    if( mParameters.Get<bool>("embedding_flag") ) {
        // Compute volume
        volume_brep = MeshUtilities::VolumeOMP(*mpTriangleMesh);
        QuESo_INFO_IF(mParameters.EchoLevel() > 0) << "Volume of B-Rep model: " << volume_brep << '\n';

        // Write Surface Mesh to vtk file if eco_level > 0
        if( mParameters.EchoLevel() > 0){
            const std::string output_filename = mParameters.Get<std::string>("output_directory_name") + "/geometry.vtk";
            IO::WriteMeshToVTK(*mpTriangleMesh, output_filename.c_str(), true);
        }
    }

    // Construct BRepOperator
    mpBRepOperator = MakeUnique<BRepOperator>(*mpTriangleMesh);
    for( const auto& r_condition : mConditions ){
        mpBrepOperatorsBC.push_back( MakeUnique<BRepOperator>(r_condition.GetTriangleMesh() ) );
    }
    // Allocate element/knotspans container
    mpElementContainer = MakeUnique<ElementContainerType>(mParameters);

    // Start computation
    Compute();

    // Count number of trimmed elements
    SizeType number_of_trimmed_elements = 0;
    std::for_each(mpElementContainer->begin(), mpElementContainer->end(), [&number_of_trimmed_elements] (auto& el_it)
        { if( el_it.IsTrimmed() ) { number_of_trimmed_elements++; } });

    if( mParameters.EchoLevel() > 0) {
        // Write vtk files (binary = true)
        const std::string output_directory_name = mParameters.Get<std::string>("output_directory_name");
        IO::WriteElementsToVTK(*mpElementContainer, (output_directory_name + "/elements.vtk").c_str(), true);
        IO::WritePointsToVTK(*mpElementContainer, "All", (output_directory_name + "/integration_points.vtk").c_str(), true);
        IndexType cond_index = 0;
        for( const auto& r_condition : mConditions ){
            const std::string bc_filename = output_directory_name + '/' + r_condition.GetSettings().Get<std::string>("type")
                + '_' + std::to_string(++cond_index) + ".stl";
            IO::WriteMeshToSTL(r_condition.GetConformingMesh(), bc_filename.c_str(), true);
        }

        QuESo_INFO << "Number of active elements: " << mpElementContainer->size() << std::endl;
        QuESo_INFO << "Number of trimmed elements: " << number_of_trimmed_elements << std::endl;

        if( mParameters.EchoLevel() > 1 ) {
            const double volume_ips = mpElementContainer->GetVolumeOfAllIPs();
            QuESo_INFO << "The computed quadrature represents " << volume_ips/volume_brep * 100.0
                << "% of the volume of the BRep model.\n";
        }

        QuESo_INFO << "Elapsed time: " << timer.Measure() << std::endl;
        QuESo_INFO << "QuESo ------------------------------------------- END\n" << std::endl;
    }
}

void QuESo::Compute(){

    // Reserve element container
    const IndexType global_number_of_elements = mMapper.NumberOfElements();
    mpElementContainer->reserve(global_number_of_elements);

    // Time Variables
    double et_check_intersect = 0.0;
    double et_compute_intersection = 0.0;
    double et_moment_fitting = 0.0;

    // Get neccessary parameters
    const bool embedding_flag = mParameters.Get<bool>("embedding_flag");
    const bool ggq_rule_ise_used = mParameters.GGQRuleIsUsed();
    const double min_vol_element_ratio = mParameters.Get<double>("min_element_volume_ratio");
    const IndexType num_boundary_triangles = mParameters.MinimumNumberOfTriangles();
    const double moment_fitting_residual = mParameters.Get<double>("moment_fitting_residual");
    const Vector3i polynomial_order = mParameters.Get<Vector3i>("polynomial_order");
    const bool neglect_elements_if_mesh_is_flawed = mParameters.Get<bool>("neglect_elements_if_mesh_is_flawed");
    const IntegrationMethod integration_method = mParameters.IntegrationMethod();
    const IndexType echo_level = mParameters.EchoLevel();

    // Classify all elements.
    Unique<BRepOperator::StatusVectorType> p_classifications = nullptr;
    if( embedding_flag ){
        Timer timer_check_intersect{};
        p_classifications = mpBRepOperator->pGetElementClassifications(mParameters);
        et_check_intersect += timer_check_intersect.Measure();
    }

    #pragma omp parallel for reduction(+ : et_compute_intersection) reduction(+ : et_check_intersect) reduction(+ : et_moment_fitting) schedule(dynamic)
    for( int index = 0; index < static_cast<int>(global_number_of_elements); ++index) {
        // Check classification status
        IntersectionStatus status{};
        if( embedding_flag ){
            status = (*p_classifications)[index];
        }
        else { // If flag is false, consider all knotspans/ elements as inside
            status = IntersectionStatus::Inside;
        }

        if( status == IntersectionStatus::Inside || status == IntersectionStatus::Trimmed ) {
            // Get bounding box of element
            const auto bounding_box_xyz = mMapper.GetBoundingBoxXYZFromIndex(index);
            const auto bounding_box_uvw = mMapper.GetBoundingBoxUVWFromIndex(index);

            // Construct element and check status:
            Unique<ElementType> new_element = MakeUnique<ElementType>(index+1, bounding_box_xyz, bounding_box_uvw);
            bool valid_element = false;

            // Distinguish between trimmed and non-trimmed elements.
            if( status == IntersectionStatus::Trimmed) {
                new_element->SetIsTrimmed(true);
                Timer timer_compute_intersection{};
                auto p_trimmed_domain = mpBRepOperator->pGetTrimmedDomain(bounding_box_xyz.first, bounding_box_xyz.second,
                                                                          min_vol_element_ratio, num_boundary_triangles, neglect_elements_if_mesh_is_flawed);
                if( p_trimmed_domain ){
                    new_element->pSetTrimmedDomain(p_trimmed_domain);
                    valid_element = true;
                }
                et_compute_intersection += timer_compute_intersection.Measure();

                // If valid solve moment fitting equation
                if( valid_element ){
                    Timer timer_moment_fitting{};
                    QuadratureTrimmedElement<ElementType>::AssembleIPs(*new_element, polynomial_order, moment_fitting_residual, echo_level);
                    et_moment_fitting += timer_moment_fitting.Measure();

                    if( new_element->GetIntegrationPoints().size() == 0 ){
                        valid_element = false;
                    }
                }
            }
            else if( status == IntersectionStatus::Inside){
                // Get standard gauss legendre points
                if( !ggq_rule_ise_used ){
                    QuadratureSingleElement<ElementType>::AssembleIPs(*new_element, polynomial_order, integration_method);
                }
                valid_element = true;
            }

            if( valid_element ){
                #pragma omp critical // TODO: improve this.
                mpElementContainer->AddElement(new_element); // After this new_element is a null_ptr. Is std::moved to container.
            }
        }

    }

    // Treat conditions
    #pragma omp parallel for
    for( int index = 0; index < static_cast<int>(global_number_of_elements); ++index ) {
        for( IndexType i = 0; i < mConditions.size(); ++i ){
            const auto bounding_box_xyz = mMapper.GetBoundingBoxXYZFromIndex(index);
            const auto p_new_mesh = mpBrepOperatorsBC[i]->pClipTriangleMeshUnique(bounding_box_xyz.first, bounding_box_xyz.second);
            if( p_new_mesh->NumOfTriangles() > 0 ){
                #pragma omp critical
                mConditions[i].AddToConformingMesh(*p_new_mesh);
            }
        }
    }

    if( mParameters.GGQRuleIsUsed() ){

        const Vector3i number_of_elements = mParameters.Get<Vector3i>("number_of_elements");
        const Vector3i polynomial_order = mParameters.Get<Vector3i>("polynomial_order");
        const IntegrationMethodType integration_method = mParameters.IntegrationMethod();
        QuadratureMultipleElements<ElementType>::AssembleIPs(*mpElementContainer, number_of_elements, polynomial_order, integration_method);
    }

    // Average time spent for each task
    if( mParameters.EchoLevel() > 1 ){
        const IndexType num_procs = std::thread::hardware_concurrency();
        QuESo_INFO << "Elapsed times of individual tasks -------------- \n";
        QuESo_INFO << "Detection of trimmed elements: --- " << et_check_intersect / ((double) num_procs) << '\n';
        QuESo_INFO << "Compute intersection: ------------ " << et_compute_intersection / ((double) num_procs) << "\n";
        QuESo_INFO << "Moment fitting: ------------------ " << et_moment_fitting / ((double) num_procs) << "\n";
        QuESo_INFO << "------------------------------------------------ \n";
    }

}

Condition& QuESo::CreateNewCondition(const ConditionParameters& rConditionParameters){
    Unique<TriangleMeshInterface> p_new_mesh = MakeUnique<TriangleMesh>();
    if( rConditionParameters.Get<std::string>("input_type") == "stl_file" ) {
        const std::string& r_filename = rConditionParameters.Get<std::string>("input_filename");
        IO::ReadMeshFromSTL(*p_new_mesh, r_filename.c_str());
    }

    // Condition owns triangle mesh.
    mConditions.push_back( Condition(p_new_mesh, rConditionParameters) );
    return mConditions.back();
}

void QuESo::Check() const {
    // Check if bounding box fully contains the triangle mesh.
    if( mParameters.EchoLevel() > 0 ){
        PointType lower_bound = mParameters.LowerBoundXYZ();
        PointType upper_bound = mParameters.UpperBoundXYZ();
        auto bb_mesh = MeshUtilities::BoundingBox(*mpTriangleMesh);
        if( lower_bound[0] > bb_mesh.first[0]  ||
            lower_bound[1] > bb_mesh.first[1]  ||
            lower_bound[2] > bb_mesh.first[2]  ||
            upper_bound[0] < bb_mesh.second[0] ||
            upper_bound[1] < bb_mesh.second[1] ||
            upper_bound[2] < bb_mesh.second[2] ) {
                QuESo_INFO << "Warning :: The given bounding box: 'lower_bound_xyz' : " << lower_bound
                    << ", 'upper_bound_xyz:' " << upper_bound << " does not fully contain the bounding box of "
                    << "the input STL: 'lower_bound_xyz' : " << bb_mesh.first << ", 'upper_bound_xyz:' "
                    << bb_mesh.second << '\n';
        }
    }

}

} // End namespace queso
