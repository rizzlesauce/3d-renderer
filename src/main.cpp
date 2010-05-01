#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
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
vector<vector<float> > zbuffer;
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

void main_loop_function()
{
	unsigned int polygonGroupIndex;
	unsigned int polygonIndex;
	unsigned int triangleIndex;
	unsigned int vertexIndex;
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

	//float angle;
	//float xAdd = 0;
	//float yAdd = 0;

	while(events())
	{
		// find the bounding box
		first = false;
		for (polygonGroupIndex = 0; polygonGroupIndex < collection->polygonGroups.size(); ++polygonGroupIndex) {
			polygonGroup = &collection->polygonGroups[polygonGroupIndex];
			for (polygonIndex = 0; polygonIndex < polygonGroup->polygons.size(); ++polygonIndex) {
				polygon = &polygonGroup->polygons[polygonIndex];
				for (vertexIndex = 0; vertexIndex < polygon->vertices.size(); ++vertexIndex) {
					vertex3f = &polygon->vertices[vertexIndex];
					if (first) {
						boundingXMin = boundingXMax = vertex3f->coordinates[0];
						boundingYMin = boundingYMax = vertex3f->coordinates[1];
						first = false;
					} else {
						if(vertex3f->coordinates[0] < boundingXMin) {
							boundingXMin = vertex3f->coordinates[0];
						}
						if(vertex3f->coordinates[0] > boundingXMax) {
							boundingXMax = vertex3f->coordinates[0];
						}
						if(vertex3f->coordinates[1] < boundingYMin) {
							boundingYMin = vertex3f->coordinates[1];
						}
						if(vertex3f->coordinates[1] > boundingYMax) {
							boundingYMax = vertex3f->coordinates[1];
						}
					}
				}
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
		glClear(GL_COLOR_BUFFER_BIT);
		//glLoadIdentity();
		//glTranslatef(0,0, -10);
		//glRotatef(angle, 0, 0, 1);
		//glBegin(GL_QUADS);
		glBegin(GL_POINTS);
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

			// set the color
			glColor3ub(floor(0.5 + color3f->components[0] * 255.0),
					floor(0.5 + color3f->components[1] * 255.0),
					floor(0.5 + color3f->components[2] * 255.0));

			// draw the polygons
			for (polygonIndex = 0; polygonIndex < polygonGroup->polygons.size(); ++polygonIndex) {
				polygon = &polygonGroup->polygons[polygonIndex];

				vector<RzTriangle> triangles = polygon->getTriangles();

				// get the triangles
				for (triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {
					// draw the triangle vertices
					triangle = &triangles[triangleIndex];
					for (vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
						vertex3f = &triangle->vertices[vertexIndex];

						glVertex2f(
								floor(0.5 + translateScaleX(boundingXMin, boundingXMax,
										boundingYMin, boundingYMax, vertex3f->coordinates[0])),
								floor(0.5 + translateScaleY(boundingXMin, boundingXMax,
										boundingYMin, boundingYMax, vertex3f->coordinates[1])));
					}
				}
			}
		}

		glEnd();
		SDL_GL_SwapBuffers();
		// Check keypresses
		if(key[SDLK_RIGHT]) {
			//angle-=0.5;
		}
		if(key[SDLK_LEFT]) {
			//angle+=0.5;
		}
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

	zbuffer.resize(WINDOW_WIDTH, vector<float>(WINDOW_HEIGHT));
	colorBuffer.resize(WINDOW_WIDTH, vector<RzColor3f>(WINDOW_HEIGHT));

	main_loop_function();

	delete collection;
}
