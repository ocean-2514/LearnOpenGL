#pragma once

#include "spatial/Geometry2D.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

// A finite, dense grid whose world-space origin is (0, 0).
// AABBs use closed bounds, so an object touching a cell boundary belongs to
// both adjacent cells. ObjectId is assumed to index the supplied object array.
class UniformGrid2D
{
public:
    UniformGrid2D(int worldWidth, int worldHeight, int gridSize = 50)
        : worldWidth_(worldWidth), worldHeight_(worldHeight), gridSize_(gridSize)
    {
        if (worldWidth_ <= 0 || worldHeight_ <= 0 || gridSize_ <= 0)
            throw std::invalid_argument("UniformGrid2D dimensions and gridSize must be positive");

        columns_ = (worldWidth_ + gridSize_ - 1) / gridSize_;
        rows_ = (worldHeight_ + gridSize_ - 1) / gridSize_;
        grid_.resize(static_cast<std::size_t>(rows_) * static_cast<std::size_t>(columns_));
    }

    void clear()
    {
        for (auto &cell : grid_)
            cell.clear();
    }

    //need to make sure id is not inserted before
    void insert(ObjectId id, const AABB2 &bounds)
    {
        CellRange cells;
        if (!coveredCells(bounds, cells))
            return;

        forEachCell(cells, [this, id](int row, int column) {
            insertIntoCell(id, cellIndex(row, column));
        });

        ensureStampCapacity(id);
    }

    void remove(ObjectId id, const AABB2 &bounds)
    {
        CellRange cells;
        if (!coveredCells(bounds, cells))
            return;

        forEachCell(cells, [this, id](int row, int column) {
            removeFromCell(id, cellIndex(row, column));
        });
    }

    void update(ObjectId id, const AABB2 &oldBounds, const AABB2 &newBounds)
    {
        ensureStampCapacity(id);

        CellRange oldCells;
        CellRange newCells;
        const bool hadOldCells = coveredCells(oldBounds, oldCells);
        const bool hasNewCells = coveredCells(newBounds, newCells);

        if (hadOldCells)
        {
            forEachCell(oldCells, [this, id, hasNewCells, &newCells](int row, int column) {
                if (!hasNewCells || !newCells.contains(row, column))
                    removeFromCell(id, cellIndex(row, column));
            });
        }

        if (hasNewCells)
        {
            forEachCell(newCells, [this, id, hadOldCells, &oldCells](int row, int column) {
                if (!hadOldCells || !oldCells.contains(row, column))
                    insertIntoCell(id, cellIndex(row, column));
            });
        }
    }

    std::vector<ObjectId> queryRange(
        const AABB2 &range,
        const std::vector<Object2D> &objects) const
    {
        if (queryStamp_ == std::numeric_limits<std::uint32_t>::max())
        {
            std::fill(visitedStamp_.begin(), visitedStamp_.end(), 0);
            queryStamp_ = 1;
        }
        else
            ++queryStamp_;

        std::vector<ObjectId> result;
        CellRange cells;
        if (!coveredCells(range, cells))
            return result;

        forEachCell(cells, [this, &range, &objects, &result](int row, int column) {
            const auto &cell = grid_[cellIndex(row, column)];
            for (ObjectId id : cell)
            {
                if (id >= objects.size() || id >= visitedStamp_.size() ||
                    visitedStamp_[id] == queryStamp_)
                    continue;

                visitedStamp_[id] = queryStamp_;
                if (range.intersects(objects[id].bounds))
                    result.push_back(id);
            }
        });
        return result;
    }

    int rows() const { return rows_; }
    int columns() const { return columns_; }
    int gridSize() const { return gridSize_; }

    const std::vector<ObjectId> &cell(int row, int column) const
    {
        if (row < 0 || row >= rows_ || column < 0 || column >= columns_)
            throw std::out_of_range("UniformGrid2D cell coordinates are out of range");
        return grid_[cellIndex(row, column)];
    }

private:
    struct CellRange
    {
        int minColumn = 0;
        int maxColumn = -1;
        int minRow = 0;
        int maxRow = -1;

        bool contains(int row, int column) const
        {
            return row >= minRow && row <= maxRow &&
                   column >= minColumn && column <= maxColumn;
        }
    };

    bool coveredCells(const AABB2 &bounds, CellRange &result) const
    {
        if (bounds.min.x > bounds.max.x || bounds.min.y > bounds.max.y)
            return false;

        if (bounds.max.x < 0.0f || bounds.max.y < 0.0f ||
            bounds.min.x > static_cast<float>(worldWidth_) ||
            bounds.min.y > static_cast<float>(worldHeight_))
            return false;

        const auto toCell = [this](float coordinate) {
            return static_cast<int>(std::floor(coordinate / static_cast<float>(gridSize_)));
        };

        result.minColumn = std::clamp(toCell(bounds.min.x), 0, columns_ - 1);
        result.maxColumn = std::clamp(toCell(bounds.max.x), 0, columns_ - 1);
        result.minRow = std::clamp(toCell(bounds.min.y), 0, rows_ - 1);
        result.maxRow = std::clamp(toCell(bounds.max.y), 0, rows_ - 1);
        return true;
    }

    std::size_t cellIndex(int row, int column) const
    {
        return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns_) +
               static_cast<std::size_t>(column);
    }

    void insertIntoCell(ObjectId id, std::size_t index)
    {
        auto &cell = grid_[index];
        // if (std::find(cell.begin(), cell.end(), id) == cell.end())
        //     cell.push_back(id);
        cell.push_back(id);
    }

    void ensureStampCapacity(ObjectId id)
    {
        const std::size_t requiredSize = static_cast<std::size_t>(id) + 1;
        if (visitedStamp_.size() < requiredSize)
            visitedStamp_.resize(requiredSize, 0);
    }

    void removeFromCell(ObjectId id, std::size_t index)
    {
        auto &cell = grid_[index];
        const auto it = std::find(cell.begin(), cell.end(), id);
        if (it == cell.end())
            return;

        *it = cell.back();
        cell.pop_back();
    }

    template <typename Function>
    void forEachCell(const CellRange &cells, Function &&function) const
    {
        for (int row = cells.minRow; row <= cells.maxRow; ++row)
        {
            for (int column = cells.minColumn; column <= cells.maxColumn; ++column)
                function(row, column);
        }
    }

    int worldWidth_ = 0;
    int worldHeight_ = 0;
    int gridSize_ = 0;
    int rows_ = 0;
    int columns_ = 0;
    mutable std::uint32_t queryStamp_ = 0;
    std::vector<std::vector<ObjectId>> grid_;
    mutable std::vector<std::uint32_t> visitedStamp_;
};
