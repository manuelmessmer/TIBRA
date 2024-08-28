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

// STL includes
#include <cstdlib>

// Project includes
#include "queso/utilities/voxel_indexing_utilities.h"

namespace queso {

// Static member operations for VoxelIndexing
Vector3i VoxelIndexing::GetMatrixIndicesFromVectorIndex(const IndexType Index, const Vector3i& rNumberOfElements) {
    Vector3i result;
    const IndexType index_in_row_column_plane = Index % (rNumberOfElements[0]*rNumberOfElements[1]);
    result[0] = index_in_row_column_plane % rNumberOfElements[0]; // row
    result[1] = index_in_row_column_plane / rNumberOfElements[0]; // column
    result[2] = Index / (rNumberOfElements[0]*rNumberOfElements[1]);   // depth

    return result;
}

IndexType VoxelIndexing::GetVectorIndexFromMatrixIndices(const IndexType RowIndex, const IndexType ColumnIndex, const IndexType DepthIndex, const Vector3i& rNumberOfElements) {
    return DepthIndex * (rNumberOfElements[1]*rNumberOfElements[0]) + ColumnIndex * rNumberOfElements[0] + RowIndex;
}

IndexType VoxelIndexing::GetVectorIndexFromMatrixIndices(const Vector3i& rIndices, const Vector3i& rNumberOfElements) {
    return rIndices[2] * (rNumberOfElements[1]*rNumberOfElements[0]) + rIndices[1] * rNumberOfElements[0] + rIndices[0];
}
std::pair<PointType, PointType> VoxelIndexing::GetBoundingBoxFromIndex(IndexType Index, const PointType& rLowerBound, const PointType& rUpperBound, const Vector3i& rNumberOfElements)  {
    const auto indices = GetMatrixIndicesFromVectorIndex(Index, rNumberOfElements);
    return GetBoundingBoxFromIndex( indices[0], indices[1], indices[2], rLowerBound, rUpperBound, rNumberOfElements);
}

std::pair<PointType, PointType> VoxelIndexing::GetBoundingBoxFromIndex(const Vector3i& rIndices, const PointType& rLowerBound, const PointType& rUpperBound, const Vector3i& rNumberOfElements )  {
    return GetBoundingBoxFromIndex( rIndices[0], rIndices[1], rIndices[2], rLowerBound, rUpperBound, rNumberOfElements);
}

std::pair<PointType, PointType> VoxelIndexing::GetBoundingBoxFromIndex(IndexType i, IndexType j, IndexType k, const PointType& rLowerBound, const PointType& rUpperBound, const Vector3i& rNumberOfElements)  {
    const PointType indices_d{ static_cast<double>(i), static_cast<double>(j), static_cast<double>(k) };
    PointType delta;
    delta[0] = std::abs(rUpperBound[0] - rLowerBound[0]) / (rNumberOfElements[0]);
    delta[1] = std::abs(rUpperBound[1] - rLowerBound[1]) / (rNumberOfElements[1]);
    delta[2] = std::abs(rUpperBound[2] - rLowerBound[2]) / (rNumberOfElements[2]);
    return std::make_pair( Math::Add( rLowerBound, Math::MultElementWise(delta, indices_d)),
                           Math::Add( rLowerBound, Math::MultElementWise(delta, Math::Add({1.0, 1.0, 1.0}, indices_d))) );
}

// Member operations for VoxelIndexer
Vector3i VoxelIndexer::GetMatrixIndicesFromVectorIndex(const IndexType Index ) const {
    return VoxelIndexing::GetMatrixIndicesFromVectorIndex(Index, mNumberOfElements);
}

IndexType VoxelIndexer::GetVectorIndexFromMatrixIndices(const IndexType RowIndex, const IndexType ColumnIndex, const IndexType DepthIndex ) const {
    return VoxelIndexing::GetVectorIndexFromMatrixIndices(RowIndex, ColumnIndex, DepthIndex, mNumberOfElements );
}

IndexType VoxelIndexer::GetVectorIndexFromMatrixIndices(const Vector3i& rIndices ) const {
    return VoxelIndexing::GetVectorIndexFromMatrixIndices(rIndices, mNumberOfElements );
}

std::pair<PointType, PointType> VoxelIndexer::GetBoundingBoxXYZFromIndex(IndexType Index) const {
    return VoxelIndexing::GetBoundingBoxFromIndex(Index, mBoundXYZ.first, mBoundXYZ.second, mNumberOfElements);
}

std::pair<PointType, PointType> VoxelIndexer::GetBoundingBoxXYZFromIndex(const Vector3i& Indices) const {
    return VoxelIndexing::GetBoundingBoxFromIndex(Indices, mBoundXYZ.first, mBoundXYZ.second, mNumberOfElements);
}

std::pair<PointType, PointType> VoxelIndexer::GetBoundingBoxXYZFromIndex(IndexType i, IndexType j, IndexType k) const {
    return VoxelIndexing::GetBoundingBoxFromIndex(i, j, k, mBoundXYZ.first, mBoundXYZ.second, mNumberOfElements);
}

std::pair<PointType, PointType> VoxelIndexer::GetBoundingBoxUVWFromIndex(IndexType Index) const {
    if( mBSplineMesh ){
        return VoxelIndexing::GetBoundingBoxFromIndex(Index, mBoundUVW.first, mBoundUVW.second, mNumberOfElements);
    }
    return mBoundUVW;
}

std::pair<PointType, PointType> VoxelIndexer::GetBoundingBoxUVWFromIndex(const Vector3i& Indices) const {
    if( mBSplineMesh ){
        return VoxelIndexing::GetBoundingBoxFromIndex(Indices, mBoundUVW.first, mBoundUVW.second, mNumberOfElements);
    }
    return mBoundUVW;
}

std::pair<PointType, PointType> VoxelIndexer::GetBoundingBoxUVWFromIndex(IndexType i, IndexType j, IndexType k) const {
    if( mBSplineMesh ){
        return VoxelIndexing::GetBoundingBoxFromIndex(i, j, k, mBoundUVW.first, mBoundUVW.second, mNumberOfElements);
    }
    return mBoundUVW;
}

IndexType VoxelIndexer::NumberOfElements() const {
    return mNumberOfElements[0]*mNumberOfElements[1]*mNumberOfElements[2];
}

} // End namespace queso