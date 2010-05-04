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
vector<vector<float> > zBuffer;
vector<vector<bool> > zBufferSet;
vector<vector<RzColor3f> > colorBuffer;

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

void drawPoint3fForReal(int x, int y, float z, RzColor3f color) {
	zBufferSet[x][y] = true;
	zBuffer[x][y] = z;
	colorBuffer[x][y] = color;
	//glVertex2f((float)x, (float)y);
}

void drawPoint3f(float x, float y, float z, RzColor3f color) {
	// get buffer coordinates
	int i_x = round(x);
	int i_y = round(y);

	if (i_x >= WINDOW_WIDTH || i_x < 0 || i_y >= WINDOW_HEIGHT || i_y < 0) {
			// out of bounds
		return;
	}

	// check the z buffer
	if (zBufferSet[i_x][i_y]) {
		if (z > zBuffer[i_x][i_y]) {
			drawPoint3fForReal(i_x, i_y, z, color);
		}
	} else {
		drawPoint3fForReal(i_x, i_y, z, color);
	}
}

float interpolateZ(float z1, float z2, float z3, float y1, float ys, float y2, float y3, float xa, float xp, float xb) {
	float za, zb, zp;

	za = z1 - (z1 - z2) * (y1 - ys) / (y1 - y2);
	zb = z1 - (z1 - z3) * (y1 - ys) / (y1 - y3);
	zp = zb - (zb - za) * (xb - xp) / (xb - xa);

	return zp;
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
	float boundingXMin, boundingXMax, boundingYMin, boundingYMax;
	//float boundingWidth;
	//float boundingHeight;
	//float scaleFactor;
	stringstream ss;
	bool first;
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

	float leftUpperSlope, leftLowerSlope, rightUpperSlope, rightLowerSlope;
	float xLeft, xRight;
	float row;
	float yPosition;
	float xPosition;


	//float angle;
	//float xAdd = 0;
	//float yAdd = 0;
	//float zAdd = 0.0;

	while(events())
	{
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
					for (vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
						vertex3f = &triangle->vertices[vertexIndex];

						// put vertex in our coordinate system
						vertex3f->setX(
								translateScaleX(boundingXMin, boundingXMax,
										boundingYMin, boundingYMax, vertex3f->getX()));
						vertex3f->setY(
								translateScaleY(boundingXMin, boundingXMax,
										boundingYMin, boundingYMax, vertex3f->getY()));

						/*
						glVertex2f(round(vertex3f->getX()),
								round(vertex3f->getY()));
								*/
					}
					// scan line convert

					/*
					vector<RzVertex3f> vertices;
					vertices.push_back(triangle->vertices[0]);
					vertices.push_back(triangle->vertices[1]);
					vertices.push_back(triangle->vertices[2]);
					sort(vertices.begin(), vertices.end(), compareVertexYs);

					topVertex = &vertices[0];
					midVertex = &vertices[1];
					bottomVertex = &vertices[2];
					*/

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

					//RzVertex3f *left, *right;

					if (deltaTopMid < deltaTopBottom) {
						//left = midVertex;
						//right = bottomVertex;

						leftUpperSlope = slopeTopMid;
						leftLowerSlope = slopeMidBottom;
						rightUpperSlope = rightLowerSlope = slopeTopBottom;
					} else {
						//left = bottomVertex;
						//right = midVertex;

						leftUpperSlope = leftLowerSlope = slopeTopBottom;
						rightUpperSlope = slopeTopMid;
						rightLowerSlope = slopeMidBottom;
					}

					/*
					// test for infinite slopes
					if (1.0 / leftUpperSlope == 0 || 1.0 / leftUpperSlope == -0) {
							// infinite
						leftUpperSlope = 0;
					}
					if (1.0 / rightUpperSlope == 0 || 1.0 / rightUpperSlope == -0) {
							// infinite
						rightUpperSlope = 0;
					}
					if (1.0 / leftLowerSlope == 0 || 1.0 / leftLowerSlope == -0) {
							// infinite
						leftLowerSlope = 0;
					}
					if (1.0 / rightLowerSlope == 0 || 1.0 / rightLowerSlope == -0) {
							// infinite
						rightLowerSlope = 0;
					}
					*/

					/*
					ss.clear();
					ss << "yPosition: " << yPosition;
					ss << " ";
					ss << "midVertex.y: " << midVertex->getY();
					Debugger::getInstance().print(ss.str());
					*/

					// fill the upper rows
					row = 0.0;
					yPosition = topVertex->getY();
					while (yPosition <= midVertex->getY()) {
						// fill in the line

						xLeft = xRight = topVertex->getX();

						xLeft += row / leftUpperSlope;
						xRight += row / rightUpperSlope;

						/*
						ss.clear();
						ss << "xLeft: " << xLeft;
						ss << " ";
						ss << "xRight: " << xRight;
						Debugger::getInstance().print(ss.str());
						*/

						/* xRight = -inf?
						if (xLeft >= xRight) {
							ss.clear();
							ss << "xLeft: " << xLeft;
							ss << " ";
							ss << "xRight: " << xRight;
							Debugger::getInstance().print(ss.str());
						}
						*/

						xPosition = xLeft;
						while (xPosition < xRight) {
							// draw the point
							drawPoint3f(xPosition, yPosition,
									interpolateZ(topVertex->getZ(),
											midVertex->getZ(),
											bottomVertex->getZ(),
											topVertex->getY(),
											yPosition,
											midVertex->getY(),
											bottomVertex->getY(),
											xLeft,
											xPosition,
											xRight
											),
											*color3f);

							/*
							if (xPosition < 0 || yPosition < 0) {
								ss.clear();
								ss << "xPos: " << xPosition << "yPos: " << yPosition;
								Debugger::getInstance().print(ss.str());
							}
							*/
							xPosition += 1.0;
						}

						yPosition += 1.0;
						row = row + 1.0;
					}

					// fill the lower rows
					yPosition = bottomVertex->getY();
					row = 0.0;
					while (yPosition > midVertex->getY()) {
						// fill in the line

						xLeft = xRight = bottomVertex->getX();
						xLeft += row / leftLowerSlope;
						xRight += row / rightLowerSlope;

						xPosition = xLeft;
						while (xPosition < xRight) {
							// draw the point
							drawPoint3f(xPosition, yPosition,
									interpolateZ(bottomVertex->getZ(),
											midVertex->getZ(),
											topVertex->getZ(),
											bottomVertex->getY(),
											yPosition,
											midVertex->getY(),
											topVertex->getY(),
											xLeft,
											xPosition,
											xRight
											),
											*color3f);

							xPosition += 1.0;
						}

						yPosition -= 1.0;
						row -= 1.0;
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
			zAdd += 10.0;
			Debugger::getInstance().print("up key");
		}
		if (key[SDLK_DOWN]) {
			zAdd -= 10.0;
			Debugger::getInstance().print("down key");
		}
		*/
	}
}

// Initialze OpenGL perspective matrix
void GL_Setup(int width, int height)
{
	glViewport(0, 0, width, height);
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

	zBuffer.resize(WINDOW_WIDTH, vector<float>(WINDOW_HEIGHT));
	zBufferSet.resize(WINDOW_WIDTH, vector<bool>(WINDOW_HEIGHT, false));
	colorBuffer.resize(WINDOW_WIDTH, vector<RzColor3f>(WINDOW_HEIGHT));

	main_loop_function();

	delete collection;
}
