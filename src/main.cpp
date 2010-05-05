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

RzPolygonGroupCollection *collection;

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

float computeSlope2d(RzVertex3f *v1, RzVertex3f *v2) {
	float denominator = v1->getX() - v2->getX();
	float slope;
	/* float takes care of divide by zero for us
	if (denominator == 0) {
		// don't try to divide by zero
		slope = 0;
	} else {
		slope = (v1->getY() - v2->getY()) / denominator;
	}
	*/
	slope = (v1->getY() - v2->getY()) / denominator;
	return slope;
}

float computeDelta2d(RzVertex3f *v1, RzVertex3f *v2) {
	float delta = 1.0 / computeSlope2d(v1, v2);
	return delta;
}

void swapPointers(void **v1, void **v2) {
	void *tmp_v = *v1;
	*v1 = *v2;
	*v2 = tmp_v;
}

int round1f(float f) {
	return floor(0.5 + f);
}

/*
bool compareVertexYs(RzVertex3f v1, RzVertex3f v2) {
	return v1.getY() < v2.getY();
}
*/

void drawPoint3iForReal(int x, int y, float z, RzColor3f color) {
	zBufferSet[x][y] = true;
	zBuffer[x][y] = z;
	colorBuffer[x][y] = color;
	//glVertex2f((float)x, (float)y);
}

void drawPoint3f(float x, float y, float z, RzColor3f color) {
	// get buffer coordinates
	int i_x = round(x);
	int i_y = round(y);
	//int i_x = floor(x);
	//int i_y = floor(y);

	if (i_x >= WINDOW_WIDTH || i_x < 0 || i_y >= WINDOW_HEIGHT || i_y < 0) {
			// out of bounds
		return;
	}

	// check the z buffer
	if (zBufferSet[i_x][i_y]) {
		if (z > zBuffer[i_x][i_y]) {
			drawPoint3iForReal(i_x, i_y, z, color);
		}
	} else {
		drawPoint3iForReal(i_x, i_y, z, color);
	}
}

float interpolateZ(float z1, float z2, float z3, float y1, float ys, float y2, float y3, float xa, float xp, float xb) {
	float za, zb, zp;

	za = z1 - (z1 - z2) * (y1 - ys) / (y1 - y2);
	zb = z1 - (z1 - z3) * (y1 - ys) / (y1 - y3);
	zp = zb - (zb - za) * (xb - xp) / (xb - xa);

	return zp;
}

float interpolateZhorizontal(float za, float zb, float xa, float xp, float xb) {
	float zp;

	zp = zb - (zb - za) * (xb - xp) / (xb - xa);
	return zp;
}

void fillUpperPart(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, float leftUpperDelta,
		float rightUpperDelta, RzColor3f *color3f) {

	float yPosition = top->getY();
	float xLeft, xRight;
	float xPosition;
	float endY = mid->getY();
	xLeft = xRight = top->getX();
	/*
	if ((float)round(yPosition) > yPosition) {
		yPosition -= 1.0;
		endY -= 1.0;
	}
	*/
	while (yPosition < endY) {
		// fill in the line

		xPosition = xLeft;
		while (xPosition < xRight) {
			// draw the point
			drawPoint3f(xPosition, yPosition,
					interpolateZ(top->getZ(),
							mid->getZ(),
							bottom->getZ(),
							top->getY(),
							yPosition,
							mid->getY(),
							bottom->getY(),
							xLeft,
							xPosition,
							xRight
							),
							*color3f);

			xPosition += 1.0;
		}

		xLeft += leftUpperDelta;
		xRight += rightUpperDelta;

		yPosition += 1.0;
	}
}

/*
void fillLine(RzVertex3f *v1, RzVertex3f *v2, RzColor3f *color3f) {
	float slope = computeSlope2d(v1, v2);

}
*/
void fillHorizontalLine(RzVertex3f *v1, RzVertex3f *v2, RzColor3f *color3f) {
	float xLeft, xRight;
	float xPosition;
	float yPosition = v1->getY();
	// fill in the line

	xLeft = v1->getX();
	xRight = v2->getX();

	xPosition = xLeft;
	while (xPosition <= xRight) {
		// draw the point
		drawPoint3f(xPosition, yPosition,
				interpolateZhorizontal(v1->getZ(),
						v2->getZ(),
						xLeft,
						xPosition,
						xRight
						),
						*color3f);

		xPosition += 1.0;
	}
}

void fillLowerPart(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, float leftLowerDelta,
		float rightLowerDelta, RzColor3f *color3f) {

	float yPosition = bottom->getY();
	float xLeft, xRight;
	float xPosition;
	xLeft = xRight = bottom->getX();
	while (yPosition > mid->getY()) {
		// fill in the line

		xPosition = xLeft;
		while (xPosition < xRight) {
			// draw the point
			drawPoint3f(xPosition, yPosition,
					interpolateZ(bottom->getZ(),
							mid->getZ(),
							top->getZ(),
							bottom->getY(),
							yPosition,
							mid->getY(),
							top->getY(),
							xLeft,
							xPosition,
							xRight
							),
							*color3f);

			xPosition += 1.0;
		}

		xLeft -= leftLowerDelta;
		xRight -= rightLowerDelta;
		yPosition += -1.0;
	}
}

