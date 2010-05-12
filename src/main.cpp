#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "CS455FileParser.h"
#include "RzPolygonGroupCollection.h"
#include "RzPolygonGroup.h"
#include "RzPolygon.h"
#include "RzTriangle.h"
#include "RzColor3f.h"
#include "RzVertex3f.h"
#include "Debugger.h"

using namespace std;

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 700

// Keydown booleans
bool key[321];
float zBuffer[WINDOW_WIDTH][WINDOW_HEIGHT];
bool zBufferSet[WINDOW_WIDTH][WINDOW_HEIGHT];
RzColor3f colorBuffer[WINDOW_WIDTH][WINDOW_HEIGHT];

//int numPointPlots = 0;
//int maxPointPlots = 0;
int numScanLines = 0;
int maxScanLines = 1000000000;
//int maxScanLines = 0;

RzPolygonGroupCollection *collection;
RzPolygonGroupCollection *perspectified;

// Process pending events
bool events() {
	SDL_Event event;
	if (SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_KEYDOWN : key[event.key.keysym.sym] = true; break;
		case SDL_KEYUP   : key[event.key.keysym.sym] = false; break;
		case SDL_QUIT    : return false; break;
		}
	}
	return true;
}

float translateScaleX(float boundingXMin, float boundingXMax, float boundingYMin, float boundingYMax, float x) {
	float ret;

	float boundingWidth = boundingXMax - boundingXMin;
	float scaleFactor = ((float)WINDOW_WIDTH / boundingWidth);

	ret = (x - boundingXMin) * scaleFactor;

	return ret;
}

float translateScaleY(float boundingXMin, float boundingXMax, float boundingYMin, float boundingYMax, float y) {
	float ret;

	float boundingWidth = boundingXMax - boundingXMin;
	float scaleFactor = ((float)WINDOW_WIDTH / boundingWidth);

	ret = (y - boundingYMax) * -1 * scaleFactor;
	//ret = (y - boundingYMin) * scaleFactor;

	return ret;
}

void swapPointers(void **v1, void **v2) {
	void *tmp_v = *v1;
	*v1 = *v2;
	*v2 = tmp_v;
}

int round1f(float f) {
	return floor(0.5 + f);
}

int intify(float f) {
	//return floor(f);
	return round1f(f);
	//return ceil(f);
}

float computeSlope2d(RzVertex3f *v1, RzVertex3f *v2) {
	float denominator = (float)(intify(v1->getX()) - intify(v2->getX()));
	float numerator = (float)(intify(v1->getY()) - intify(v2->getY()));

	float slope;

	slope = numerator / denominator;

	/*
	if (numerator == 0) {
		if (denominator == 0) {
			slope = 0;
		} else {
			if (denominator < 0) {
				slope = -slope;
			}
		}
	}
	*/
	/*
	if (denominator == 0) {
		stringstream ss;
		ss << "denominator 0: slope: " << slope;
		Debugger::getInstance().print(ss.str());
	}
	*/
	return slope;
}

float computeDelta2d(RzVertex3f *v1, RzVertex3f *v2) {
	float delta = 1.0 / computeSlope2d(v1, v2);
	return delta;
}

/*
bool compareVertexYs(RzVertex3f v1, RzVertex3f v2) {
	return v1.getY() < v2.getY();
}
*/

void drawPoint2iForReal(int x, int y, float z, RzColor3f *color) {

	/*
	if (numPointPlots >= maxPointPlots) {
		// maxed out
		return;
	} else {
		++numPointPlots;
	}
	*/

	zBufferSet[x][y] = true;
	zBuffer[x][y] = z;
	colorBuffer[x][y] = *color;
	//glVertex2f((float)x, (float)y);
}

void drawPoint2i(int x, int y, float z, RzColor3f *color) {
	if (x >= WINDOW_WIDTH || x < 0 || y >= WINDOW_HEIGHT || y < 0) {
			// out of bounds
		return;
	}

	// check the z buffer
	if (zBufferSet[x][y]) {
		if (z > zBuffer[x][y]) {
			drawPoint2iForReal(x, y, z, color);
		}
	} else {
		drawPoint2iForReal(x, y, z, color);
	}
}

void drawPoint2f(float x, float y, float z, RzColor3f *color) {
	// get buffer coordinates
	//int i_x = round1f(x);
	//int i_y = round1f(y);
	drawPoint2i(intify(x), intify(y), z, color);
}

