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

#ifndef ELEMENT_INCLUDE_H
#define ELEMENT_INCLUDE_H

//// STL includes
#include <stdexcept>
#include <memory>
//// Project includes
#include "queso/embedding/trimmed_domain.h"
#include "queso/utilities/mapping_utilities.h"

namespace queso {

///@name QuESo Classes
///@{

/**
 * @class  Element
 * @author Manuel Messmer
 * @brief  Element/Knot Spans. Is is defined by a simpe bounding box in physical and in parametric space.
 *         Stores quadrature points and trimmed domain (if element is trimmed).
*/
template<typename TIntegrationPointType, typename TBoundaryIntegrationPointType>
class Element
{
public:

    ///@name Type Defintitions
    ///@{
    typedef std::size_t IndexType;
    typedef std::size_t SizeType;
    typedef TIntegrationPointType IntegrationPointType;
    typedef TBoundaryIntegrationPointType BoundaryIntegrationPointType;
    typedef std::vector<IntegrationPointType> IntegrationPointVectorType;
    typedef std::vector<BoundaryIntegrationPointType> BoundaryIntegrationPointVectorType;

    typedef std::vector<std::array<double, 2>> IntegrationPoint1DVectorType;
    typedef Unique<TrimmedDomain> TrimmedDomainPtrType;

    ///@}
    ///@name Life Cycle
    ///@{

    /// Constructor
    ///@param ElementId UniqueID
    ///@param rBoundXYZ Bounds of Element in physical space.
    ///@param rBoundUVW Bounds of Element in parametric space.
    Element(IndexType ElementId, const BoundingBoxType& rBoundXYZ, const BoundingBoxType& rBoundUVW) :
                mElementId(ElementId), mIsTrimmed(false), mIsVisited(false), mBoundsXYZ(rBoundXYZ),
                mBoundsUVW(rBoundUVW), mpTrimmedDomain(nullptr)
    {
    }

    // Destructor
    ~Element() = default;

    // Delete copy constructor
    Element(const Element& rOther) = delete;

    // Assignement operator
    Element& operator=(const Element& rOther) = delete;

    ///@}
    ///@name Operations
    ///@{

    /// @brief Set Element as trimmed.
    /// @param Value
    void SetIsTrimmed(bool Value){
        mIsTrimmed = Value;
    }

    /// @brief Set Id
    /// @param Value
    void SetId(IndexType Value){
        mElementId = Value;
    }

    /// @brief Return Id of this element.
    /// @return IndexType
    const IndexType GetId() const {
        return mElementId;
    }

    /// @brief Returns true if element is trimmed.
    /// @return bool
    bool IsTrimmed() const {
        return mIsTrimmed;
    }

    /// @brief Returns Vector of integration points. (non-const)
    /// @return IntegrationPointVectorType&
    IntegrationPointVectorType& GetIntegrationPoints() {
        return mIntegrationPoints;
    }

    /// @brief Returns const& Vector of integration points. (const)
    /// @return const IntegrationPointVectorType&
    const IntegrationPointVectorType& GetIntegrationPoints() const{
        return mIntegrationPoints;
    }


    /// @brief Get bounds of element in physical/global coordinates.
    /// @return BoundingBoxType
    const BoundingBoxType& GetBoundsXYZ() const {
        return mBoundsXYZ;
    }

    /// @brief Get bounds of element in parametric coordinates.
    /// @return BoundingBoxType
    const BoundingBoxType& GetBoundsUVW() const{
        return mBoundsUVW;
    }

    /// @brief Map point from global space to parametric space.
    /// @param rGlobalCoord
    /// @return PointType
    PointType PointFromGlobalToParam( const PointType& rGlobalCoord ) const {
        return VoxelIndexing::PointFromGlobalToParam(rGlobalCoord, mBoundsXYZ, mBoundsUVW);
    }

