/*
 * RzPolygon.cpp
 *
 *  Created on: Apr 29, 2010
 *      Author: ross
 */

#include "RzPolygon.h"

RzPolygon::RzPolygon() {
	// TODO Auto-generated constructor stub

}

RzPolygon::~RzPolygon() {
	// TODO Auto-generated destructor stub
}

/**
 * Get the triangles that make up this polygon.
 */
vector<RzTriangle> RzPolygon::getTriangles() {
	vector<RzTriangle> triangles;
	unsigned int vertexIndex;
	unsigned int coordinateIndex;

	for (vertexIndex = 1; vertexIndex < vertices.size() - 1; ++vertexIndex) {
		RzTriangle triangle;
		// the first vertex is the first polygon vertex
		for (coordinateIndex = 0; coordinateIndex < 3; ++coordinateIndex) {
			triangle.vertices[0].coordinates[coordinateIndex] = vertices[0].coordinates[coordinateIndex];
		}
		// second triangle vertex
		for (coordinateIndex = 0; coordinateIndex < 3; ++coordinateIndex) {
			triangle.vertices[1].coordinates[coordinateIndex] = vertices[vertexIndex].coordinates[coordinateIndex];
		}
		// third triangle vertex
		for (coordinateIndex = 0; coordinateIndex < 3; ++coordinateIndex) {
			triangle.vertices[2].coordinates[coordinateIndex] = vertices[vertexIndex + 1].coordinates[coordinateIndex];
		}

		triangles.push_back(triangle);
	}

	return triangles;
}