float interpolateZ(float z1, float z2, float z3, float y1, float ys, float y2, float y3, float xa, float xp, float xb) {
	float za, zb, zp;

	y1 = (float)intify(y1);
	ys = (float)intify(ys);
	y2 = (float)intify(y2);
	y3 = (float)intify(y3);
	xa = (float)intify(xa);
	xp = (float)intify(xp);
	xb = (float)intify(xb);

	za = z1 - (z1 - z2) * (y1 - ys) / (y1 - y2);
	zb = z1 - (z1 - z3) * (y1 - ys) / (y1 - y3);
	zp = zb - (zb - za) * (xb - xp) / (xb - xa);

	return zp;
}

/*
float interpolateZhorizontal(float za, float zb, float xa, float xp, float xb) {
	float zp;

	zp = zb - (zb - za) * (xb - xp) / (xb - xa);
	return zp;
}
*/

void fillUpperPart(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, float leftUpperDelta,
		float rightUpperDelta, RzColor3f *color3f) {

	int yPos = intify(top->getY());
	int endY = intify(mid->getY());
	float xLeft, xRight;
	int xMin, xMax, xPos;

	/*
	int leftmost, rightmost;
	leftmost = intify(mid->getX());
	if (intify(bottom->getX()) < leftmost) {
		leftmost = intify(bottom->getX());
		rightmost = intify(mid->getX());
	} else {
		rightmost = intify(bottom->getX());
	}
	*/

	xLeft = xRight = (float)intify(top->getX());

	/*
	RzColor3f color;
	color.setRed(1.0);
	drawPoint2i(intify(top->getX()), intify(top->getY()), top->getZ(), &color);
	*/

	while (yPos <= endY) {
		// fill in the line
		xMin = intify(xLeft);
		xMax = intify(xRight);

		/*
		if (xMin < leftmost) {
			xMin = leftmost;
		}
		if (xMax > rightmost) {
			xMax = rightmost;
		}
		*/

		if (xMin < xMax) {
			if (numScanLines >= maxScanLines) {
				//return;
			} else {
				++numScanLines;
			}
		}

		xPos = xMin;
		while (xPos < xMax) {

			// draw the point
			drawPoint2i(xPos, yPos,
					interpolateZ(top->getZ(),
							mid->getZ(),
							bottom->getZ(),
							top->getY(),
							(float)yPos,
							mid->getY(),
							bottom->getY(),
							xLeft,
							(float)xPos,
							xRight
							),
							color3f);

			++xPos;
		}

		xLeft += leftUpperDelta;
		xRight += rightUpperDelta;

		++yPos;
	}
}

/*
void fillLine(RzVertex3f *v1, RzVertex3f *v2, RzColor3f *color3f) {
	float slope = computeSlope2d(v1, v2);

}
*/

/*
void fillHorizontalLine(RzVertex3f *v1, RzVertex3f *v2, RzColor3f *color3f) {
	float xLeft, xRight;
	float xPosition;
	float yPosition = v1->getY();
	// fill in the line

	xLeft = v1->getX();
	xRight = v2->getX();

	xPosition = xLeft;
	while (xPosition < intify(xRight)) {
		// draw the point
		drawPoint2f(xPosition, yPosition,
				interpolateZhorizontal(v1->getZ(),
						v2->getZ(),
						xLeft,
						xPosition,
						xRight
						),
						color3f);

		xPosition += 1.0;
	}
}
*/

/*
void fillLowerPart(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, float leftLowerSlope,
		float rightLowerSlope, RzColor3f *color3f) {

	float slopeTopMid, slopeTopBottom, slopeMidBottom;
	slopeTopMid = computeSlope2d(top, mid);
	slopeTopBottom = computeSlope2d(top, bottom);
	slopeMidBottom = computeSlope2d(mid, bottom);

	float deltaTopMid, deltaTopBottom, deltaMidBottom;
	deltaTopMid = 1.0 / slopeTopMid;
	deltaTopBottom = 1.0 / slopeTopBottom;
	deltaMidBottom = 1.0 / slopeMidBottom;

	RzVertex3f *left, *right;

	float leftSlope, rightSlope;

	// slopes can be positive and negative infinity

	// smaller delta is to the right
	if (deltaMidBottom < deltaTopBottom) {
		right = mid;
		left = top;
		leftSlope = slopeTopBottom;
		rightSlope = slopeMidBottom;
	} else {
		right = top;
		left = mid;
		leftSlope = slopeMidBottom;
		rightSlope = slopeTopBottom;
	}

	float bLeft, bRight;

	if (isfinite(leftSlope)) {
		bLeft = bottom->getY() - leftSlope * left->getX();
	}
	if (isfinite(rightSlope)) {
		bRight = bottom->getY() - rightSlope * right->getX();
	}

	int yPos = intify(bottom->getY());
	int yEnd = intify(mid->getY());
	int xPos;
	float xLeft, xRight;
	int xMin, xMax;


	//xMin = xMaxLeft = xRight = (float)intify(bottom->getX());

	//RzColor3f color;
	//color.setRed(1.0);
	//drawPoint2i(intify(bottom->getX()), intify(bottom->getY()), bottom->getZ(), &color);

	while (yPos > yEnd) {
		// fill in the line

		if (isfinite(leftSlope)) {
			xLeft = ((float)yPos - bLeft) / leftSlope;
		} else {
			xLeft = (float)intify(left->getX());
		}

		if (isfinite(rightSlope)) {
			xRight = ((float)yPos - bRight) / rightSlope;
		} else {
			xRight = (float)intify(right->getX());
		}
		xMin = intify(xLeft);
		xMax = intify(xRight);

		if (xMin < xMax) {
			if (numScanLines >= maxScanLines) {
				return;
			} else {
				++numScanLines;
			}
		}

		xPos = xMin;
		while (xPos < xMax) {
			// draw the point
			drawPoint2i(xPos, yPos,
					interpolateZ(bottom->getZ(),
							mid->getZ(),
							top->getZ(),
							bottom->getY(),
							(float)yPos,
							mid->getY(),
							top->getY(),
							xLeft,
							(float)xPos,
							xRight
							),
							color3f);

			++xPos;
		}

		--yPos;
	}
}
*/

