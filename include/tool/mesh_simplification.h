#pragma once

#include "tool/mesh.h"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <limits>

#define EPSILON 1e-15

struct Edge {
    int first, second; // vertex id of the edge endpoints (first < second)
    float cost; // cost of the edge
    glm::vec3 optimal_pos;

    bool operator==(const Edge& other) const {
        return first == other.first && second == other.second && std::abs(cost - other.cost) < EPSILON;
    }
    bool operator<(const Edge& other) const {
        if (std::abs(cost - other.cost) < EPSILON) {
            if (first == other.first) {
                return second < other.second;
            } else {
                return first < other.first;
            }
        } else {
          	return cost < other.cost;
        }
    }
};

struct EdgeCompare {
    bool operator()(const Edge& e1, const Edge& e2) const {
        return e1.cost > e2.cost;
    }
};

struct Quadric {
    glm::mat3 A{0.0f};
    glm::vec3 b{0.0f};
    float c{0.0f};

    Quadric& operator+=(const Quadric& q) {
        A += q.A;
        b += q.b;
        c += q.c;
        return *this;
    }

    Quadric operator+(const Quadric& q) const {
        return Quadric{A + q.A, b + q.b, c + q.c};
    }
};

long long get_edge_key(const Edge& edge) {
    return ((long long)edge.first << 32) | edge.second;
}

std::vector<Edge> build_initial_edges(const std::vector<glm::tvec3<int>>& faces) {
    std::unordered_set<long long> edge_set;
    std::vector<Edge> edges;
    edges.reserve(faces.size() * 3 / 2);

    for (auto& face : faces) {
        for (int i = 0; i < 3; ++i) {
            Edge edge{};
            edge.first = face[i], edge.second = face[(i + 1) % 3];
            if (edge.first > edge.second) {
                std::swap(edge.first, edge.second);
            }
            long long key = get_edge_key(edge);
            if (edge_set.find(key) != edge_set.end()) continue;

            edge_set.insert(key);
            edges.emplace_back(edge);
        }
    }

    return edges;
}

float evaluate_quadric(const Quadric& q, const glm::vec3& v) {
    // Q(v) = v^T A v - 2 b^T v + c
    return glm::dot(v, q.A * v) - 2 * glm::dot(q.b, v) + q.c;
}

void compute_edge_cost(Edge& edge, 
    const std::vector<glm::vec3>& vertices,
    const std::vector<Quadric>& quadric_of_vertices) {

    int v1 = edge.first, v2 = edge.second;
    Quadric q = quadric_of_vertices[v1] + quadric_of_vertices[v2];

    float det = glm::determinant(q.A);
    glm::vec3 optimal_pos;
    if (std::abs(det) > 1e-6) {
        optimal_pos = glm::inverse(q.A) * q.b;
        edge.optimal_pos = optimal_pos;
        edge.cost = evaluate_quadric(q, optimal_pos);
    } else {
        glm::vec3 candidates[3] = {
            vertices[v1],
            vertices[v2],
            (vertices[v1] + vertices[v2]) * 0.5f
        };
        float min_cost = std::numeric_limits<float>::max();
        for (const auto& cand : candidates) {
            float cost = evaluate_quadric(q, cand);
            if (cost < min_cost) {
                min_cost = cost;
                optimal_pos = cand;
            }
        }
        edge.cost = min_cost;
        edge.optimal_pos = optimal_pos;
    }
}

void reorder(int& a, int& b, int& c, int v1, int v2) {
    if (a == v1) {
        if (c == v2) std::swap(b, c);
        return;
    }
    if (b == v1) {
        std::swap(a, b);
        if (c == v2) std::swap(a, c);
        return;
    }
    if (c == v1) {
        std::swap(c, a);
        if (a == v2) std::swap(a, b);
        return;
    }
}

Quadric compute_face_quadric(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
    float length = glm::length(normal);
    if (length < 1e-9f) {
        return Quadric{};
    }
    normal /= length;
    float distance = glm::dot(p0, normal);
    glm::mat3 A = glm::outerProduct(normal, normal);
    glm::vec3 b = distance * normal;
    float c = distance * distance;
    return Quadric{A, b, c};
}

bool is_degenerate_face(const glm::tvec3<int>& face) {
    return face[0] == face[1] || face[1] == face[2] || face[2] == face[0];
}

void replace_vertex_in_face(glm::tvec3<int>& face, int old_v, int new_v) {
    for (int i = 0; i < 3; ++i) {
        if (face[i] == old_v) {
            face[i] = new_v;
        }
    }
}

