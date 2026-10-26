#pragma once

#include "spatial/Geometry2D.h"
#include <algorithm>
#include <vector>

// Learning version: ObjectId is the object's stable index in the scene vector.
// Do not reorder or erase scene objects without rebuilding this index.
class BruteForce2D
{
public:
    void insert(ObjectId id)
    {
        objectIds.push_back(id);
    }

    void clear()
    {
        objectIds.clear();
    }

    void remove(ObjectId id)
    {
        objectIds.erase(std::remove(objectIds.begin(), objectIds.end(), id),
                        objectIds.end());
    }

    std::vector<ObjectId> queryRange(
        const AABB2 &range,
        const std::vector<Object2D> &objects) const
    {
        std::vector<ObjectId> result;
        result.reserve(objectIds.size());

        for (ObjectId id : objectIds)
        {
            if (id < objects.size() && range.intersects(objects[id].bounds))
                result.push_back(id);
        }

        return result;
    }

private:
    std::vector<ObjectId> objectIds;
};