void fillFlatTop(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, RzVertex3f *left, RzVertex3f *right,
		float leftSlope, float rightSlope, RzColor3f *color3f) {

	float bLeft, bRight;

	if (isfinite(leftSlope)) {
		bLeft = (float)intify(left->getY()) - leftSlope * (float)intify(left->getX());
	}
	if (isfinite(rightSlope)) {
		bRight = (float)intify(right->getY()) - rightSlope * (float)intify(right->getX());
	}

	int yPos = intify(bottom->getY());
	int yEnd = intify(mid->getY());
	int xPos;
	float xLeft, xRight;
	int xMin, xMax;

	//xMin = xMaxLeft = xRight = (float)intify(bottom->getX());

	//RzColor3f color;
	//color.setRed(1.0);
	//drawPoint2i(intify(bottom->getX()), intify(bottom->getY()), bottom->getZ(), &color);

	while (yPos > yEnd) {
		// fill in the line

		if (isfinite(leftSlope)) {
			xLeft = ((float)yPos - bLeft) / leftSlope;
		} else {
			xLeft = (float)intify(left->getX());
		}

		if (isfinite(rightSlope)) {
			xRight = ((float)yPos - bRight) / rightSlope;
		} else {
			xRight = (float)intify(right->getX());
		}

		xMin = intify(xLeft);
		xMax = intify(xRight);

		if (xMin < xMax) {
			if (numScanLines >= maxScanLines) {
				return;
			} else {
				++numScanLines;
			}
		}

		xPos = xMin;
		while (xPos < xMax) {
			// draw the point
			drawPoint2i(xPos, yPos,
					interpolateZ(bottom->getZ(),
							mid->getZ(),
							top->getZ(),
							bottom->getY(),
							(float)yPos,
							mid->getY(),
							top->getY(),
							xLeft,
							(float)xPos,
							xRight
							),
							color3f);

			++xPos;
		}

		--yPos;
	}
}

void fillFlatBottom(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, RzVertex3f *left, RzVertex3f *right,
		float leftSlope, float rightSlope, RzColor3f *color3f) {

	float bLeft, bRight;

	if (isfinite(leftSlope)) {
		bLeft = (float)intify(left->getY()) - leftSlope * (float)intify(left->getX());
	}
	if (isfinite(rightSlope)) {
		bRight = (float)intify(right->getY()) - rightSlope * (float)intify(right->getX());
	}

	int yPos = intify(top->getY());
	int yEnd = intify(mid->getY());
	int xPos;
	float xLeft, xRight;
	int xMin, xMax;

	//xMin = xMaxLeft = xRight = (float)intify(bottom->getX());

	//RzColor3f color;
	//color.setRed(1.0);
	//drawPoint2i(intify(bottom->getX()), intify(bottom->getY()), bottom->getZ(), &color);

	while (yPos <= yEnd) {
		// fill in the line

		if (isfinite(leftSlope)) {
			xLeft = ((float)yPos - bLeft) / leftSlope;
		} else {
			xLeft = (float)intify(left->getX());
		}

		if (isfinite(rightSlope)) {
			xRight = ((float)yPos - bRight) / rightSlope;
		} else {
			xRight = (float)intify(right->getX());
		}

		xMin = intify(xLeft);
		xMax = intify(xRight);

		if (xMin < xMax) {
			if (numScanLines >= maxScanLines) {
				return;
			} else {
				++numScanLines;
			}
		}

		xPos = xMin;
		while (xPos < xMax) {

			// draw the point
			drawPoint2i(xPos, yPos,
					interpolateZ(top->getZ(),
							mid->getZ(),
							bottom->getZ(),
							top->getY(),
							(float)yPos,
							mid->getY(),
							bottom->getY(),
							xLeft,
							(float)xPos,
							xRight
							),
							color3f);

			++xPos;
		}

		++yPos;
	}
}

