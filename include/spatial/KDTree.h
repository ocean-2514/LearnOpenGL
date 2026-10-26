#pragma once

#include "spatial/Geometry2D.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

class KDTree
{
private:
    struct KDNode
    {
        Point2D point;
        int axis = 0;
        std::unique_ptr<KDNode> left;
        std::unique_ptr<KDNode> right;
    };

    struct Candidate
    {
        float distanceSq;
        Point2D point;
    };

    // Better candidates compare as lower priority, so top() is the worst one.
    struct CandidateCompare
    {
        bool operator()(const Candidate &a, const Candidate &b) const
        {
            if (a.distanceSq != b.distanceSq)
                return a.distanceSq < b.distanceSq;
            return a.point.id < b.point.id;
        }
    };

    using CandidateHeap = std::priority_queue<
        Candidate, std::vector<Candidate>, CandidateCompare>;

public:
    struct SplitLine
    {
        glm::vec2 begin;
        glm::vec2 end;
        int axis;
    };

    void build(const std::vector<Point2D> &points)
    {
        std::vector<Point2D> workingPoints = points;
        m_root = buildNode(workingPoints.begin(), workingPoints.end(), 0);
        m_size = points.size();
    }

    void clear()
    {
        m_root.reset();
        m_size = 0;
    }

    bool empty() const { return m_root == nullptr; }
    std::size_t size() const { return m_size; }

    std::vector<Point2D> queryRange(const AABB2 &range) const
    {
        std::vector<Point2D> result;
        queryRange(m_root.get(), range, result);
        return result;
    }

    void queryRange(const AABB2 &range, std::vector<Point2D> &result) const
    {
        queryRange(m_root.get(), range, result);
    }

    std::optional<Point2D> nearest(const glm::vec2 &query) const
    {
        if (m_root == nullptr)
            return std::nullopt;

        const KDNode *best = m_root.get();
        float bestDistanceSq = square(best->point.pos - query);
        nearestRecursive(m_root.get(), query, best, bestDistanceSq);
        return best->point;
    }

    // Results are sorted from nearest to farthest. k == 0 returns an empty list.
    std::vector<Point2D> kNearest(const glm::vec2 &query, std::size_t k) const
    {
        if (m_root == nullptr || k == 0)
            return {};

        k = std::min(k, m_size);
        CandidateHeap candidates;
        kNearestRecursive(m_root.get(), query, k, candidates);

        std::vector<Point2D> result(candidates.size());
        for (std::size_t index = result.size(); index > 0; --index)
        {
            result[index - 1] = candidates.top().point;
            candidates.pop();
        }
        return result;
    }

    void getSplitLines(const AABB2 &world, std::vector<SplitLine> &lines) const
    {
        getSplitLines(m_root.get(), world, lines);
    }

private:
    using PointIterator = std::vector<Point2D>::iterator;

    std::unique_ptr<KDNode> buildNode(PointIterator begin, PointIterator end, int depth)
    {
        if (begin == end)
            return nullptr;

        const int axis = depth % 2;
        const auto count = end - begin;
        const PointIterator middle = begin + count / 2;
        std::nth_element(begin, middle, end,
                         [axis](const Point2D &a, const Point2D &b) {
                             if (a.pos[axis] != b.pos[axis])
                                 return a.pos[axis] < b.pos[axis];
                             return a.id < b.id;
                         });

        auto node = std::make_unique<KDNode>();
        node->point = *middle;
        node->axis = axis;
        node->left = buildNode(begin, middle, depth + 1);
        node->right = buildNode(middle + 1, end, depth + 1);
        return node;
    }

    static void queryRange(const KDNode *node, const AABB2 &range,
                           std::vector<Point2D> &result)
    {
        if (node == nullptr)
            return;

        if (range.contains(node->point.pos))
            result.push_back(node->point);

        const int axis = node->axis;
        const float split = node->point.pos[axis];
        if (range.min[axis] <= split)
            queryRange(node->left.get(), range, result);
        if (range.max[axis] >= split)
            queryRange(node->right.get(), range, result);
    }

    static void nearestRecursive(const KDNode *node, const glm::vec2 &query,
                                 const KDNode *&best, float &bestDistanceSq)
    {
        if (node == nullptr)
            return;

        const float nodeDistanceSq = square(node->point.pos - query);
        if (nodeDistanceSq < bestDistanceSq ||
            (nodeDistanceSq == bestDistanceSq && node->point.id < best->point.id))
        {
            best = node;
            bestDistanceSq = nodeDistanceSq;
        }

        const int axis = node->axis;
        const float delta = query[axis] - node->point.pos[axis];
        const KDNode *nearChild = delta < 0.0f ? node->left.get() : node->right.get();
        const KDNode *farChild = delta < 0.0f ? node->right.get() : node->left.get();

        nearestRecursive(nearChild, query, best, bestDistanceSq);
        if (delta * delta <= bestDistanceSq)
            nearestRecursive(farChild, query, best, bestDistanceSq);
    }

    static bool betterCandidate(const Candidate &candidate, const Candidate &worst)
    {
        return candidate.distanceSq < worst.distanceSq ||
               (candidate.distanceSq == worst.distanceSq &&
                candidate.point.id < worst.point.id);
    }

    static void kNearestRecursive(const KDNode *node, const glm::vec2 &query,
                                  std::size_t k, CandidateHeap &candidates)
    {
        if (node == nullptr)
            return;

        const Candidate candidate{square(node->point.pos - query), node->point};
        if (candidates.size() < k)
            candidates.push(candidate);
        else if (betterCandidate(candidate, candidates.top()))
        {
            candidates.pop();
            candidates.push(candidate);
        }

        const int axis = node->axis;
        const float delta = query[axis] - node->point.pos[axis];
        const KDNode *nearChild = delta < 0.0f ? node->left.get() : node->right.get();
        const KDNode *farChild = delta < 0.0f ? node->right.get() : node->left.get();

        kNearestRecursive(nearChild, query, k, candidates);
        const float searchRadiusSq = candidates.size() < k
                                         ? std::numeric_limits<float>::infinity()
                                         : candidates.top().distanceSq;
        if (delta * delta <= searchRadiusSq)
            kNearestRecursive(farChild, query, k, candidates);
    }

    static void getSplitLines(const KDNode *node, const AABB2 &region,
                              std::vector<SplitLine> &lines)
    {
        if (node == nullptr)
            return;

        AABB2 leftRegion = region;
        AABB2 rightRegion = region;
        if (node->axis == 0)
        {
            const float x = node->point.pos.x;
            lines.push_back({{x, region.min.y}, {x, region.max.y}, 0});
            leftRegion.max.x = x;
            rightRegion.min.x = x;
        }
        else
        {
            const float y = node->point.pos.y;
            lines.push_back({{region.min.x, y}, {region.max.x, y}, 1});
            leftRegion.max.y = y;
            rightRegion.min.y = y;
        }

        getSplitLines(node->left.get(), leftRegion, lines);
        getSplitLines(node->right.get(), rightRegion, lines);
    }

    static float square(const glm::vec2 &value)
    {
        return glm::dot(value, value);
    }

    std::unique_ptr<KDNode> m_root;
    std::size_t m_size = 0;
};
