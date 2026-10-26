#pragma once

#include "spatial/Geometry2D.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

class BVHTree
{
private:
    struct BVHNode
    {
        AABB2 bounds;
        BVHNode* parent = nullptr;
        std::unique_ptr<BVHNode> left;
        std::unique_ptr<BVHNode> right;
        std::size_t first = 0;
        std::size_t count = 0;

        bool isLeaf() const
        {
            return left == nullptr && right == nullptr;
        }
    };

    struct BVHPrimitive
    {
        ObjectId id;
        AABB2 bounds;
        glm::vec2 center;
    };

    using PrimitiveIterator = std::vector<BVHPrimitive>::iterator;

public:
    struct DebugNode
    {
        AABB2 bounds;
        std::size_t depth;
        bool leaf;
    };

    void build(const std::vector<Object2D> &objects)
    {
        m_quality = m_qualityOnBuilt = 0.0f;
        populatePrimitives(objects);
        m_root = buildNode(m_primitives.begin(), m_primitives.end());

        m_qualityOnBuilt = m_quality;
    }

    void clear()
    {
        m_quality = m_qualityOnBuilt = 0.0f;
        m_root.reset();
        m_primitives.clear();
        m_objectLeaves.clear();
        m_primitiveIndices.clear();
    }

    bool empty() const { return m_root == nullptr; }
    std::size_t size() const { return m_primitives.size(); }

    std::vector<ObjectId> queryRange(const AABB2 &range) const
    {
        std::vector<ObjectId> result;
        queryRange(range, m_root.get(), result);
        return result;
    }

    void queryRange(const AABB2 &range, std::vector<ObjectId> &result) const
    {
        queryRange(range, m_root.get(), result);
    }

    void update(ObjectId id, const AABB2& newBounds) {
        if (id >= m_objectLeaves.size() || id >= m_primitiveIndices.size())
            return;

        BVHNode* node = m_objectLeaves[id];
        if (node == nullptr)
            return;

        const std::size_t primitiveIndex = m_primitiveIndices[id];
        if (primitiveIndex >= m_primitives.size() ||
            m_primitives[primitiveIndex].id != id)
            return;

        BVHPrimitive& primitive = m_primitives[primitiveIndex];

        primitive.bounds = newBounds;
        primitive.center = newBounds.center();
        m_quality -= node->bounds.area();
        node->bounds = computeBounds(m_primitives.begin() + node->first, 
            m_primitives.begin() + node->first + node->count);
        m_quality += node->bounds.area();
        refitAncestors(node->parent);

        if (m_quality > m_qualityOnBuilt * m_threshold) rebuild();
    }

    void getDebugNodes(std::vector<DebugNode> &nodes) const
    {
        getDebugNodes(m_root.get(), 0, nodes);
    }

private:
    std::unique_ptr<BVHNode> m_root;
    std::vector<BVHPrimitive> m_primitives;
    std::vector<BVHNode*> m_objectLeaves;
    std::vector<std::size_t> m_primitiveIndices;
    std::size_t m_leafCapacity = 4;
    float m_quality = 0.0f;
    float m_qualityOnBuilt = 0.0f;
    const float m_threshold = 1.75f; 

    void populatePrimitives(const std::vector<Object2D> &objects)
    {
        m_primitives.clear();
        m_primitives.reserve(objects.size());
        for (const Object2D &object : objects)
            m_primitives.push_back({object.id, object.bounds, object.bounds.center()});
        
        prepareLookupTables();
    }

    void prepareLookupTables()
    {
        std::size_t lookupSize = 0;
        for (const BVHPrimitive &primitive : m_primitives)
            lookupSize = std::max(lookupSize,
                                  static_cast<std::size_t>(primitive.id) + 1);

        m_objectLeaves.assign(lookupSize, nullptr);
        m_primitiveIndices.assign(lookupSize, 0);
    }

    static AABB2 computeBounds(PrimitiveIterator begin, PrimitiveIterator end)
    {
        AABB2 bounds = begin->bounds;
        for (PrimitiveIterator it = begin + 1; it != end; ++it)
            bounds = AABB2::merge(bounds, it->bounds);
        return bounds;
    }

    static AABB2 computeCentroidBounds(PrimitiveIterator begin, PrimitiveIterator end)
    {
        AABB2 bounds{begin->center, begin->center};
        for (PrimitiveIterator it = begin + 1; it != end; ++it)
        {
            bounds.min = glm::min(bounds.min, it->center);
            bounds.max = glm::max(bounds.max, it->center);
        }
        return bounds;
    }

    std::unique_ptr<BVHNode> buildNode(PrimitiveIterator begin, PrimitiveIterator end)
    {
        if (begin == end)
            return nullptr;

        auto node = std::make_unique<BVHNode>();
        node->bounds = computeBounds(begin, end);
        m_quality += node->bounds.area();
        const std::size_t count = static_cast<std::size_t>(end - begin);

        if (count <= m_leafCapacity)
        {
            node->first = static_cast<std::size_t>(begin - m_primitives.begin());
            node->count = count;
            for (auto it = begin; it != end; ++it) {
                if (it->id >= m_objectLeaves.size()) continue;
                m_objectLeaves[it->id] = node.get();
                m_primitiveIndices[it->id] = 
                    static_cast<std::size_t>(it - m_primitives.begin());
            }
            return node;
        }

        const AABB2 centroidBounds = computeCentroidBounds(begin, end);
        const glm::vec2 centroidExtent = centroidBounds.size();
        const int axis = centroidExtent.x >= centroidExtent.y ? 0 : 1;
        const PrimitiveIterator middle = begin + count / 2;

        std::nth_element(begin, middle, end,
                         [axis](const BVHPrimitive &a, const BVHPrimitive &b) {
                             if (a.center[axis] != b.center[axis])
                                 return a.center[axis] < b.center[axis];
                             return a.id < b.id;
                         });

        node->left = buildNode(begin, middle);
        node->right = buildNode(middle, end);
        if (node->left) node->left->parent = node.get();
        if (node->right) node->right->parent = node.get();
        return node;
    }

    void queryRange(const AABB2 &range, const BVHNode *node,
                    std::vector<ObjectId> &result) const
    {
        if (node == nullptr || !range.intersects(node->bounds))
            return;

        if (node->isLeaf())
        {
            const std::size_t end = node->first + node->count;
            for (std::size_t index = node->first; index < end; ++index)
            {
                if (range.intersects(m_primitives[index].bounds))
                    result.push_back(m_primitives[index].id);
            }
            return;
        }

        queryRange(range, node->left.get(), result);
        queryRange(range, node->right.get(), result);
    }

    void refitAncestors(BVHNode* node) {
        while (node != nullptr) {
            m_quality -= node->bounds.area();
            node->bounds = AABB2::merge(
                node->left->bounds,
                node->right->bounds);
            m_quality += node->bounds.area();
                
            node = node->parent;
        }
    }

    static void getDebugNodes(const BVHNode *node, std::size_t depth,
                              std::vector<DebugNode> &nodes)
    {
        if (node == nullptr)
            return;
        nodes.push_back({node->bounds, depth, node->isLeaf()});
        getDebugNodes(node->left.get(), depth + 1, nodes);
        getDebugNodes(node->right.get(), depth + 1, nodes);
    }

    void rebuild() {
        m_quality = m_qualityOnBuilt = 0.0f;
        m_root.reset();
        prepareLookupTables();
        m_root = buildNode(m_primitives.begin(), m_primitives.end());

        m_qualityOnBuilt = m_quality;
    }
};
