// Author: Manuel Meßmer
// Email: manuel.messmer@tum.de

//// STL includes
#include <stdexcept>
#include <cmath>
//// Project includes
#include "define.hpp"
#include "quadrature/moment_fitting_utilities.h"
#include "utilities/mapping_utilities.h"
#include "utilities/polynomial_utilities.h"
#include "containers/element.hpp"
#include "solvers/nnls.h"
#include "io/io_utilities.h"

namespace tibra {

typedef boost::numeric::ublas::matrix<double> MatrixType;
typedef boost::numeric::ublas::vector<double> VectorType;

void MomentFitting::DistributeIntegrationPoints(Octree<TrimmedDomainBase>& rOctree, IntegrationPointVectorType& rIntegrationPoint, SizeType MinNumPoints, Element& rElement, const Parameters& rParam){
    IndexType refinemen_level = std::max<IndexType>(rOctree.MaxRefinementLevel(), 1UL);
    while( rIntegrationPoint.size() < MinNumPoints ){
        rOctree.Refine(std::min<IndexType>(refinemen_level, 4UL), refinemen_level);
        rIntegrationPoint.clear();
        rOctree.AddIntegrationPoints(rIntegrationPoint, {rParam.Order()[0]+1,rParam.Order()[1]+1,rParam.Order()[2]+1});
        refinemen_level++;
    }
}

void MomentFitting::ComputeConstantTerms(const Element& rElement, const BoundaryIPsVectorPtrType& pBoundaryIps,
                                         VectorType& rConstantTerms, const Parameters& rParam){
    // Initialize const variables.
    const auto lower_bound = rParam.LowerBound();
    const auto upper_bound = rParam.UpperBound();

    const double jacobian_x = std::abs(lower_bound[0] - upper_bound[0]);
    const double jacobian_y = std::abs(lower_bound[1] - upper_bound[1]);
    const double jacobian_z = std::abs(lower_bound[2] - upper_bound[2]);

    const PointType& a = rElement.GetLowerBoundParam();
    const PointType& b = rElement.GetUpperBoundParam();

    const int ffactor = 1;
    const int order_u = rParam.Order()[0];
    const int order_v = rParam.Order()[1];
    const int order_w = rParam.Order()[2];

    const IndexType number_of_functions = (order_u*ffactor + 1) * (order_v*ffactor+1) * (order_w*ffactor + 1);

    // Resize constant terms.
    rConstantTerms.resize(number_of_functions, false);
    std::fill( rConstantTerms.begin(),rConstantTerms.end(), 0.0);

    /// Initialize containers for f_x and f_x_int
    // X-direction
    std::vector<double> f_x_x(order_u*ffactor+1);
    std::vector<double> f_x_int_x(order_u*ffactor+1);
    // Y-direction
    std::vector<double> f_x_y(order_v*ffactor+1);
    std::vector<double> f_x_int_y(order_v*ffactor+1);
    // Z-direction
    std::vector<double> f_x_z(order_w*ffactor+1);
    std::vector<double> f_x_int_z(order_w*ffactor+1);

    // Loop over all points.
    IndexType row_index = 0;
    const auto begin_points_it_ptr = pBoundaryIps->begin();
    for( int i = 0; i < pBoundaryIps->size(); ++i ){
        // Note: The evaluation of polynomials is expensive. Therefore, precompute and store values
        // for f_x_x and f_x_int at each point.
        auto point_it = (begin_points_it_ptr + i);
        const auto& normal = point_it->Normal();
        PointType local_point = Mapping::GlobalToParam(*point_it, lower_bound, upper_bound);

        // X-Direction
        for( IndexType i_x = 0; i_x <= order_u*ffactor; ++i_x){
            f_x_x[i_x] = Polynomial::f_x(local_point[0], i_x, a[0], b[0]);
            f_x_int_x[i_x] = Polynomial::f_x_int(local_point[0], i_x, a[0], b[0]);
        }
        // Y-Direction
        for( IndexType i_y = 0; i_y <= order_v*ffactor; ++i_y){
            f_x_y[i_y] = Polynomial::f_x(local_point[1], i_y, a[1], b[1]);
            f_x_int_y[i_y] = Polynomial::f_x_int(local_point[1], i_y, a[1], b[1]);
        }
        // Z-Direction
        for( IndexType i_z = 0; i_z <= order_w*ffactor; ++i_z){
            f_x_z[i_z] = Polynomial::f_x(local_point[2], i_z, a[2], b[2]);
            f_x_int_z[i_z] = Polynomial::f_x_int(local_point[2], i_z, a[2], b[2]);
        }

        // Assembly RHS
        row_index = 0;
        const double weight = 1.0/3.0*point_it->GetWeight();
        for( int i_x = 0; i_x <= order_u*ffactor; ++i_x){
            for( int i_y = 0; i_y <= order_v*ffactor; ++i_y ){
                for( int i_z = 0; i_z <= order_w*ffactor; ++i_z){
                    // Compute normal for each face/triangle.
                    PointType value;
                    value[0] = f_x_int_x[i_x]*f_x_y[i_y]*f_x_z[i_z];
                    value[1] = f_x_x[i_x]*f_x_int_y[i_y]*f_x_z[i_z];
                    value[2] = f_x_x[i_x]*f_x_y[i_y]*f_x_int_z[i_z];

                    double integrand = normal[0]*value[0]*jacobian_x + normal[1]*value[1]*jacobian_y + normal[2]*value[2]*jacobian_z;
                    rConstantTerms[row_index] += integrand * weight;
                    row_index++;
                }
            }
        }
    }
}


void MomentFitting::CreateIntegrationPointsTrimmed(Element& rElement, const Parameters& rParam){

    // Get boundary integration points.
    const auto p_trimmed_domain = rElement.pGetTrimmedDomain();
    const auto p_boundary_ips = p_trimmed_domain->pGetBoundaryIps();

    // Get constant terms.
    VectorType constant_terms{};
    ComputeConstantTerms(rElement, p_boundary_ips, constant_terms, rParam);

    // Construct octree. Octree is used to distribute inital points within trimmed domain.
    const auto bounding_box = p_trimmed_domain->GetBoundingBoxOfTrimmedDomain();
    Octree octree(p_trimmed_domain, bounding_box.first, bounding_box.second, rParam);

    // Start point elimination.
    double residual = MAXD;
    SizeType iteration = 0UL;
    SizeType point_distribution_factor = rParam.GetPointDistributionFactor();
    IntegrationPointVectorType integration_points{};

    // If residual can not be statisfied, try with more points in initial set.
    while( residual > rParam.MomentFittingResidual() && iteration < 4UL){

        // Distribute intial points via an octree.
        const SizeType min_num_points = (rParam.Order()[0]+1)*(rParam.Order()[1]+1)*(rParam.Order()[2]+1)*(point_distribution_factor);
        DistributeIntegrationPoints(octree, integration_points, min_num_points, rElement, rParam);

        // Also add old, moment fitted points to new set. 'old_integration_points' only contains points with weights > 0.0;
        auto& old_integration_points = rElement.GetIntegrationPoints();
        integration_points.insert(integration_points.end(), old_integration_points.begin(), old_integration_points.end() );
        old_integration_points.clear();

        // Run point elimination.
        residual = PointElimination(constant_terms, integration_points, rElement, rParam);

        // If residual is very high, remove all points. Note, elements without points will be neglected.
        if( residual > 1e-2 ) {
            auto& reduced_points = rElement.GetIntegrationPoints();
            reduced_points.clear();
        }

        // Update variables.
        point_distribution_factor *= 2;
        iteration++;
    }

    if( residual > rParam.MomentFittingResidual() && rParam.EchoLevel() > 2){
        TIBRA_INFO << "Moment Fitting :: Targeted residual can not be achieved: " << residual << std::endl;
    }
}

double MomentFitting::MomentFitting1(const VectorType& rConstantTerms, IntegrationPointVectorType& rIntegrationPoint, const Element& rElement, const Parameters& rParam){

    const double jacobian_x = std::abs(rParam.UpperBound()[0] - rParam.LowerBound()[0]);
    const double jacobian_y = std::abs(rParam.UpperBound()[1] - rParam.LowerBound()[1]);
    const double jacobian_z = std::abs(rParam.UpperBound()[2] - rParam.LowerBound()[2]);

    PointType a = rElement.GetLowerBoundParam();
    PointType b = rElement.GetUpperBoundParam();

    const int ffactor = 1;
    const int order_u = rParam.Order()[0];
    const int order_v = rParam.Order()[1];
    const int order_w = rParam.Order()[2];

    const IndexType number_of_functions = (order_u*ffactor + 1) * (order_v*ffactor+1) * (order_w*ffactor + 1);
    const IndexType number_reduced_points = rIntegrationPoint.size();

    /// Assemble moment fitting matrix.
    MatrixType fitting_matrix(number_of_functions, number_reduced_points);
    IndexType row_index = 0;
    for( int i_x = 0; i_x <= order_u*ffactor; ++i_x){
        for( int i_y = 0; i_y <= order_v*ffactor; ++i_y ){
            for( int i_z = 0; i_z <= order_w*ffactor; ++i_z){
                // Loop over all points
                const auto points_it_begin = rIntegrationPoint.begin();
                for( int column_index = 0; column_index < number_reduced_points; ++column_index ){
                    auto point_it = points_it_begin + column_index;

                    const double value = Polynomial::f_x(point_it->X(), i_x, a[0], b[0])
                                       * Polynomial::f_x(point_it->Y(), i_y, a[1], b[1])
                                       * Polynomial::f_x(point_it->Z(), i_z, a[2], b[2]);

                    fitting_matrix(row_index,column_index) = value;
                }
                row_index++;
            }
        }
    }

    VectorType weights(number_reduced_points);
    // Solve non-negative Least-Square-Error problem.
    auto residual = nnls::nnls(fitting_matrix, rConstantTerms, weights)/number_of_functions;

    // Write computed weights onto integration points
    for( int i = 0; i < number_reduced_points; ++i){
        // Divide by det_jacobian to account for the corresponding multiplication during the element integration within the used external solver.
        double new_weight = weights[i]/(jacobian_x*jacobian_y*jacobian_z);
        rIntegrationPoint[i].SetWeight(new_weight);
    }

    return residual;
}


double MomentFitting::PointElimination(const VectorType& rConstantTerms, IntegrationPointVectorType& rIntegrationPoint, Element& rElement, const Parameters& rParam) {

    /// Initialize const variables
    const SizeType ffactor = 1;
    const SizeType order_u = rParam.Order()[0];
    const SizeType order_v = rParam.Order()[1];
    const SizeType order_w = rParam.Order()[2];
    const IndexType number_of_functions = (order_u*ffactor + 1) * (order_v*ffactor+1) * (order_w*ffactor + 1);

    /// Enter point elimination algorithm
    const double allowed_residual = rParam.MomentFittingResidual();
    double global_residual = MIND;
    double prev_residual = 0.0;
    const SizeType maximum_iteration = 1000;
    SizeType number_iterations = 0;
    bool point_removed = false;
    IntegrationPointVectorType prev_solution{};
    while( point_removed || (global_residual < allowed_residual && number_iterations < maximum_iteration) ){
        point_removed = false;
        global_residual = MomentFitting1(rConstantTerms, rIntegrationPoint, rElement, rParam);
        if( number_iterations == 0){
            // Sort integration points according to weight
            std::sort(rIntegrationPoint.begin(), rIntegrationPoint.end(), [](const IntegrationPoint& point_a, const IntegrationPoint& point_b) -> bool {
                    return point_a.GetWeight() > point_b.GetWeight();
                });
            // Only keep number_of_functions integration points.
            rIntegrationPoint.erase(rIntegrationPoint.begin()+number_of_functions, rIntegrationPoint.end());
            point_removed = true;
        }
        else if( global_residual < allowed_residual ){
            prev_solution.clear();
            prev_solution.insert(prev_solution.begin(), rIntegrationPoint.begin(), rIntegrationPoint.end());
            prev_residual = global_residual;
            auto min_value_it = rIntegrationPoint.begin();
            double min_value = 1e10;
            double max_value = -1e10;
            auto begin_it = rIntegrationPoint.begin();
            for(int i = 0; i < rIntegrationPoint.size(); i++){
                auto it = begin_it + i;
                if( it->GetWeight() < min_value ) {
                    min_value_it = it;
                    min_value = it->GetWeight();
                }
                if( it->GetWeight() > max_value ) {
                    max_value = it->GetWeight();
                }
            }
            begin_it = rIntegrationPoint.begin();
            int counter = 0;
            for(int i = 0; i < rIntegrationPoint.size(); i++){
                auto it = begin_it + i;
                // TODO: Fix this > 2..4
                if( it->GetWeight() < EPS1*max_value && rIntegrationPoint.size() > 4){
                    rIntegrationPoint.erase(it);
                    point_removed = true;
                    counter++;
                }
            }
            if( counter == 0 && rIntegrationPoint.size() > 4){
                rIntegrationPoint.erase(min_value_it);
                point_removed = true;
            }
            if( rIntegrationPoint.size() <= 4 && !point_removed ){
                number_iterations = maximum_iteration + 10;
            }
        }
        number_iterations++;
    }

    auto& reduced_points = rElement.GetIntegrationPoints();
    if( (global_residual >= allowed_residual && prev_solution.size() > 0 && number_iterations < maximum_iteration) ) {
        reduced_points.insert(reduced_points.begin(), prev_solution.begin(), prev_solution.end());
        reduced_points.erase(std::remove_if(reduced_points.begin(), reduced_points.end(), [](const IntegrationPoint& point) {
            return point.GetWeight() < EPS4; }), reduced_points.end());
        return prev_residual;
    }
    else{
        reduced_points.insert(reduced_points.begin(), rIntegrationPoint.begin(), rIntegrationPoint.end());
        reduced_points.erase(std::remove_if(reduced_points.begin(), reduced_points.end(), [](const IntegrationPoint& point) {
            return point.GetWeight() < EPS4; }), reduced_points.end());
        return global_residual;
    }

}

} // End namespace tibra