void fillLowerPart(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, float leftLowerDelta,
		float rightLowerDelta, RzColor3f *color3f) {

	int yPos = intify(bottom->getY());
	int yEnd = intify(mid->getY());
	float xLeft, xRight;
	int xPos;
	int xMin, xMax;
	xLeft = xRight = (float)intify(bottom->getX());

	/*
	int leftmost, rightmost;
	leftmost = intify(mid->getX());
	if (intify(top->getX()) < leftmost) {
		leftmost = intify(top->getX());
		rightmost = intify(mid->getX());
	} else {
		rightmost = intify(top->getX());
	}
	*/

	//RzColor3f color;
	//color.setRed(1.0);
	//drawPoint2i(intify(bottom->getX()), intify(bottom->getY()), bottom->getZ(), &color);

	while (yPos > yEnd) {
		// fill in the line

		xMin = intify(xLeft);
		xMax = intify(xRight);

		/*
		if (xMin < leftmost) {
			xMin = leftmost;
		}
		if (xMax > rightmost) {
			xMax = rightmost;
		}
		*/

		if (xMin < xMax) {
			if (numScanLines >= maxScanLines) {
				//return;
			} else {
				++numScanLines;
			}
		}

		xPos = xMin;
		while (xPos < xMax) {
			// draw the point
			drawPoint2i(xPos, yPos,
					interpolateZ(bottom->getZ(),
							mid->getZ(),
							top->getZ(),
							bottom->getY(),
							(float)yPos,
							mid->getY(),
							top->getY(),
							xLeft,
							(float)xPos,
							xRight
							),
							color3f);

			++xPos;
		}

		xLeft -= leftLowerDelta;
		xRight -= rightLowerDelta;

		--yPos;
	}
}

