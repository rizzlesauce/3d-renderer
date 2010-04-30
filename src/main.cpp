#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <string>
#include "CS455FileParser.h"
#include "RzPolygonGroupCollection.h"
#include "Debugger.h"

using namespace std;

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 700

// Keydown booleans
bool key[321];

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

void main_loop_function()
{
	float angle;
	while(events())
	{
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT);
		//glLoadIdentity();
		//glTranslatef(0,0, -10);
		//glRotatef(angle, 0, 0, 1);
		glBegin(GL_QUADS);
		//glColor3ub(255, 000, 000); glVertex2f(-1,  1);
		//glColor3ub(000, 255, 000); glVertex2f( 1,  1);
		//glColor3ub(000, 000, 255); glVertex2f( 1, -1);
		//glColor3ub(255, 255, 000); glVertex2f(-1, -1);
		glColor3ub(255, 000, 000); glVertex2f(200,  500);
		glColor3ub(000, 255, 000); glVertex2f( 500,  500);
		glColor3ub(000, 000, 255); glVertex2f( 500, 200);
		glColor3ub(255, 255, 000); glVertex2f(200, 200);
		glEnd();
		SDL_GL_SwapBuffers();
		// Check keypresses
		if(key[SDLK_RIGHT]) {
			angle-=0.5;
		}
		if(key[SDLK_LEFT]) {
			angle+=0.5;
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

	main_loop_function();

	delete collection;
}