// double MomentFitting::f_x(double x, int order, double a, double b){
//     double tmp_x = (2*x - a - b)/(b-a);
//     return p_n(tmp_x,order);
// }



// double MomentFitting::f_x(double x, int order){
//     if( order == 0){double
//     }
//     else {
//         return std::pow(x, order);
//     }
// }

// double MomentFitting::f_x_integral(double x, int order) {
//     if(  order == 0 ){
//         return x;
//     }
//     else {
//         return 1.0/ ( (double)order + 1.0) * std::pow(x, order+1);
//     }
// }

// double MomentFitting::f_x_integral(double x, int order, double a, double b){
//     switch(order)
//     {
//         case 0:
//             return x;
//         case 1:
//             return -std::pow((a + b - 2.0*x),2)/(4.0*(a - b));
//         case 2:
//             return - x/2.0 - std::pow((a + b - 2.0*x),3)/(4.0*std::pow((a - b),2));
//         case 3:
//             return (3.0*std::pow( (a + b - 2.0*x),2) )/(8.0*(a - b)) - (5*std::pow((a + b - 2.0*x),4))/(16*std::pow((a - b),3));
//         case 4:
//             return (3.0*x)/8.0 + (5.0*std::pow((a + b - 2.0*x),3))/(8*std::pow((a - b),2)) - (7.0*std::pow((a + b - 2*x),5))/(16*std::pow((a - b),4));
//         case 5:
//             return (35*std::pow((a + b - 2*x),4))/(32*std::pow((a - b),3)) - (15*std::pow((a + b - 2*x),2))/(32*(a - b)) - (21*std::pow((a + b - 2*x),6))/(32*std::pow((a - b),5));
//         case 6:
//             return (63*std::pow((a + b - 2*x),5))/(32*std::pow((a - b),4)) - (35*std::pow((a + b - 2*x),3))/(32*std::pow((a - b),2)) - (5*x)/16 - (33*std::pow((a + b - 2*x),7))/(32*std::pow((a - b),6));
//         case 7:
//             return (35.0*std::pow( (a + b - 2*x),2))/(64*(a - b)) - (315*std::pow((a + b - 2*x),4))/(128.0*std::pow((a - b),3)) + (231.0*std::pow((a + b - 2*x),6))/(64.0*std::pow((a - b),5)) - (429.0*std::pow((a + b - 2*x),8))/(256.0*std::pow((a - b),7));
//         case 8:
//             return (35.0*x)/128.0 + (105.0*std::pow( (a + b - 2*x),3) )/(64.0*std::pow( (a - b),2) ) - (693.0*std::pow( (a + b - 2*x),5) )/(128.0*std::pow((a - b),4) ) + (429.0*std::pow((a + b - 2*x),7))/(64.0*std::pow((a - b),6)) - (715.0*std::pow( (a + b - 2*x),9) )/(256.0*std::pow((a - b),8));
//     }