    /// @brief Map point from parametric space to global space.
    /// @param rLocalCoord
    /// @return PointType
    PointType PointFromParamToGlobal( const PointType& rLocalCoord ) const {
        return VoxelIndexing::PointFromParamToGlobal(rLocalCoord, mBoundsXYZ, mBoundsUVW);
    }

    /// @brief Returns determinant of jacobian.
    /// @return double.
    double DetJ() const {
        const auto detla_xyz = Math::Subtract( mBoundsXYZ.second, mBoundsXYZ.first );
        const auto detla_uvw = Math::Subtract( mBoundsUVW.second, mBoundsUVW.first );
        return (detla_xyz[0]*detla_xyz[1]*detla_xyz[2]) / (detla_uvw[0]*detla_uvw[1]*detla_uvw[2]);
    }

    /// @brief Returns 1D integration points. Required for assembly of GGQ rules.
    /// @param Dir Space Direction: 0-x, 1-y, 2-z.
    /// @return IntegrationPoint1DVectorType&
    IntegrationPoint1DVectorType& IntegrationPoints1D(IndexType Dir){
        if(Dir==0)
            return mIntegrationPointsX;
        if(Dir==1)
            return mIntegrationPointsY;

        return mIntegrationPointsZ;
    }

    /// @brief Set trimmed domain of element.
    /// @param pTrimmedDomain Ptr (Unique) to new trimmed domain.
    void pSetTrimmedDomain(TrimmedDomainPtrType& pTrimmedDomain ){
        mpTrimmedDomain = std::move(pTrimmedDomain);
    }

    /// @brief Get ptr to trimmed domain of element.
    /// @note Return raw ptr. No transfer of ownership. Element owns trimmed domain.
    /// @return const TrimmedDomain*
    const TrimmedDomain* const pGetTrimmedDomain() const {
        if( !IsTrimmed() ){
            QuESo_ERROR << "Element is not Trimmed.\n";
        }
        if( !mpTrimmedDomain ){
            QuESo_ERROR << "Trimmed Domain Pointer has not been set.\n";
        }
        return mpTrimmedDomain.get();
    }

    /// @brief Clear trimmed domain of element.
    void ClearTrimmedDomain(){
        mpTrimmedDomain = nullptr;
    }

    /// @brief Set neighbour coefficient. Required for assembly of GGQ rule. See: multiple_elements.hpp.
    /// @param Value New Value.
    /// @param Direction Space Direction: 0-x, 1-y, 2-z.
    void SetNeighbourCoefficient(double Value, IndexType Direction){
        mNumberOfNeighbours[Direction] = Value;
    }

    /// @brief Get neighbour coeefficient of this element. Required for assembly of GGQ rule. See: multiple_elements.hpp.
    /// @return double
    double NeighbourCoefficient() const {
        return mNumberOfNeighbours[0]*mNumberOfNeighbours[1]*mNumberOfNeighbours[2];
    }

    /// @brief Set Flag.
    /// @param Value
    void SetVisited(bool Value){
        mIsVisited = Value;
    }

    /// @brief Returns Flag. (see. SetVisited(value)).
    /// @return bool
    bool IsVisited(){
        return mIsVisited;
    }

    ///@}
private:

    ///@name Private member variables
    ///@{
    IntegrationPointVectorType mIntegrationPoints;

    IntegrationPoint1DVectorType mIntegrationPointsX;
    IntegrationPoint1DVectorType mIntegrationPointsY;
    IntegrationPoint1DVectorType mIntegrationPointsZ;

    const IndexType mElementId;
    bool mIsTrimmed;
    bool mIsVisited;

    const BoundingBoxType mBoundsXYZ;
    const BoundingBoxType mBoundsUVW;

    TrimmedDomainPtrType mpTrimmedDomain;
    PointType mNumberOfNeighbours;
    ///@}
}; // End class Element
///@} // QuESo classes

} // End namespace queso

#endif // ELEMENT_INCLUDE_H