Mesh mesh_simplification(const std::vector<glm::vec3>& _vertices, 
    const std::vector<glm::tvec3<int>>& _faces, float ratio) {
    
    std::vector<glm::vec3> vertices = _vertices;
    std::vector<glm::tvec3<int>> faces = _faces;
    
    std::vector<std::unordered_set<int>> faces_of_vertices(vertices.size());
    std::vector<std::unordered_set<int>> vertex_neighbors(vertices.size());
    std::vector<Quadric> quadric_of_faces(faces.size());
    std::vector<Quadric> quadric_of_vertices(vertices.size());
    std::vector<bool> faces_deleted(faces.size(), false);
    std::vector<bool> vertices_deleted(vertices.size(), false);

    for (int i = 0; i < faces.size(); ++i) {
        auto& face = faces[i];
        faces_of_vertices[face[0]].insert(i);
        faces_of_vertices[face[1]].insert(i);
        faces_of_vertices[face[2]].insert(i);

        vertex_neighbors[face[0]].insert(face[1]);
        vertex_neighbors[face[0]].insert(face[2]);
        vertex_neighbors[face[1]].insert(face[0]);
        vertex_neighbors[face[1]].insert(face[2]);
        vertex_neighbors[face[2]].insert(face[0]);
        vertex_neighbors[face[2]].insert(face[1]);

        quadric_of_faces[i] = compute_face_quadric(
            vertices[face[0]], vertices[face[1]], vertices[face[2]]);
    }

    for (int i = 0; i < vertices.size(); ++i) {
        if (faces_of_vertices[i].empty()) {
            vertices_deleted[i] = true;
        }
        Quadric sum{};
        for (int face_idx : faces_of_vertices[i]) {
            sum += quadric_of_faces[face_idx];
        }
        quadric_of_vertices[i] = sum;
    }

    std::vector<Edge> edges = build_initial_edges(faces);
    std::priority_queue<Edge, std::vector<Edge>, EdgeCompare> heap;
    for (auto& edge : edges) {
        compute_edge_cost(edge, vertices, quadric_of_vertices);
        heap.push(edge);
    }

    int face_count = static_cast<int>(faces.size());
    ratio = std::clamp(ratio, 0.0f, 1.0f);
    int target_face_count = static_cast<int>(faces.size() * ratio);

    int stale_pop_count = 0;
    while (face_count > target_face_count && !heap.empty()) {
        Edge edge = heap.top();
        heap.pop();

        if (vertices_deleted[edge.first] || vertices_deleted[edge.second]) {
            continue;
        }
        if (vertex_neighbors[edge.first].count(edge.second) == 0) {
            continue;
        }

        //lazy delete: edge data in edges may not be latest
        Edge refreshed = edge;
        compute_edge_cost(refreshed, vertices, quadric_of_vertices);
        if (std::abs(refreshed.cost - edge.cost) > 1e-6f) {
            heap.push(refreshed);
            continue;
        }
        edge = refreshed;

        int v_keep = edge.first;
        int v_remove = edge.second;

        std::unordered_set<int> old_neighbors = vertex_neighbors[v_keep];
        old_neighbors.insert(vertex_neighbors[v_remove].begin(), vertex_neighbors[v_remove].end());

        vertices[v_keep] = edge.optimal_pos;
        vertices_deleted[v_remove] = true;
        // Classic QEM: preserve and accumulate the quadrics of the original
        // planes instead of refitting planes after every vertex movement.
        quadric_of_vertices[v_keep] = quadric_of_vertices[v_keep] + quadric_of_vertices[v_remove];

        std::vector<int> changed_faces;

        for (int face_idx : faces_of_vertices[v_remove]) {
            if (faces_deleted[face_idx]) {
                continue;
            }
            auto& face = faces[face_idx];
            if (face[0] == v_keep || face[1] == v_keep || face[2] == v_keep) {
                faces_deleted[face_idx] = true;
                face_count--;
                for (int j = 0; j < 3; ++j) {
                    int vid = face[j];
                    if (vid != v_remove) {
                        faces_of_vertices[vid].erase(face_idx);
                    }
                }
            } else {
                replace_vertex_in_face(face, v_remove, v_keep);
                faces_of_vertices[v_keep].insert(face_idx);
                changed_faces.push_back(face_idx);
            }
        }
        faces_of_vertices[v_remove].clear();

        for (int face_idx : changed_faces) {
            if (faces_deleted[face_idx]) {
                continue;
            }
            auto& face = faces[face_idx];
            if (is_degenerate_face(face)) {
                faces_deleted[face_idx] = true;
                face_count--;
                for (int j = 0; j < 3; ++j) {
                    faces_of_vertices[face[j]].erase(face_idx);
                }
                continue;
            }
        }

        // Rebuild the surviving vertex's one-ring from live faces. This also
        // removes edges that disappeared together with the collapsed faces.
        for (int neighbor : old_neighbors) {
            vertex_neighbors[neighbor].erase(v_keep);
            vertex_neighbors[neighbor].erase(v_remove);
        }
        vertex_neighbors[v_keep].clear();
        vertex_neighbors[v_remove].clear();
        for (int face_idx : faces_of_vertices[v_keep]) {
            if (faces_deleted[face_idx]) continue;
            const auto& face = faces[face_idx];
            for (int i = 0; i < 3; ++i) {
                int neighbor = face[i];
                if (neighbor == v_keep || vertices_deleted[neighbor]) continue;
                vertex_neighbors[v_keep].insert(neighbor);
                vertex_neighbors[neighbor].insert(v_keep);
            }
        }
        for (int vid : old_neighbors) {
            if (vid != v_keep && !vertices_deleted[vid] && faces_of_vertices[vid].empty()) {
                vertices_deleted[vid] = true;
                vertex_neighbors[vid].clear();
            }
        }

        for (int neighbor : vertex_neighbors[v_keep]) {
            if (vertices_deleted[neighbor]) {
                continue;
            }
            Edge updated_edge;
            updated_edge.first = std::min(v_keep, neighbor);
            updated_edge.second = std::max(v_keep, neighbor);
            compute_edge_cost(updated_edge, vertices, quadric_of_vertices);
            heap.push(updated_edge);
        }
    }

    std::vector<int> remap(vertices.size(), -1);
    int new_vert_count = 0;
    for (int i = 0; i < vertices.size(); ++i) {
        if (!vertices_deleted[i]) {
            remap[i] = new_vert_count++;
        }
    }

    std::vector<Vertex> out_vertices(new_vert_count);
    for (int i = 0; i < vertices.size(); ++i) {
        if (remap[i] >= 0) {
            Vertex vert{};
            vert.Position = vertices[i];
            vert.Normal = glm::vec3(0.0f);
            vert.TexCoords = glm::vec2(0.0f);
            vert.Tangent = glm::vec3(0.0f);
            vert.Bitangent = glm::vec3(0.0f);
            out_vertices[remap[i]] = vert;
        }
    }

    std::vector<unsigned int> out_indices;
    out_indices.reserve(face_count * 3);
    for (int i = 0; i < faces.size(); ++i) {
        if (faces_deleted[i]) {
            continue;
        }
        auto face = faces[i];
        face[0] = remap[face[0]];
        face[1] = remap[face[1]];
        face[2] = remap[face[2]];
        if (!is_degenerate_face(face)) {
            out_indices.push_back(static_cast<unsigned int>(face[0]));
            out_indices.push_back(static_cast<unsigned int>(face[1]));
            out_indices.push_back(static_cast<unsigned int>(face[2]));
            glm::vec3 p0 = out_vertices[face[0]].Position;
            glm::vec3 p1 = out_vertices[face[1]].Position;
            glm::vec3 p2 = out_vertices[face[2]].Position;
            glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
            if (glm::length(normal) > 1e-9f) {
                // normal = glm::normalize(normal); 
                out_vertices[face[0]].Normal += normal;
                out_vertices[face[1]].Normal += normal;
                out_vertices[face[2]].Normal += normal;
            }
        }
    }

    for (auto& vert : out_vertices) {
        if (glm::length(vert.Normal) > 1e-9f) {
            vert.Normal = glm::normalize(vert.Normal);
        } else {
            vert.Normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    return Mesh(out_vertices, out_indices, std::vector<Texture>());
}

inline Mesh Mesh::simplify(const Mesh& mesh, float ratio) {
    std::vector<glm::vec3> positions;
    std::vector<glm::tvec3<int>> faces;
    positions.reserve(mesh.vertices.size());
    faces.reserve(mesh.indices.size() / 3);

    for (const auto& vertex : mesh.vertices) {
        positions.push_back(vertex.Position);
    }
    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
        faces.emplace_back(
            static_cast<int>(mesh.indices[i]),
            static_cast<int>(mesh.indices[i + 1]),
            static_cast<int>(mesh.indices[i + 2]));
    }

    return mesh_simplification(positions, faces, ratio);
}

inline std::vector<glm::vec3> generate_normals(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::tvec3<int>>& faces)
{
    std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));
    for (const auto& face : faces) {
        const glm::vec3& p0 = positions[face[0]];
        const glm::vec3& p1 = positions[face[1]];
        const glm::vec3& p2 = positions[face[2]];
        glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
        normals[face[0]] += normal;
        normals[face[1]] += normal;
        normals[face[2]] += normal;
    }
    for (auto& n : normals) {
        float len = glm::length(n);
        if (len > 1e-9f)
            n /= len;
        else
            n = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return normals;
}