void main_loop_function()
{
	unsigned int polygonGroupIndex;
	unsigned int polygonIndex;
	unsigned int triangleIndex;
	//unsigned int vertexIndex;
	unsigned int x, y;
	RzPolygonGroup *polygonGroup;
	RzPolygon *polygon;
	RzTriangle *triangle;
	//RzVertex3f *vertex3f;
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

	//float angle;
	//float xAdd = 0;
	//float yAdd = 0;
	//float zAdd = 0.0;

	while(events())
	{
		// initialize the buffers
		for (x = 0; x < WINDOW_WIDTH; ++x) {
			for (y = 0; y < WINDOW_HEIGHT; ++y) {
				zBufferSet[x][y] = false;
				colorBuffer[x][y].setRed(0.6);
				colorBuffer[x][y].setGreen(0.6);
				colorBuffer[x][y].setBlue(1.0);
			}
		}

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

		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

		for (polygonGroupIndex = 0; polygonGroupIndex < collection->polygonGroups.size(); ++polygonGroupIndex) {
			polygonGroup = &collection->polygonGroups[polygonGroupIndex];
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
					if (topVertex->getY() == bottomVertex->getY()) {
						// perfectly flat line
						// sort by x value
						if (midVertex->getX() > bottomVertex->getX()) {
							swapPointers((void**)&midVertex, (void**)&bottomVertex);
						}
						if (topVertex->getX() > midVertex->getX()) {
							swapPointers((void**)&topVertex, (void**)&midVertex);

							if (midVertex->getX() > bottomVertex->getX()) {
								swapPointers((void**)&midVertex, (void**)&bottomVertex);
							}
						}
					} else if (topVertex->getY() == midVertex->getY()) {
						// flat on top
						if (topVertex->getX() > midVertex->getX()) {
							swapPointers((void**)&topVertex, (void**)&midVertex);
						}
					} else if (midVertex->getY() == bottomVertex->getY()) {
						// flat on bottom
						if (bottomVertex->getX() > midVertex->getX()) {
							swapPointers((void**)&bottomVertex, (void**)&midVertex);
						}
					}

					// what about the case where the triangle is perfectly thin?

					/*
					glColor3f(1.0, 0, 0);
					glVertex2f(round(bottomVertex->getX()),
							round(bottomVertex->getY()));
					glColor3f(0, 1.0, 0);
					glVertex2f(round(midVertex->getX()),
							round(midVertex->getY()));
					glColor3f(0, 0, 1.0);
					glVertex2f(round(topVertex->getX()),
							round(topVertex->getY()));
							*/

					/*
					if (glGetError() != GL_NO_ERROR) {
						Debugger::getInstance().print("gl error");
					}
					*/

					// calculate the slopes
					slopeTopMid = computeSlope2d(topVertex, midVertex);
					slopeTopBottom = computeSlope2d(topVertex, bottomVertex);
					slopeMidBottom = computeSlope2d(midVertex, bottomVertex);

					// figure out which side is left and right
					// compare deltas
					deltaTopMid = 1.0 / slopeTopMid;
					deltaTopBottom = 1.0 / slopeTopBottom;
					deltaMidBottom = 1.0 / slopeMidBottom;

					//RzVertex3f *left, *right;

					if (topVertex->getY() == bottomVertex->getY()) {
						// perfectly flat line
						fillHorizontalLine(topVertex, midVertex, color3f);
						fillHorizontalLine(midVertex, bottomVertex, color3f);

					} else if (topVertex->getY() == midVertex->getY()) {
						// flat on top
						fillLowerPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaMidBottom, color3f);
					} else if (midVertex->getY() == bottomVertex->getY()) {
						// flat on bottom
						fillUpperPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaTopMid, color3f);
					} else if (deltaTopMid < deltaTopBottom) {
						fillUpperPart(topVertex, midVertex, bottomVertex,
								deltaTopMid, deltaTopBottom, color3f);
						fillLowerPart(topVertex, midVertex, bottomVertex,
								deltaMidBottom, deltaTopBottom, color3f);
					} else {
						fillUpperPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaTopMid, color3f);
						fillLowerPart(topVertex, midVertex, bottomVertex,
								deltaTopBottom, deltaMidBottom, color3f);
					}
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

				glVertex2d((float)x, (float)y);
			}
		}
		glEnd();

		SDL_GL_SwapBuffers();
		/*
		// Check keypresses
		if(key[SDLK_RIGHT]) {
			//angle-=0.5;
		}
		if(key[SDLK_LEFT]) {
			//angle+=0.5;
		}
		if (key[SDLK_UP]) {
			//zAdd += 10.0;
			Debugger::getInstance().print("up key");
		}
		if (key[SDLK_DOWN]) {
			//zAdd -= 10.0;
			Debugger::getInstance().print("down key");
		}
		*/
	}
}

// Initialze OpenGL perspective matrix
void GL_Setup(int width, int height)
{
	//glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	//glEnable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);
	//gluPerspective(45, (float)width/height, 0.1, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375, 0.375, 0);
}

int main(int argc, char *argv[]) {
	// Initialize SDL with best video mode
	unsigned int polygonGroupIndex;
	unsigned int polygonIndex;
	unsigned int vertexIndex;
	RzPolygonGroup *polygonGroup;
	RzPolygon *polygon;
	RzVertex3f *vertex3f;
	float boundingXMin, boundingXMax, boundingYMin, boundingYMax;
	//float boundingWidth;
	//float boundingHeight;
	//float scaleFactor;
	stringstream ss;
	bool first;
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

	boundingXMin -= 0.1 * (boundingXMax - boundingXMin);
	boundingXMax += 0.1 * (boundingXMax - boundingXMin);

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

	main_loop_function();

	delete collection;
}
