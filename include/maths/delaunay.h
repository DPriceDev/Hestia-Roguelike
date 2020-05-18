//
// Created by David Price on 16/05/2020.
//

#ifndef HESTIA_ROGUELIKE_DEPENDS_HGE_MATHS_DELAUNAY_H
#define HESTIA_ROGUELIKE_DEPENDS_HGE_MATHS_DELAUNAY_H

#include <cmath>
#include <vector>
#include <algorithm>

#include <maths/maths_types.h>
#include <util/logger.h>

#include "polygon.h"
#include "triangle.h"
#include "edge.h"
#include "vertex.h"

#include "triangulation.h"

/* Function Declarations */
static void removeSuperTriangle(Triangulation &triangulation);
static Vertex getFurthestPointOfVertices(const std::vector<Vertex> &vertices);
static void createSuperTriangleFromFurthestPoint(Triangulation &triangulation, Vertex & furthest);

static void getSharedAndNonSharedEdgesOfTriangles(const std::vector<Triangle*> &triangles,
                                      std::vector<Edge*> &edges,
                                      Polygon &polygon);

static void createTrianglesFromNewPoint(Triangulation &triangulation,
                                        const Vertex &vert,
                                        const Polygon &polygon);

/**
 *
 * @param points
 * @return
 */
static Triangulation delaunayTriangulationFromPoints(const std::vector<HGE::Vector2f>& points) {

    auto triangulation = Triangulation();

    auto vertices = std::vector<Vertex>();
    std::transform(points.begin(), points.end(), std::back_inserter(vertices), [] (const auto & point) {
        return Vertex(point);
    });

    auto radius = getFurthestPointOfVertices(vertices);
    createSuperTriangleFromFurthestPoint(triangulation, radius);

    for(const auto & vert: vertices) {
        auto badTriangles = std::vector<Triangle*>();

        for(auto & triangle : triangulation.mTriangles) {
            if(HGE::isPointInACircle(vert.mPosition, triangle->mCircumcenter, triangle->mCircumradius)) {
                badTriangles.push_back(triangle.get());
            }
        }

        auto polygon = Polygon();
        auto badEdges = std::vector<Edge*>();
        getSharedAndNonSharedEdgesOfTriangles(badTriangles, badEdges, polygon);
        triangulation.deleteTrianglesAndEdges(badTriangles, badEdges);
        createTrianglesFromNewPoint(triangulation, vert, polygon);
    }

    removeSuperTriangle(triangulation);

    return std::move(triangulation);
}


/** retrieve and edge from a pair of vertices. */
static std::optional<Edge*> getEdgeFromVertices(const std::vector<std::unique_ptr<Edge>> &edges,
                                                const Vertex* a,
                                                const Vertex* b) {
    auto it = std::find_if(edges.begin(), edges.end(), [a, b] (auto & edge) {
       return (edge->a() == a && edge->b() == b) || (edge->b() == a && edge->a() == b);
    });
    if(it != edges.end()) {
        return it->get();
    } else {
        return std::nullopt;
    }
}

/** get the furthest point from the origin, in a set of vertices */
static Vertex getFurthestPointOfVertices(const std::vector<Vertex> &vertices) {
    return *std::max_element(vertices.begin(), vertices.end(), [] (auto & a, auto & b) {
        return a.mMagnitude < b.mMagnitude;
    });
}

/** create a super triangle, bigger than the furthest point. */
static void createSuperTriangleFromFurthestPoint(Triangulation &triangulation, Vertex & furthest) {
    constexpr static float sSuperTriangleOffset = 30.0;
    const static double adjacent = sqrt(3);
    const static double height = adjacent/2;
    auto radius = furthest.mMagnitude + sSuperTriangleOffset;

    auto vertA = triangulation.mVertices.emplace_back(
            std::make_unique<Vertex>(-(radius * adjacent), -radius)).get();
    auto vertB = triangulation.mVertices.emplace_back(
            std::make_unique<Vertex>(radius * adjacent, -radius)).get();
    auto vertC = triangulation.mVertices.emplace_back(
            std::make_unique<Vertex>( 0.0f , ((radius * 2) + (radius * adjacent)) * 0.5)).get();

    auto edgeAB = triangulation.mEdges.emplace_back(
            std::make_unique<Edge>(vertA, vertB)).get();
    auto edgeBC = triangulation.mEdges.emplace_back(
            std::make_unique<Edge>(vertB, vertC)).get();
    auto edgeCA = triangulation.mEdges.emplace_back(
            std::make_unique<Edge>(vertC, vertA)).get();

    triangulation.mTriangles.emplace_back(
            std::make_unique<Triangle>(vertA, vertB, vertC, edgeAB, edgeBC, edgeCA));
}