inline void parse_face_vertex(const std::string& token, int& vidx, int& tidx, int& nidx) {
    vidx = 0; tidx = 0; nidx = 0;
    size_t firstSlash  = token.find('/');
    if (firstSlash == std::string::npos) {
        vidx = std::stoi(token);
        return;
    }
    vidx = std::stoi(token.substr(0, firstSlash));
    size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos) {
        // format: v/vt
        tidx = std::stoi(token.substr(firstSlash + 1));
    } else if (secondSlash == firstSlash + 1) {
        // format: v//vn
        nidx = std::stoi(token.substr(secondSlash + 1));
    } else {
        // format: v/vt/vn
        tidx = std::stoi(token.substr(firstSlash + 1, secondSlash - firstSlash - 1));
        nidx = std::stoi(token.substr(secondSlash + 1));
    }
}

inline Mesh Mesh::loadOBJ(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[E] Failed to open OBJ: " << path << std::endl;
        return Mesh({}, {}, {});
    }

    auto size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::string content(size, '\0');
    file.read(&content[0], static_cast<std::streamsize>(size));
    file.close();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> normals;
    std::vector<glm::tvec3<int>> faces;

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string id;
        ss >> id;

        if (id == "v") {
            glm::vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (id == "vn") {
            glm::vec3 norm;
            ss >> norm.x >> norm.y >> norm.z;
            normals.push_back(norm);
        } else if (id == "vt") {
            glm::vec2 tex;
            ss >> tex.x >> tex.y;
            texcoords.push_back(tex);
        } else if (id == "f") {
            std::vector<std::string> tokens;
            std::string token;
            while (ss >> token) tokens.push_back(token);

            std::vector<int> vids;
            std::vector<int> tids;
            std::vector<int> nids;
            for (const auto& tok : tokens) {
                int vi, ti, ni;
                parse_face_vertex(tok, vi, ti, ni);
                vids.push_back(vi > 0 ? vi - 1 : static_cast<int>(positions.size()) + vi);
                tids.push_back(ti);
                nids.push_back(ni);
            }

            for (size_t i = 1; i + 1 < vids.size(); ++i) {
                faces.push_back(glm::tvec3<int>(vids[0], vids[i], vids[i + 1]));
            }
        }
    }

    if (positions.empty() || faces.empty()) {
        std::cerr << "[E] OBJ has no geometry: " << path << std::endl;
        return Mesh({}, {}, {});
    }

    // center & normalize (same as homework)
    glm::vec3 center(0.0f);
    for (const auto& p : positions) center += p;
    center /= static_cast<float>(positions.size());

    float maxDist = 0.0f;
    for (const auto& p : positions) {
        maxDist = std::max(maxDist, glm::length(p - center));
    }
    float scale = maxDist > 1e-9f ? 1.0f / maxDist : 1.0f;
    for (auto& p : positions) {
        p = (p - center) * scale;
    }

    if (normals.empty()) {
        normals = generate_normals(positions, faces);
    }

    std::vector<Vertex> out_vertices(positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex vert{};
        vert.Position  = positions[i];
        vert.Normal    = normals[i];
        vert.TexCoords = (i < texcoords.size()) ? texcoords[i] : glm::vec2(0.0f);
        vert.Tangent   = glm::vec3(0.0f);
        vert.Bitangent = glm::vec3(0.0f);
        out_vertices[i] = vert;
    }

    std::vector<unsigned int> out_indices;
    out_indices.reserve(faces.size() * 3);
    for (const auto& f : faces) {
        out_indices.push_back(static_cast<unsigned int>(f[0]));
        out_indices.push_back(static_cast<unsigned int>(f[1]));
        out_indices.push_back(static_cast<unsigned int>(f[2]));
    }

    std::cout << "Loaded OBJ (" << path << "): "
              << positions.size() << " vertices, "
              << faces.size() << " faces\n";

    return Mesh(out_vertices, out_indices, std::vector<Texture>());
}