//     throw  std::invalid_argument("MomentFitting :: Order out of range!\n");
// }

// double MomentFitting::p_n(double x, int order) {
//     switch(order)
//     {
//         case 0:
//             return 1;
//         case 1:
//             return x;
//         case 2:
//             return 1.0/2.0*(3.0*std::pow(x,2)-1.0);
//         case 3:
//             return 1.0/2.0*(5.0*std::pow(x,3) - 3.0*x);
//         case 4:
//             return 1.0/8.0*(35.0*std::pow(x,4)-30.0*std::pow(x,2) +3.0);
//         case 5:
//             return 1.0/8.0*(63.0*std::pow(x,5)-70.0*std::pow(x,3)+15.0*x);
//         case 6:
//             return 1.0/16.0*(231.0*std::pow(x,6)-315.0*std::pow(x,4)+105.0*std::pow(x,2)-5.0);
//         case 7:
//             return 1.0/16.0*(429.0*std::pow(x,7)-693.0*std::pow(x,5)+315.0*std::pow(x,3)-35.0*x);
//         case 8:
//             return 1.0/128.0*(6435.0*std::pow(x,8) - 12012.0*std::pow(x,6)+6930.0*std::pow(x,4)-1260.0*std::pow(x,2)+35.0);
//     }

//     throw  std::invalid_argument("MomentFitting :: Order out of range!\n");
// }