/** remove the super triangle and attached trangles, edges and vertices. */
// todo: refactor to less duplication
static void removeSuperTriangle(Triangulation &triangulation) {

    auto triangles = &triangulation.mTriangles;
    auto edges = &triangulation.mEdges;
    auto vertices = &triangulation.mVertices;
    triangles->erase(std::remove_if(triangles->begin(), triangles->end(), [&] (const auto & triangle) {
        return triangle->a() == vertices->at(0).get()
               || triangle->a() == vertices->at(1).get()
               || triangle->a() == vertices->at(2).get()
               || triangle->b() == vertices->at(0).get()
               || triangle->b() == vertices->at(1).get()
               || triangle->b() == vertices->at(2).get()
               || triangle->c() == vertices->at(0).get()
               || triangle->c() == vertices->at(1).get()
               || triangle->c() == vertices->at(2).get();
    }), triangles->end());

    /* get all edges linked to first 3 verts and delete them */
    edges->erase(std::remove_if(edges->begin(), edges->end(), [&] (const auto & edge) {
        return edge->a() == vertices->at(0).get()
               || edge->a() == vertices->at(1).get()
               || edge->a() == vertices->at(2).get()
               || edge->b() == vertices->at(0).get()
               || edge->b() == vertices->at(1).get()
               || edge->b() == vertices->at(2).get();
    }), edges->end());

    /* delete first 3 verts */
    vertices->erase(vertices->begin() + 2);
    vertices->erase(vertices->begin() + 1);
    vertices->erase(vertices->begin() + 0);
}

/** adds non shared edges to a polygon, and shared edges of triangles in an edge array. */
// todo: refactor to stl
static void getSharedAndNonSharedEdgesOfTriangles(const std::vector<Triangle*> &triangles,
                                                  std::vector<Edge*> &edges,
                                                  Polygon &polygon) {
    for(auto & triangle : triangles) {
        for(auto & edge : triangle->mEdges) {
            bool isNotShared = true;
            for(auto & badTriangle : triangles) {
                if(badTriangle != triangle) {
                    if(badTriangle->mEdges[0] == edge ||
                       badTriangle->mEdges[1] == edge ||
                       badTriangle->mEdges[2] == edge) {
                        isNotShared = false;
                    }

                }
            }

            if(isNotShared) {
                polygon.edges.push_back(edge);
            } else {
                auto it = std::find(edges.begin(), edges.end(), edge);
                if(it == edges.end()) {
                    edges.push_back(edge);
                }
            }
        }
    }
}

/** Constructs triangles from a point and a polygon. */
static void createTrianglesFromNewPoint(Triangulation &triangulation,
                                 const Vertex &vert,
                                 const Polygon &polygon) {

    auto newVert = triangulation.mVertices.emplace_back(
            std::make_unique<Vertex>(vert)).get();

    for(auto & edge : polygon.edges) {

        Edge* edgeNA;
        Edge* edgeBN;
        auto existingEdgeNA = getEdgeFromVertices(triangulation.mEdges, newVert, edge->a());
        auto existingEdgeBN = getEdgeFromVertices(triangulation.mEdges, edge->b(), newVert);

        if(existingEdgeNA.has_value()) {
            edgeNA = existingEdgeNA.value();
        } else {
            edgeNA = triangulation.mEdges.emplace_back(
                    std::make_unique<Edge>(newVert, edge->a())).get();
        }

        if(existingEdgeBN.has_value()) {
            edgeBN = existingEdgeBN.value();
        } else {
            edgeBN = triangulation.mEdges.emplace_back(
                    std::make_unique<Edge>(edge->b(), newVert)).get();
        }

        triangulation.mTriangles.emplace_back(
                std::make_unique<Triangle>(newVert, edge->a(), edge->b(), edgeNA, edge, edgeBN));
    }
}

#endif //HESTIA_ROGUELIKE_DEPENDS_HGE_MATHS_DELAUNAY_H