#pragma once

#include "spatial/Geometry2D.h"
#include <vector>
#include <array>
#include <memory>
#include <cstddef>
#include <algorithm>

class Quadtree2D
{
public:
    Quadtree2D(const AABB2& range, std::size_t depth = 0)    
        : m_range(range), m_depth(depth) {
        if (m_depth < MAX_DEPTH) {
            populateChildRanges();
        }
    }

    // need to make sure id is not in the tree before
    bool insert(ObjectId id, const AABB2& bounds) {
        Quadtree2D* tree = locateInQuadtree(bounds, true);
        if (tree) {
            tree->m_objects.push_back(id);
            return true;
        }
        return false;
    }

    bool contains(ObjectId id) const {
        for (auto obj : m_objects) {
            if (obj == id) return true;
        }

        for (const auto& child : m_children) {
            if (child == nullptr) continue;
            if (child->contains(id)) return true;
        }

        return false;
    }

    std::vector<ObjectId> getAllObjects() const {
        std::vector<ObjectId> res;
        getAllObjects(res);
        return res;
    }

    void getAllObjects(std::vector<ObjectId>& objects) const {
        objects.insert(objects.end(), m_objects.begin(), m_objects.end());
        for (auto& child : m_children) {
            if (child == nullptr) continue;

            child->getAllObjects(objects);
        }
    }

    void getNodeRanges(std::vector<AABB2>& ranges) const {
        ranges.push_back(m_range);
        for (const auto& child : m_children) {
            if (child != nullptr) child->getNodeRanges(ranges);
        }
    }

    // 
    std::vector<ObjectId> queryRange(const AABB2& range, 
        const std::vector<Object2D> &objects) const {
        
        std::vector<ObjectId> res;
        queryRange(range, objects, res);

        return res;
    }

    void queryRange(const AABB2& range, 
        const std::vector<Object2D>& objects,
        std::vector<ObjectId>& res) const {
        
        if (!range.intersects(m_range)) return;
        if (range.contains(m_range)) {
            getAllObjects(res);
            return;
        }
        for (auto obj : m_objects) {
            if (obj >= objects.size()) continue;
            if (range.intersects(objects[obj].bounds)) {
                res.push_back(obj);
            }
        }
        for (auto& child : m_children) {
            if (child == nullptr) continue;
            child->queryRange(range, objects, res);
        }
    }

    void remove(ObjectId id, const AABB2& bounds) {
        Quadtree2D* tree = locateInQuadtree(bounds, false);
        if (tree != nullptr) {
            removeObject(id, tree->m_objects);
        }
    }

    void update(ObjectId id, const AABB2& oldBounds, const AABB2& newBounds) {
        Quadtree2D* oldTree = locateInQuadtree(oldBounds, false);
        Quadtree2D* newTree = locateInQuadtree(newBounds, true);
        if (newTree == oldTree) return;

        if (oldTree) {
            removeObject(id, oldTree->m_objects);
        } 
        if (newTree) {
            newTree->m_objects.push_back(id);
        }
    }

private:
    std::vector<ObjectId> m_objects;
    std::array<std::unique_ptr<Quadtree2D>, 4> m_children;
    std::array<AABB2, 4> m_childRanges;
    AABB2 m_range;
    const std::size_t m_depth;

    static constexpr std::size_t MAX_DEPTH = 4;

    void assureChildren(int index) {
        if (m_depth >= MAX_DEPTH) return;

        if (m_children[index] == nullptr) {
            m_children[index] = std::make_unique<Quadtree2D>(
                m_childRanges[index], m_depth + 1);
        }
    }

    void populateChildRanges() {
        for (int i = 0; i < 4; ++i) {
            glm::vec2 center = m_range.center();
            glm::vec2 min, max;
            min.x = (i == 0 || i == 2) ? m_range.min.x : center.x;
            min.y = (i == 0 || i == 1) ? center.y : m_range.min.y;
            max.x = (i == 0 || i == 2) ? center.x : m_range.max.x;
            max.y = (i == 0 || i == 1) ? m_range.max.y : center.y;
            m_childRanges[i] = AABB2{min, max};
        }
    }

    void removeObject(ObjectId id, std::vector<ObjectId>& objects) {
        auto it = std::find(objects.begin(), objects.end(), id);
        if (it == objects.end()) return;

        (*it) = objects.back();
        objects.pop_back();
    }

    Quadtree2D* locateInQuadtree(const AABB2& bounds, bool create = false) {
        if (!m_range.contains(bounds)) return nullptr;

        if (m_depth < MAX_DEPTH) {
            for (int i = 0; i < 4; ++i) {
                if (m_childRanges[i].contains(bounds)) {
                    if (m_children[i] == nullptr) {
                        if (!create) return nullptr;
                        assureChildren(i);
                        if (m_children[i] == nullptr) return nullptr;
                        return m_children[i]->locateInQuadtree(bounds, create);
                    }

                    return m_children[i]->locateInQuadtree(bounds, create);
                }
            }
        }

        return this;
    }

};
