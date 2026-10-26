#pragma once

#include "spatial/Geometry2D.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

class DynamicBVHTree2D
{
private:
    using NodeIndex = std::int32_t;
    static constexpr NodeIndex nullNode = -1;
    static constexpr ObjectId invalidObject = std::numeric_limits<ObjectId>::max();

    struct DynamicBVHNode
    {
        AABB2 bounds;      // Fat AABB used by the hierarchy.
        AABB2 tightBounds; // Actual object AABB used for exact query results.

        NodeIndex parent = nullNode;
        NodeIndex left = nullNode;
        NodeIndex right = nullNode;
        NodeIndex next = nullNode;

        ObjectId object = invalidObject;
        int height = 0;
        bool allocated = false;

        bool isLeaf() const { return left == nullNode; }
    };

public:
    using ProxyId = std::int32_t;
    static constexpr ProxyId invalidProxy = nullNode;

    struct DebugNode
    {
        AABB2 bounds;
        int height;
        bool leaf;
    };

    explicit DynamicBVHTree2D(float fatMargin = 10.0f,
                              float predictionMultiplier = 2.0f)
        : m_fatMargin(std::max(0.0f, fatMargin)),
          m_predictionMultiplier(std::max(0.0f, predictionMultiplier))
    {
    }

    ProxyId createProxy(ObjectId id, const AABB2 &bounds)
    {
        const NodeIndex leaf = allocateNode();
        DynamicBVHNode &node = m_nodes[leaf];
        node.object = id;
        node.tightBounds = bounds;
        node.bounds = makeFatAABB(bounds, glm::vec2(0.0f));
        node.height = 0;
        insertLeaf(leaf);
        ++m_proxyCount;
        return leaf;
    }

    // Returns true if the leaf had to be removed and reinserted.
    bool moveProxy(ProxyId proxy, const AABB2 &newBounds,
                   const glm::vec2 &displacement = glm::vec2(0.0f))
    {
        if (!isValidProxy(proxy))
            return false;

        DynamicBVHNode &leaf = m_nodes[proxy];
        leaf.tightBounds = newBounds;
        if (leaf.bounds.contains(newBounds))
            return false;

        detachLeaf(proxy);
        m_nodes[proxy].bounds = makeFatAABB(newBounds, displacement);
        insertLeaf(proxy);
        return true;
    }

    bool destroyProxy(ProxyId proxy)
    {
        if (!isValidProxy(proxy))
            return false;

        detachLeaf(proxy);
        freeNode(proxy);
        --m_proxyCount;
        return true;
    }

    std::vector<ObjectId> query(const AABB2 &range) const
    {
        std::vector<ObjectId> result;
        query(range, result);
        return result;
    }

