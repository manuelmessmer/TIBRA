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

#ifndef CONDITION_INCLUDE_HPP
#define CONDITION_INCLUDE_HPP

//// STL includes
#include "queso/includes/settings.hpp"
#include "queso/containers/condition_segment.hpp"
#include "queso/containers/triangle_mesh.hpp"

namespace queso {

///@name QuESo Classes
///@{

/**
 * @class  Condition
 * @author Manuel Messmer
 * @brief  Interface for conditions. Stores respective triangle mesh, condition settings and a list of condition segments.
 *         Each segment is clipped to the element boundaries in the background grid and holds the respective
 *         section of the triangle mesh.
 * @see    condition_segment.h
**/
template<typename TElementType>
class Condition {
public:

    ///@name Type Definitions
    ///@{
    typedef TElementType ElementType;

    typedef ConditionSegment<ElementType> ConditionSegmentType;
    typedef Unique<ConditionSegmentType> ConditionSegmentPtrType;
    typedef std::vector<ConditionSegmentPtrType> ConditionSegmentPtrVectorType;

    ///@}
    ///@name Life Cycle
    ///@{

    /// @brief Constructor
    /// @param pTriangleMesh Ptr is moved. Ownership is passed to Condition.
    /// @param rConditionSettings
    Condition(Unique<TriangleMeshInterface>& pTriangleMesh, const SettingsBaseType& rConditionSettings )
        : mpTriangleMesh(std::move(pTriangleMesh)), mConditionSettings(rConditionSettings)
    {
    }

    /// @brief Adds new ConditionSegment to this condition. Segment is moved into container.
    /// @param pNewSegment
    void AddSegment(ConditionSegmentPtrType& pNewSegment) {
        mSegments.push_back(std::move(pNewSegment));
    }

    /// @brief Returns all stored ConditionSegments.
    /// @return const ConditionSegmentPtrVectorType&
    const ConditionSegmentPtrVectorType& GetSegments() const {
        return mSegments;
    }

    /// @brief Returns triangle mesh.
    /// @return const TriangleMeshInterface&
    const TriangleMeshInterface& GetTriangleMesh() const {
        return *mpTriangleMesh;
    }

    /// @brief Returns condition settings.
    /// @return const SettingsBaseType&
    const SettingsBaseType& GetSettings() const {
        return mConditionSettings;
    }

    /// @brief Returns the number of segments.
    /// @return IndexType
    IndexType NumberOfSegments() const {
        return mSegments.size();
    }

    ///@}
    ///@name Iterators
    ///@{

    /// @brief Returns dereferenced iterator. This means iterator does not point to UniquePtr,
    ///        but directly to the actual object.
    /// @return DereferenceIterator
    DereferenceIterator<typename ConditionSegmentPtrVectorType::iterator> SegmentsBegin() {
        return dereference_iterator(mSegments.begin());
    }

    /// @brief Returns dereferenced iterator. This means iterator does not point to UniquePtr,
    ///        but directly to the actual object.
    /// @return DereferenceIterator
    DereferenceIterator<typename ConditionSegmentPtrVectorType::const_iterator> SegmentsBegin() const {
        return dereference_iterator(mSegments.begin());
    }

    /// @brief Returns dereferenced iterator. This means iterator does not point to UniquePtr,
    ///        but directly to the actual object.
    /// @return DereferenceIterator
    DereferenceIterator<typename ConditionSegmentPtrVectorType::iterator> SegmentsEnd() {
        return dereference_iterator(mSegments.end());
    }

    /// @brief Returns dereferenced iterator. This means iterator does not point to UniquePtr,
    ///        but directly to the actual object.
    /// @return DereferenceIterator
    DereferenceIterator<typename ConditionSegmentPtrVectorType::const_iterator> SegmentsEnd() const {
        return dereference_iterator(mSegments.end());
    }

    /// @brief Returns iterator to raw ptr. This means iterator does not point to UniquePtr<Object>,
    ///        but to Object*.
    /// @return DereferenceIterator
    RawPointerIterator<typename ConditionSegmentPtrVectorType::iterator> SegmentsBeginToPtr() {
        return raw_pointer_iterator(mSegments.begin());
    }

    /// @brief Returns iterator to raw ptr. This means iterator does not point to UniquePtr<Object>,
    ///        but to Object*.
    /// @return DereferenceIterator
    RawPointerIterator<typename ConditionSegmentPtrVectorType::const_iterator> SegmentsBeginToPtr() const {
        return raw_pointer_iterator(mSegments.begin());
    }

    /// @brief Returns iterator to raw ptr. This means iterator does not point to UniquePtr<Object>,
    ///        but to Object*.
    /// @return DereferenceIterator
    RawPointerIterator<typename ConditionSegmentPtrVectorType::iterator> SegmentsEndToPtr() {
        return raw_pointer_iterator(mSegments.end());
    }

    /// @brief Returns iterator to raw ptr. This means iterator does not point to UniquePtr<Object>,
    ///        but to Object*.
    /// @return DereferenceIterator
    RawPointerIterator<typename ConditionSegmentPtrVectorType::const_iterator> SegmentsEndToPtr() const {
        return raw_pointer_iterator(mSegments.end());
    }

private:
    ///@}
    ///@name Private Members
    ///@{

    Unique<TriangleMeshInterface> mpTriangleMesh;
    const SettingsBaseType& mConditionSettings;
    ConditionSegmentPtrVectorType mSegments;

    ///@}
}; // End class Condition
///@}
} // End queso namespace.

#endif // End CONDITION_INCLUDE_HPP