void main_loop_function()
{
	unsigned int polygonGroupIndex;
	unsigned int polygonIndex;
	unsigned int triangleIndex;
	unsigned int vertexIndex;
	unsigned int x, y;
	RzPolygonGroup *polygonGroup;
	RzPolygon *polygon;
	RzTriangle *triangle;
	RzVertex3f *vertex3f;
	RzColor3f *color3f;
	//float boundingWidth;
	//float boundingHeight;
	//float scaleFactor;
	stringstream ss;
	RzVertex3f *midVertex, *topVertex, *bottomVertex;
	//int numLogs = 20;
	vector<RzTriangle> triangles;
	float slopeTopMid;
	float slopeTopBottom;
	float slopeMidBottom;

	// figure out which side is left and right
	// compare deltas
	float deltaTopMid;
	float deltaTopBottom;
	float deltaMidBottom;

	bool draw_triangles = false;
	//float angle;
	//float xAdd = 0;
	//float yAdd = 0;
	//float zAdd = 0.0;

	int lastNumScanLines = 0;

	float eye_z = 1.0;
	float near_z = 0.0;
	float far_z = -1000.0;

	while(events())
	{
		//numPointPlots = 0;
		numScanLines = 0;

		// initialize the buffers
		for (x = 0; x < WINDOW_WIDTH; ++x) {
			for (y = 0; y < WINDOW_HEIGHT; ++y) {
				zBufferSet[x][y] = false;
				colorBuffer[x][y].setRed(0.6);
				colorBuffer[x][y].setGreen(0.6);
				colorBuffer[x][y].setBlue(1.0);
			}
		}

		// create the perspectified version of the model
		perspectified = new RzPolygonGroupCollection(*collection);

		/*
		ss.clear();
		ss << "bounding box: (" << boundingXMin << ", " << boundingYMin << "), (" << boundingXMax << ", " << boundingYMax << ")";
		Debugger::getInstance().print(ss.str());
		*/

		//boundingWidth = boundingXMax - boundingXMin;
		//scaleFactor = ((float)WINDOW_WIDTH / boundingWidth);

		/*
		ss.clear();
		ss << "boundingWidth: " << boundingWidth;
		Debugger::getInstance().print(ss.str());
		*/

		/*
		ss.clear();
		ss << "scaleFactor: " << scaleFactor;
		Debugger::getInstance().print(ss.str());
		*/

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);
		//glLoadIdentity();
		//glTranslatef(0,0, -10);
		//glRotatef(angle, 0, 0, 1);
		//glBegin(GL_QUADS);
		//glBegin(GL_POINTS);
		//glBegin(GL_TRIANGLES);
		//glColor3ub(255, 000, 000); glVertex2f(-1,  1);
		//glColor3ub(000, 255, 000); glVertex2f( 1,  1);
		//glColor3ub(000, 000, 255); glVertex2f( 1, -1);
		//glColor3ub(255, 255, 000); glVertex2f(-1, -1);
		/*
		glColor3ub(255, 000, 000); glVertex2f(200,  500);
		glColor3ub(000, 255, 000); glVertex2f( 500,  500);
		glColor3ub(000, 000, 255); glVertex2f( 500, 200);
		glColor3ub(255, 255, 000); glVertex2f(200, 200);
		*/

		for (polygonGroupIndex = 0; polygonGroupIndex < perspectified->polygonGroups.size(); ++polygonGroupIndex) {
			polygonGroup = &perspectified->polygonGroups[polygonGroupIndex];
			color3f = &polygonGroup->color;

			/*
			// set the color
			glColor3f(color3f->getRed(),
					color3f->getGreen(),
					color3f->getBlue());
					*/

			// draw the polygons
			for (polygonIndex = 0; polygonIndex < polygonGroup->polygons.size(); ++polygonIndex) {
				polygon = &polygonGroup->polygons[polygonIndex];

				// convert vertices to perspective
				for (vertexIndex = 0; vertexIndex < polygon->vertices.size(); ++vertexIndex) {
					vertex3f = &polygon->vertices[vertexIndex];

					float big_z = eye_z - vertex3f->getZ();
					float big_x = vertex3f->getX();
					float big_y = vertex3f->getY();
					float little_z = eye_z - near_z;

					float factor = little_z / big_z;

					float new_x = vertex3f->getX() * factor;
					float new_y = vertex3f->getY() * factor;

					new_x = (float)WINDOW_WIDTH / 2.0 + new_x;
					new_y = (float)WINDOW_HEIGHT / 2.0 - new_y;

					vertex3f->setX(new_x);
					vertex3f->setY(new_y);
				}


				triangles = polygon->getTriangles();

				// get the triangles
				for (triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {

						// draw the triangle vertices
					triangle = &triangles[triangleIndex];
					// scan line convert

					// get bottom, mid, top vertices
					bottomVertex = &triangle->vertices[0];
					midVertex = &triangle->vertices[1];
					topVertex = &triangle->vertices[2];

					if (midVertex->getY() > bottomVertex->getY()) {
						swapPointers((void**)&midVertex, (void**)&bottomVertex);
					}
					if (topVertex->getY() > midVertex->getY()) {
						swapPointers((void**)&topVertex, (void**)&midVertex);

						if (midVertex->getY() > bottomVertex->getY()) {
							swapPointers((void**)&midVertex, (void**)&bottomVertex);
						}
					}

					// sort vertices with same y
					if (intify(topVertex->getY()) == intify(bottomVertex->getY())) {
						// perfectly flat line
						// sort by x value
						if (intify(midVertex->getX()) > intify(bottomVertex->getX())) {
							swapPointers((void**)&midVertex, (void**)&bottomVertex);
						}
						if (intify(topVertex->getX()) > intify(midVertex->getX())) {
							swapPointers((void**)&topVertex, (void**)&midVertex);

							if (intify(midVertex->getX()) > intify(bottomVertex->getX())) {
								swapPointers((void**)&midVertex, (void**)&bottomVertex);
							}
						}
					} else if (intify(topVertex->getY()) == intify(midVertex->getY())) {
						// flat on top
						if (intify(topVertex->getX()) > intify(midVertex->getX())) {
							swapPointers((void**)&topVertex, (void**)&midVertex);
						}
					} else if (intify(midVertex->getY()) == intify(bottomVertex->getY())) {
						// flat on bottom
						if (intify(bottomVertex->getX()) > intify(midVertex->getX())) {
							swapPointers((void**)&bottomVertex, (void**)&midVertex);
						}
					}

					if (draw_triangles) {
						glBegin(GL_TRIANGLES);
						glColor3f(1.0, 0, 0);
						glVertex2f(intify(bottomVertex->getX()),
								intify(bottomVertex->getY()));
						glColor3f(0, 1.0, 0);
						glVertex2f(intify(midVertex->getX()),
								intify(midVertex->getY()));
						glColor3f(0, 0, 1.0);
						glVertex2f(intify(topVertex->getX()),
								intify(topVertex->getY()));
						glEnd();
					}

						/*
					if (glGetError() != GL_NO_ERROR) {
						Debugger::getInstance().print("gl error");
					}
					*/

					// calculate the slopes
					slopeTopMid = computeSlope2d(topVertex, midVertex);
					slopeTopBottom = computeSlope2d(topVertex, bottomVertex);
					slopeMidBottom = computeSlope2d(midVertex, bottomVertex);

					RzVertex3f *bottomLeft, *bottomRight, *topLeft, *topRight;

					float bottomLeftSlope, bottomRightSlope, topLeftSlope, topRightSlope;

					if (slopeTopMid == 0) {
							// top and mid are on same y, different x

						if (slopeTopBottom == 0) {
								// all points on same y
							// ignore
						} else if (isnan(slopeTopBottom) || isnan(slopeMidBottom)) {
							// ignore
						} else {
							if (intify(topVertex->getX()) < intify(midVertex->getX())) {
								topLeft = topVertex;
								topRight = midVertex;
								topLeftSlope = slopeTopBottom;
								topRightSlope = slopeMidBottom;
							} else {
								topLeft = midVertex;
								topRight = topVertex;
								topLeftSlope = slopeMidBottom;
								topRightSlope = slopeTopBottom;
							}

							fillFlatTop(topVertex, midVertex, bottomVertex, topLeft, topRight,
									topLeftSlope, topRightSlope, color3f);
						}
					} else if (isnan(slopeTopMid)) {
							// top and mid are the same point
						// ignore
					} else if (!isfinite(slopeTopMid)) {
							// top and mid are the same horizontal

						if (isnan(slopeMidBottom)) {
							// ignore
						} else if (!isfinite(slopeMidBottom)) {
							// ignore
						} else {
							if (intify(bottomVertex->getX()) > intify(midVertex->getX())) {
								bottomLeft = midVertex;
								bottomLeftSlope = slopeTopMid;
								bottomRight = bottomVertex;
								bottomRightSlope = slopeTopBottom;

							} else {
								bottomRight = midVertex;
								bottomLeft = bottomVertex;
								bottomRightSlope = slopeTopMid;
								bottomLeftSlope = slopeTopBottom;
							}

							fillFlatBottom(topVertex, midVertex, bottomVertex, bottomLeft, bottomRight,
									bottomLeftSlope, bottomRightSlope, color3f);

							if (slopeMidBottom == 0) {
								// ignore drawing flat top triangle
							} else {
								if (intify(bottomVertex->getX()) > intify(midVertex->getX())) {
									topLeft = midVertex;
									topLeftSlope = slopeMidBottom;
									topRight = topVertex;
									topRightSlope = slopeTopBottom;

								} else {
									topRight = midVertex;
									topLeft = topVertex;
									topRightSlope = slopeMidBottom;
									topLeftSlope = slopeTopBottom;
								}

								fillFlatTop(topVertex, midVertex, bottomVertex, topLeft, topRight,
										topLeftSlope, topRightSlope, color3f);
							}
						}
					} else {
						// mid is below top but not horizontal or vertical to it
						if (isnan(slopeMidBottom)) {
							// ignore
						} else if (slopeMidBottom == 0) {
							// just fill the flat bottom
							if (intify(bottomVertex->getX()) > intify(midVertex->getX())) {
								bottomLeft = midVertex;
								bottomLeftSlope = slopeTopMid;
								bottomRight = bottomVertex;
								bottomRightSlope = slopeTopBottom;

							} else {
								bottomRight = midVertex;
								bottomLeft = bottomVertex;
								bottomRightSlope = slopeTopMid;
								bottomLeftSlope = slopeTopBottom;
							}

							fillFlatBottom(topVertex, midVertex, bottomVertex, bottomLeft, bottomRight,
									bottomLeftSlope, bottomRightSlope, color3f);
						} else {
							// figure out which point goes left and which goes right from top

							deltaTopMid = 1.0 / slopeTopMid;
							deltaTopBottom = 1.0 / slopeTopBottom;

							bool display = false;
							if ((numScanLines + 1) == maxScanLines && lastNumScanLines != numScanLines) {
								ss.clear();
								display = true;
								ss << "deltaTopMid: " << deltaTopMid << " deltaTopBottom: " << deltaTopBottom;
								ss << " top: " << intify(topVertex->getX()) << ", " << intify(topVertex->getY());
								ss << " mid: " << intify(midVertex->getX()) << ", " << intify(midVertex->getY());
								ss << " bottom: " << intify(bottomVertex->getX()) << ", " << intify(bottomVertex->getY());
								ss << endl;
								Debugger::getInstance().print(ss.str());
								lastNumScanLines = numScanLines;
							}

							if (deltaTopMid < deltaTopBottom) {
								if (display) {
									ss.clear();
									ss << "deltaTopMid < deltaTopBottom" << endl;
									Debugger::getInstance().print(ss.str());
								}
								// top to mid is left
								topLeft = midVertex;
								topRight = topVertex;
								topLeftSlope = slopeMidBottom;
								topRightSlope = slopeTopBottom;

								bottomLeft = midVertex;
								bottomRight = bottomVertex;
								bottomLeftSlope = slopeTopMid;
								bottomRightSlope = slopeTopBottom;

							} else {
								topLeft = topVertex;
								topLeftSlope = slopeTopBottom;
								topRight = midVertex;
								topRightSlope = slopeMidBottom;

								bottomLeft = bottomVertex;
								bottomRight = midVertex;
								bottomLeftSlope = slopeTopBottom;
								bottomRightSlope = slopeTopMid;
							}
							if (display) {
								ss.clear();
								ss << "isfinite(bottomRightSlope): " << isfinite(bottomRightSlope);
								ss << " ";
								ss << "isfinite(bottomLeftSlope): " << isfinite(bottomLeftSlope);
								ss << endl;
							}

							fillFlatTop(topVertex, midVertex, bottomVertex, topLeft, topRight,
									topLeftSlope, topRightSlope, color3f);
							fillFlatBottom(topVertex, midVertex, bottomVertex, bottomLeft, bottomRight,
									bottomLeftSlope, bottomRightSlope, color3f);
						}
					}

					/*
					// figure out which side is left and right
					// compare deltas
					deltaTopMid = 1.0 / slopeTopMid;
					deltaTopBottom = 1.0 / slopeTopBottom;
					deltaMidBottom = 1.0 / slopeMidBottom;

					//RzVertex3f *left, *right;

					if (topVertex->getY() == bottomVertex->getY()) {
						// perfectly flat line
						//fillHorizontalLine(topVertex, midVertex, color3f);
						//fillHorizontalLine(midVertex, bottomVertex, color3f);

					} else if (slopeTopMid == 0) {
						// flat on top
						fillLowerPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaMidBottom, color3f);
					} else if (deltaMidBottom == 0) {
						// flat on bottom
						fillUpperPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaTopMid, color3f);
//						fillFlatBottom(topVertex, midVertex, bottomVertex, bottomVertex, midVertex,
//								slopeTopBottom, slopeTopMid, color3f);
					} else if (deltaTopMid < deltaTopBottom) {
						fillUpperPart(topVertex, midVertex, bottomVertex,
								deltaTopMid, deltaTopBottom, color3f);
						fillLowerPart(topVertex, midVertex, bottomVertex,
								deltaMidBottom, deltaTopBottom, color3f);
//						fillFlatBottom(topVertex, midVertex, bottomVertex, midVertex, bottomVertex,
//								slopeTopMid, slopeTopBottom, color3f);
//						fillFlatTop(topVertex, midVertex, bottomVertex, midVertex, topVertex,
//								slopeMidBottom, slopeTopBottom, color3f);
					} else {
						fillUpperPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaTopMid, color3f);
						fillLowerPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaMidBottom, color3f);

//						fillFlatBottom(topVertex, midVertex, bottomVertex, bottomVertex, midVertex,
//								slopeTopBottom, slopeTopMid, color3f);
//						fillFlatTop(topVertex, midVertex, bottomVertex, topVertex, midVertex,
//								slopeTopBottom, slopeMidBottom, color3f);
					}
					*/
				}
			}
		}

		// draw the color buffer
		glBegin(GL_POINTS);
		for (x = 0; x < WINDOW_WIDTH; ++x) {
			for (y = 0; y < WINDOW_HEIGHT; ++y) {
				// set the color
				glColor3f(colorBuffer[x][y].getRed(),
						colorBuffer[x][y].getGreen(),
						colorBuffer[x][y].getBlue());

				//glVertex3f((float)x, (float)y, 0.5);
				glVertex2f((float)x, (float)y);

			}
		}
		glEnd();

		SDL_GL_SwapBuffers();
		// Check keypresses
		if(key[SDLK_RIGHT]) {
			//++maxPointPlots;
			++maxScanLines;
			//angle-=0.5;
		}
		if(key[SDLK_LEFT]) {
			//--maxPointPlots;
			--maxScanLines;
			//angle+=0.5;
		}
		if (key[SDLK_UP]) {
			//draw_triangles = true;
			//zAdd += 10.0;
			Debugger::getInstance().print("up key");
			near_z -= 0.1;
		}
		if (key[SDLK_DOWN]) {
			//draw_triangles = false;
			//zAdd -= 10.0;
			Debugger::getInstance().print("down key");
			near_z += 0.1;
		}

		delete perspectified;
	}
}