    void query(const AABB2 &range, std::vector<ObjectId> &result) const
    {
        if (m_root == nullNode)
            return;

        std::vector<NodeIndex> stack;
        stack.push_back(m_root);
        while (!stack.empty())
        {
            const NodeIndex index = stack.back();
            stack.pop_back();
            const DynamicBVHNode &node = m_nodes[index];

            if (!range.intersects(node.bounds))
                continue;

            if (node.isLeaf())
            {
                if (range.intersects(node.tightBounds))
                    result.push_back(node.object);
            }
            else
            {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
    }

    bool isValidProxy(ProxyId proxy) const
    {
        return proxy >= 0 && static_cast<std::size_t>(proxy) < m_nodes.size() &&
               m_nodes[proxy].allocated && m_nodes[proxy].isLeaf() &&
               m_nodes[proxy].object != invalidObject;
    }

    std::size_t size() const { return m_proxyCount; }
    bool empty() const { return m_root == nullNode; }
    int height() const { return m_root == nullNode ? 0 : m_nodes[m_root].height; }

    void clear()
    {
        m_nodes.clear();
        m_root = nullNode;
        m_freeList = nullNode;
        m_proxyCount = 0;
    }

    void getDebugNodes(std::vector<DebugNode> &nodes) const
    {
        if (m_root == nullNode)
            return;

        std::vector<NodeIndex> stack{m_root};
        while (!stack.empty())
        {
            const NodeIndex index = stack.back();
            stack.pop_back();
            const DynamicBVHNode &node = m_nodes[index];
            nodes.push_back({node.bounds, node.height, node.isLeaf()});
            if (!node.isLeaf())
            {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
    }

private:
    std::vector<DynamicBVHNode> m_nodes;
    NodeIndex m_root = nullNode;
    NodeIndex m_freeList = nullNode;
    std::size_t m_proxyCount = 0;
    float m_fatMargin = 10.0f;
    float m_predictionMultiplier = 2.0f;

    void expand()
    {
        const std::size_t oldSize = m_nodes.size();
        const std::size_t amount = std::max<std::size_t>(16, oldSize);
        m_nodes.resize(oldSize + amount);
        for (std::size_t index = oldSize; index < m_nodes.size(); ++index)
        {
            m_nodes[index].next = m_freeList;
            m_freeList = static_cast<NodeIndex>(index);
        }
    }

    NodeIndex allocateNode()
    {
        if (m_freeList == nullNode)
            expand();

        const NodeIndex index = m_freeList;
        const NodeIndex next = m_nodes[index].next;
        m_nodes[index] = DynamicBVHNode{};
        m_nodes[index].allocated = true;
        m_freeList = next;
        return index;
    }

    void freeNode(NodeIndex index)
    {
        m_nodes[index] = DynamicBVHNode{};
        m_nodes[index].height = -1;
        m_nodes[index].next = m_freeList;
        m_freeList = index;
    }

    AABB2 makeFatAABB(const AABB2 &bounds, const glm::vec2 &displacement) const
    {
        AABB2 fat{
            bounds.min - glm::vec2(m_fatMargin),
            bounds.max + glm::vec2(m_fatMargin)};

        const glm::vec2 prediction = displacement * m_predictionMultiplier;
        for (int axis = 0; axis < 2; ++axis)
        {
            if (prediction[axis] < 0.0f)
                fat.min[axis] += prediction[axis];
            else
                fat.max[axis] += prediction[axis];
        }
        return fat;
    }

    // SAH-style greedy descent. Area is used for this 2D range-query tree.
    NodeIndex findBestSibling(NodeIndex leaf) const
    {
        const AABB2 &leafBounds = m_nodes[leaf].bounds;
        NodeIndex current = m_root;
        while (!m_nodes[current].isLeaf())
        {
            const DynamicBVHNode &node = m_nodes[current];
            const float currentArea = node.bounds.area();
            const AABB2 combined = AABB2::merge(node.bounds, leafBounds);
            const float combinedArea = combined.area();
            const float directCost = combinedArea;
            const float inheritanceCost = combinedArea - currentArea;

            const auto childCost = [this, &leafBounds, inheritanceCost](NodeIndex child) {
                const DynamicBVHNode &childNode = m_nodes[child];
                const float mergedArea = AABB2::merge(childNode.bounds, leafBounds).area();
                if (childNode.isLeaf())
                    return mergedArea + inheritanceCost;
                return mergedArea - childNode.bounds.area() + inheritanceCost;
            };

            const float leftCost = childCost(node.left);
            const float rightCost = childCost(node.right);
            if (directCost <= leftCost && directCost <= rightCost)
                break;
            current = leftCost < rightCost ? node.left : node.right;
        }
        return current;
    }

    void insertLeaf(NodeIndex leaf)
    {
        if (m_root == nullNode)
        {
            m_root = leaf;
            m_nodes[leaf].parent = nullNode;
            return;
        }

        const NodeIndex sibling = findBestSibling(leaf);
        const NodeIndex oldParent = m_nodes[sibling].parent;
        const NodeIndex newParent = allocateNode();

        DynamicBVHNode &parentNode = m_nodes[newParent];
        parentNode.parent = oldParent;
        parentNode.left = sibling;
        parentNode.right = leaf;
        parentNode.object = invalidObject;
        parentNode.bounds = AABB2::merge(m_nodes[sibling].bounds, m_nodes[leaf].bounds);
        parentNode.height = 1 + std::max(m_nodes[sibling].height, m_nodes[leaf].height);

        m_nodes[sibling].parent = newParent;
        m_nodes[leaf].parent = newParent;
        if (oldParent == nullNode)
            m_root = newParent;
        else if (m_nodes[oldParent].left == sibling)
            m_nodes[oldParent].left = newParent;
        else
            m_nodes[oldParent].right = newParent;

        refitAndBalance(newParent);
    }

    // Detaches a leaf but keeps the leaf allocated, allowing moveProxy to reuse it.
    void detachLeaf(NodeIndex leaf)
    {
        if (leaf == m_root)
        {
            m_root = nullNode;
            m_nodes[leaf].parent = nullNode;
            return;
        }

        const NodeIndex parent = m_nodes[leaf].parent;
        const NodeIndex grandParent = m_nodes[parent].parent;
        const NodeIndex sibling = m_nodes[parent].left == leaf
                                      ? m_nodes[parent].right
                                      : m_nodes[parent].left;

        if (grandParent == nullNode)
        {
            m_root = sibling;
            m_nodes[sibling].parent = nullNode;
            freeNode(parent);
        }
        else
        {
            if (m_nodes[grandParent].left == parent)
                m_nodes[grandParent].left = sibling;
            else
                m_nodes[grandParent].right = sibling;
            m_nodes[sibling].parent = grandParent;
            freeNode(parent);
            refitAndBalance(grandParent);
        }
        m_nodes[leaf].parent = nullNode;
    }

    void refitAndBalance(NodeIndex index)
    {
        while (index != nullNode)
        {
            index = balance(index);
            DynamicBVHNode &node = m_nodes[index];
            if (!node.isLeaf())
            {
                const DynamicBVHNode &left = m_nodes[node.left];
                const DynamicBVHNode &right = m_nodes[node.right];
                node.height = 1 + std::max(left.height, right.height);
                node.bounds = AABB2::merge(left.bounds, right.bounds);
            }
            index = node.parent;
        }
    }

    NodeIndex balance(NodeIndex aIndex)
    {
        DynamicBVHNode &a = m_nodes[aIndex];
        if (a.isLeaf() || a.height < 2)
            return aIndex;

        const NodeIndex bIndex = a.left;
        const NodeIndex cIndex = a.right;
        const int heightBalance = m_nodes[cIndex].height - m_nodes[bIndex].height;

        if (heightBalance > 1)
            return rotateRightChildUp(aIndex, cIndex);
        if (heightBalance < -1)
            return rotateLeftChildUp(aIndex, bIndex);
        return aIndex;
    }

    NodeIndex rotateRightChildUp(NodeIndex aIndex, NodeIndex cIndex)
    {
        const NodeIndex oldParent = m_nodes[aIndex].parent;
        const NodeIndex fIndex = m_nodes[cIndex].left;
        const NodeIndex gIndex = m_nodes[cIndex].right;

        m_nodes[cIndex].left = aIndex;
        m_nodes[cIndex].parent = oldParent;
        m_nodes[aIndex].parent = cIndex;
        replaceParentChild(oldParent, aIndex, cIndex);

        if (m_nodes[fIndex].height > m_nodes[gIndex].height)
        {
            m_nodes[cIndex].right = fIndex;
            m_nodes[aIndex].right = gIndex;
            m_nodes[gIndex].parent = aIndex;
        }
        else
        {
            m_nodes[cIndex].right = gIndex;
            m_nodes[aIndex].right = fIndex;
            m_nodes[fIndex].parent = aIndex;
        }

        recomputeNode(aIndex);
        recomputeNode(cIndex);
        return cIndex;
    }

    NodeIndex rotateLeftChildUp(NodeIndex aIndex, NodeIndex bIndex)
    {
        const NodeIndex oldParent = m_nodes[aIndex].parent;
        const NodeIndex dIndex = m_nodes[bIndex].left;
        const NodeIndex eIndex = m_nodes[bIndex].right;

        m_nodes[bIndex].left = aIndex;
        m_nodes[bIndex].parent = oldParent;
        m_nodes[aIndex].parent = bIndex;
        replaceParentChild(oldParent, aIndex, bIndex);

        if (m_nodes[dIndex].height > m_nodes[eIndex].height)
        {
            m_nodes[bIndex].right = dIndex;
            m_nodes[aIndex].left = eIndex;
            m_nodes[eIndex].parent = aIndex;
        }
        else
        {
            m_nodes[bIndex].right = eIndex;
            m_nodes[aIndex].left = dIndex;
            m_nodes[dIndex].parent = aIndex;
        }

        recomputeNode(aIndex);
        recomputeNode(bIndex);
        return bIndex;
    }

    void replaceParentChild(NodeIndex parent, NodeIndex oldChild, NodeIndex newChild)
    {
        if (parent == nullNode)
            m_root = newChild;
        else if (m_nodes[parent].left == oldChild)
            m_nodes[parent].left = newChild;
        else
            m_nodes[parent].right = newChild;
    }

    void recomputeNode(NodeIndex index)
    {
        DynamicBVHNode &node = m_nodes[index];
        const DynamicBVHNode &left = m_nodes[node.left];
        const DynamicBVHNode &right = m_nodes[node.right];
        node.bounds = AABB2::merge(left.bounds, right.bounds);
        node.height = 1 + std::max(left.height, right.height);
    }
};
