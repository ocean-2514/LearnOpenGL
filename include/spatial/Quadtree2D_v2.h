#pragma once

#include "spatial/Geometry2D.h"

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

// Quadtree2D owns the complete index. Nodes are implementation details, while
// locations_ belongs to this tree instance and is shared logically by all nodes.
class Quadtree2D
{
private:
    struct Entry
    {
        ObjectId id;
        AABB2 bounds;
    };

    struct Node
    {
        Node(const AABB2 &nodeRange, std::size_t nodeDepth)
            : range(nodeRange), depth(nodeDepth)
        {
            if (depth < maxDepth)
                populateChildRanges();
        }

        void populateChildRanges()
        {
            const glm::vec2 center = range.center();
            for (std::size_t index = 0; index < childRanges.size(); ++index)
            {
                glm::vec2 min;
                glm::vec2 max;
                min.x = (index == 0 || index == 2) ? range.min.x : center.x;
                min.y = (index == 0 || index == 1) ? center.y : range.min.y;
                max.x = (index == 0 || index == 2) ? center.x : range.max.x;
                max.y = (index == 0 || index == 1) ? range.max.y : center.y;
                childRanges[index] = {min, max};
            }
        }

        bool hasChildren() const
        {
            return children[0] != nullptr;
        }

        std::vector<Entry> entries;
        std::array<std::unique_ptr<Node>, 4> children;
        std::array<AABB2, 4> childRanges;
        AABB2 range;
        std::size_t depth;
    };

    struct ObjectLocation
    {
        Node *node = nullptr;
        std::size_t slot = 0;
    };

public:
    explicit Quadtree2D(const AABB2 &range, std::size_t capacity = 10)
        : root_(std::make_unique<Node>(range, 0)), capacity_(capacity)
    {
        if (capacity_ == 0)
            capacity_ = 1;
    }

    Quadtree2D(const Quadtree2D &) = delete;
    Quadtree2D &operator=(const Quadtree2D &) = delete;
    Quadtree2D(Quadtree2D &&) noexcept = default;
    Quadtree2D &operator=(Quadtree2D &&) noexcept = default;

    bool insert(ObjectId id, const AABB2 &bounds)
    {
        ensureLocationCapacity(id);
        if (hasValidLocation(id))
            return false;

        if (!root_->range.contains(bounds))
            return false;

        insertEntry(*root_, {id, bounds});
        return true;
    }

    bool contains(ObjectId id) const
    {
        return hasValidLocation(id);
    }

    bool remove(ObjectId id)
    {
        if (!hasValidLocation(id))
            return false;

        removeAtLocation(id);
        return true;
    }

    // Returns true when the object remains indexed. Moving outside the root
    // removes it from the tree and returns false.
    bool update(ObjectId id, const AABB2 &newBounds)
    {
        if (!hasValidLocation(id))
            return false;

        removeAtLocation(id);
        if (!root_->range.contains(newBounds))
            return false;

        insertEntry(*root_, {id, newBounds});
        return true;
    }

    void clear()
    {
        const AABB2 range = root_->range;
        root_ = std::make_unique<Node>(range, 0);
        locations_.clear();
    }

    std::vector<ObjectId> getAllObjects() const
    {
        std::vector<ObjectId> result;
        getAllObjects(*root_, result);
        return result;
    }

    void getAllObjects(std::vector<ObjectId> &result) const
    {
        getAllObjects(*root_, result);
    }

    std::vector<ObjectId> queryRange(const AABB2 &range) const
    {
        std::vector<ObjectId> result;
        queryRange(*root_, range, result);
        return result;
    }

    void queryRange(const AABB2 &range, std::vector<ObjectId> &result) const
    {
        queryRange(*root_, range, result);
    }

    void getNodeRanges(std::vector<AABB2> &ranges) const
    {
        getNodeRanges(*root_, ranges);
    }

private:
    static constexpr std::size_t maxDepth = 4;

    std::unique_ptr<Node> root_;
    std::vector<ObjectLocation> locations_;
    std::size_t capacity_;

    void ensureLocationCapacity(ObjectId id)
    {
        const std::size_t requiredSize = static_cast<std::size_t>(id) + 1;
        if (locations_.size() < requiredSize)
            locations_.resize(requiredSize);
    }

    bool hasValidLocation(ObjectId id) const
    {
        if (id >= locations_.size())
            return false;

        const ObjectLocation &location = locations_[id];
        return location.node != nullptr &&
               location.slot < location.node->entries.size() &&
               location.node->entries[location.slot].id == id;
    }

    void removeAtLocation(ObjectId id)
    {
        const ObjectLocation location = locations_[id];
        std::vector<Entry> &entries = location.node->entries;
        const std::size_t lastSlot = entries.size() - 1;

        if (location.slot != lastSlot)
        {
            entries[location.slot] = entries[lastSlot];
            const ObjectId movedId = entries[location.slot].id;
            locations_[movedId] = {location.node, location.slot};
        }

        entries.pop_back();
        locations_[id] = {};
    }

    static int containingChild(const Node &node, const AABB2 &bounds)
    {
        for (std::size_t index = 0; index < node.childRanges.size(); ++index)
        {
            if (node.childRanges[index].contains(bounds))
                return static_cast<int>(index);
        }
        return -1;
    }

    void createChildren(Node &node)
    {
        if (node.hasChildren() || node.depth >= maxDepth)
            return;

        for (std::size_t index = 0; index < node.children.size(); ++index)
            node.children[index] = std::make_unique<Node>(
                node.childRanges[index], node.depth + 1);
    }

    void appendEntry(Node &node, const Entry &entry)
    {
        node.entries.push_back(entry);
        locations_[entry.id] = {&node, node.entries.size() - 1};
    }

    void split(Node &node)
    {
        createChildren(node);
        std::vector<Entry> previousEntries = std::move(node.entries);
        node.entries.clear();

        for (const Entry &entry : previousEntries)
            insertEntry(node, entry);
    }

    void insertEntry(Node &node, const Entry &entry)
    {
        if (node.hasChildren())
        {
            const int childIndex = containingChild(node, entry.bounds);
            if (childIndex >= 0)
            {
                insertEntry(*node.children[static_cast<std::size_t>(childIndex)], entry);
                return;
            }

            // Entries crossing a split line stay in their current node.
            appendEntry(node, entry);
            return;
        }

        if (node.entries.size() < capacity_ || node.depth >= maxDepth)
        {
            appendEntry(node, entry);
            return;
        }

        split(node);
        insertEntry(node, entry);
    }

    static void getAllObjects(const Node &node, std::vector<ObjectId> &result)
    {
        result.reserve(result.size() + node.entries.size());
        for (const Entry &entry : node.entries)
            result.push_back(entry.id);

        for (const auto &child : node.children)
        {
            if (child != nullptr)
                getAllObjects(*child, result);
        }
    }

    static void queryRange(const Node &node, const AABB2 &range,
                           std::vector<ObjectId> &result)
    {
        if (!range.intersects(node.range))
            return;

        if (range.contains(node.range))
        {
            getAllObjects(node, result);
            return;
        }

        for (const Entry &entry : node.entries)
        {
            if (range.intersects(entry.bounds))
                result.push_back(entry.id);
        }

        for (const auto &child : node.children)
        {
            if (child != nullptr)
                queryRange(*child, range, result);
        }
    }

    static void getNodeRanges(const Node &node, std::vector<AABB2> &ranges)
    {
        ranges.push_back(node.range);
        for (const auto &child : node.children)
        {
            if (child != nullptr)
                getNodeRanges(*child, ranges);
        }
    }
};