// Initialze OpenGL perspective matrix
void GL_Setup(int width, int height)
{
	//glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	//glDisable(GL_DEPTH_TEST);
	//gluPerspective(45, (float)width/height, 0.1, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375, 0.375, 0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

int main(int argc, char *argv[]) {
	// Initialize SDL with best video mode
	//unsigned int polygonGroupIndex;
	//unsigned int polygonIndex;
	//unsigned int vertexIndex;
	//RzPolygonGroup *polygonGroup;
	//RzPolygon *polygon;
	//RzVertex3f *vertex3f;
	//float boundingXMin, boundingXMax, boundingYMin, boundingYMax;
	//float boundingWidth;
	//float boundingHeight;
	//float scaleFactor;
	stringstream ss;
	//bool first;
	//int numLogs = 20;
	vector<RzTriangle> triangles;

	SDL_Init(SDL_INIT_VIDEO);
	const SDL_VideoInfo* info = SDL_GetVideoInfo();
	int vidFlags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER;
	if (info->hw_available) {
		vidFlags |= SDL_HWSURFACE;
		Debugger::getInstance().print("using hardware surface");
	} else {
		vidFlags |= SDL_SWSURFACE;
		Debugger::getInstance().print("using software surface");
	}
	int bpp = info->vfmt->BitsPerPixel;
	SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, bpp, vidFlags);
	GL_Setup(WINDOW_WIDTH, WINDOW_HEIGHT);

	// parse the data file
	CS455FileParser parser;
	collection = parser.parseFile("data/biplane.dat");

	/*
	zBuffer.resize(WINDOW_WIDTH, vector<float>(WINDOW_HEIGHT));
	zBufferSet.resize(WINDOW_WIDTH, vector<bool>(WINDOW_HEIGHT, false));
	colorBuffer.resize(WINDOW_WIDTH, vector<RzColor3f>(WINDOW_HEIGHT));
	*/

	/*
	// find the bounding box
	first = true;
	for (polygonGroupIndex = 0; polygonGroupIndex < collection->polygonGroups.size(); ++polygonGroupIndex) {
		polygonGroup = &collection->polygonGroups[polygonGroupIndex];
		for (polygonIndex = 0; polygonIndex < polygonGroup->polygons.size(); ++polygonIndex) {
			polygon = &polygonGroup->polygons[polygonIndex];
			for (vertexIndex = 0; vertexIndex < polygon->vertices.size(); ++vertexIndex) {
				vertex3f = &polygon->vertices[vertexIndex];
				if (first) {
					boundingXMin = boundingXMax = vertex3f->getX();
					boundingYMin = boundingYMax = vertex3f->getY();
					first = false;
				} else {
					if(vertex3f->getX() < boundingXMin) {
						boundingXMin = vertex3f->getX();
					}
					if(vertex3f->getX() > boundingXMax) {
						boundingXMax = vertex3f->getX();
					}
					if(vertex3f->getY() < boundingYMin) {
						boundingYMin = vertex3f->getY();
					}
					if(vertex3f->getY() > boundingYMax) {
						boundingYMax = vertex3f->getY();
					}
				}
			}
		}
	}

	boundingXMin -= 0.01 * (boundingXMax - boundingXMin);
	boundingXMax += 0.01 * (boundingXMax - boundingXMin);

	for (polygonGroupIndex = 0; polygonGroupIndex < collection->polygonGroups.size(); ++polygonGroupIndex) {
		polygonGroup = &collection->polygonGroups[polygonGroupIndex];
		for (polygonIndex = 0; polygonIndex < polygonGroup->polygons.size(); ++polygonIndex) {
			polygon = &polygonGroup->polygons[polygonIndex];
			for (vertexIndex = 0; vertexIndex < polygon->vertices.size(); ++vertexIndex) {
				vertex3f = &polygon->vertices[vertexIndex];
				// put vertex in our coordinate system
				vertex3f->setX(
						translateScaleX(boundingXMin, boundingXMax,
								boundingYMin, boundingYMax, vertex3f->getX()));
				vertex3f->setY(
						translateScaleY(boundingXMin, boundingXMax,
								boundingYMin, boundingYMax, vertex3f->getY()));
			}
		}
	}
	*/

	main_loop_function();

	delete collection;

	return 0;
}
