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
#include "matrixmath.h"

using namespace std;

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 700

float interpolateComponent(float, float, float, float, float, float, float, float, float, float);

// Keydown booleans
bool key[321];
float zBuffer[WINDOW_WIDTH][WINDOW_HEIGHT];
bool zBufferSet[WINDOW_WIDTH][WINDOW_HEIGHT];
RzColor3f colorBuffer[WINDOW_WIDTH][WINDOW_HEIGHT];
CS455FileParser parser;

string modelFiles[] = { //"apple.dat",
		//"arm.dat",
		"armory.dat",
		"biplane.dat",
		"camaro.dat",
		//"monster.dat",
		//"skull.dat"
};
int numModels = 3;
int currentModelIndex = 1;

VECTOR3D light_pos = {-10.0f, 10.0f, 10.0f };
//POINT3D light_pos = {0.0f, 1.0f, 5.0f};
POINT4D relative_light_pos;
RzColor3f light_color;
RzColor3f ambient_light;
RzColor3f specular_highlight;
float phong_exponent = 10.0f;

//float eye_z, near_z, far_z, other_factor;
POINT3D camera_position = {0.0f, 0.0f, 20.0f};
POINT3D camera_target = {0.0f, 0.0f, 0.0f};
VECTOR3D camera_up = {0.0f, 1.0f, 0.0f};
float near_z = -1.0f;
float far_z = -5000;
float field_of_view = (float)PI / 2.0;

//int numPointPlots = 0;
//int maxPointPlots = 0;
//int numScanLines = 0;
//int maxScanLines = 1000000000;
//int maxScanLines = 0;

RzPolygonGroupCollection *collection = NULL;
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
	float denominator = (float)(intify(v1->screen_x) - intify(v2->screen_x));
	float numerator = (float)(intify(v1->screen_y) - intify(v2->screen_y));

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
	float delta = 1.0f / computeSlope2d(v1, v2);
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

void drawPoint2iWithShading(int screen_x, int screen_y, float camera_space_x, float camera_space_y,
		float camera_space_z, VECTOR3D_PTR surface_normal, RzColor3f *surface_color) {

	RzColor3f color;

	VECTOR3D light_vector = { relative_light_pos.x - camera_space_x,
			relative_light_pos.y - camera_space_y,
			relative_light_pos.z - camera_space_z
	};

	VECTOR3D_Normalize(&light_vector);
	VECTOR3D_Normalize(surface_normal);

	VECTOR3D eye_vector = { -camera_space_x, -camera_space_y, -camera_space_z };
	VECTOR3D_Normalize(&eye_vector);

	VECTOR3D reflect_vector, scaled_normal, tmp_vector;

	VECTOR3D_Scale(2.0f, surface_normal, &scaled_normal);
	float surface_dot_light = VECTOR3D_Dot(surface_normal, &light_vector);
	VECTOR3D_Scale(surface_dot_light, &scaled_normal, &tmp_vector);
	VECTOR3D_Sub(&tmp_vector, &light_vector, &reflect_vector);

	for (int c = 0; c < 3; ++c) {
		color.components[c]
		    = min(1.0f,
		    		(
		    				// lambertion illumination
		    				surface_color->components[c] * light_color.components[c] * max(0.0f,
		    				surface_dot_light)
		    				)
		    		)
		    		+
		    		(
		    				// ambient light
		    				ambient_light.components[c] * surface_color->components[c]
		    		)
		    		+
		    		(
		    				// phong shading
		    				light_color.components[c] * specular_highlight.components[c] *
									max(0.0f, pow(VECTOR3D_Dot(&eye_vector, &reflect_vector), phong_exponent)
		    		)

		    );
	}

	drawPoint2i(screen_x, screen_y, camera_space_z, &color);
	//drawPoint2i(screen_x, screen_y, camera_space_z, surface_color);
}

void drawPoint2f(float x, float y, float z, RzColor3f *color) {
	// get buffer coordinates
	//int i_x = round1f(x);
	//int i_y = round1f(y);
	drawPoint2i(intify(x), intify(y), z, color);
}

float interpolateComponent(float c1, float c2, float c3, float y1, float ys, float y2, float y3, float xa, float xs, float xb) {
	float ca, cb, cs;

	y1 = (float)intify(y1);
	ys = (float)intify(ys);
	y2 = (float)intify(y2);
	y3 = (float)intify(y3);
	xa = (float)intify(xa);
	xs = (float)intify(xs);
	xb = (float)intify(xb);

	ca = c1 - (c1 - c2) * (y1 - ys) / (y1 - y2);
	cb = c1 - (c1 - c3) * (y1 - ys) / (y1 - y3);
	cs = cb - (cb - ca) * (xb - xs) / (xb - xa);

	return cs;
}

void interpolateNormal(RzVertex3f *v1, float xa, float xs, float ys, float xb, RzVertex3f *v2, RzVertex3f *v3,
		VECTOR3D_PTR normal) {
	int ortho_index;
	for (int i = 0; i < 3; ++i) {
		ortho_index = i + 3;
		normal->M[i] = interpolateComponent(
				v1->coordinates[ortho_index],
				v2->coordinates[ortho_index],
				v3->coordinates[ortho_index],
				(float)v1->screen_y, ys, (float)v2->screen_y, (float)v3->screen_y,
				xa, xs, xb);
	}
}

void fillFlatTop(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, RzVertex3f *left, RzVertex3f *right,
		float leftSlope, float rightSlope, RzColor3f *color3f) {

	float bLeft, bRight;

	if (isfinite(leftSlope)) {
		bLeft = (float)intify(left->screen_y) - leftSlope * (float)intify(left->screen_x);
	}
	if (isfinite(rightSlope)) {
		bRight = (float)intify(right->screen_y) - rightSlope * (float)intify(right->screen_x);
	}

	int yPos = intify(bottom->screen_y);
	int yEnd = intify(mid->screen_y);
	int xPos;
	float xLeft, xRight;
	int xMin, xMax;
	float cam_x, cam_y, cam_z;

	float leftHeight, rightHeight;
	leftHeight = intify(left->screen_y) - intify(bottom->screen_y);
	rightHeight = intify(right->screen_y) - intify(bottom->screen_y);

	VECTOR3D surface_normal;

	float left_dcam_x = (bottom->getX() - left->getX()) / leftHeight;
	float right_dcam_x = (bottom->getX() - right->getX()) / rightHeight;
	//float cam_x_left = left->getX() + left_dcam_x * y_bump;
	//float cam_x_right = right->getX() + right_dcam_x * y_bump;
	float cam_x_left = bottom->getX();
	float cam_x_right = bottom->getX();

	float left_dcam_y = (bottom->getY() - left->getY()) / leftHeight;
	float right_dcam_y = (bottom->getY() - right->getY()) / rightHeight;
	//float cam_y_left = left->getY() + left_dcam_y * y_bump;
	//float cam_y_right = right->getY() + right_dcam_y * y_bump;
	float cam_y_left = bottom->getY();
	float cam_y_right = bottom->getY();

	float left_dcam_z = (bottom->getZ() - left->getZ()) / leftHeight;
	float right_dcam_z = (bottom->getZ() - right->getZ()) / rightHeight;
	//float cam_z_left = left->getZ() + left_dcam_z * y_bump;
	//float cam_z_right = right->getZ() + right_dcam_z * y_bump;
	float cam_z_left = bottom->getZ();
	float cam_z_right = bottom->getZ();

	float left_dnorm_x = (bottom->getOrthoX() - left->getOrthoX()) / leftHeight;
	float right_dnorm_x = (bottom->getOrthoX() - right->getOrthoX()) / rightHeight;
	//float norm_x_left = left->getOrthoX() + left_dnorm_x * y_bump;
	//float norm_x_right = right->getOrthoX() + right_dnorm_x * y_bump;
	float norm_x_left = bottom->getOrthoX();
	float norm_x_right = bottom->getOrthoX();

	float left_dnorm_y = (bottom->getOrthoY() - left->getOrthoY()) / leftHeight;
	float right_dnorm_y = (bottom->getOrthoY() - right->getOrthoY()) / rightHeight;
	//float norm_y_left = left->getOrthoY() + left_dnorm_y * y_bump;
	//float norm_y_right = right->getOrthoY() + right_dnorm_y * y_bump;
	float norm_y_left = bottom->getOrthoY();
	float norm_y_right = bottom->getOrthoY();

	float left_dnorm_z = (bottom->getOrthoZ() - left->getOrthoZ()) / leftHeight;
	float right_dnorm_z = (bottom->getOrthoZ() - right->getOrthoZ()) / rightHeight;
	//float norm_z_left = left->getOrthoZ() + left_dnorm_z * y_bump;
	//float norm_z_right = right->getOrthoZ() + right_dnorm_z * y_bump;
	float norm_z_left = bottom->getOrthoZ();
	float norm_z_right = bottom->getOrthoZ();

	float line_width;
	float cam_x_value, cam_y_value, cam_z_value,
			norm_x_value, norm_y_value, norm_z_value;
	float horizontal_dcam_x, horizontal_dcam_y, horizontal_dcam_z,
			horizontal_dnorm_x, horizontal_dnorm_y, horizontal_dnorm_z;

	//xMin = xMaxLeft = xRight = (float)intify(bottom->screen_x);

	//RzColor3f color;
	//color.setRed(1.0);
	//drawPoint2i(intify(bottom->screen_x), intify(bottom->screen_y), bottom->getZ(), &color);

	while (yPos > yEnd) {
		// fill in the line

		if (isfinite(leftSlope)) {
			xLeft = ((float)yPos - bLeft) / leftSlope;
		} else {
			xLeft = (float)intify(left->screen_x);
		}

		if (isfinite(rightSlope)) {
			xRight = ((float)yPos - bRight) / rightSlope;
		} else {
			xRight = (float)intify(right->screen_x);
		}

		xMin = intify(xLeft);
		xMax = intify(xRight);

//		if (xMin < xMax) {
//			if (numScanLines >= maxScanLines) {
//				return;
//			} else {
//				++numScanLines;
//			}
//		}

		line_width = xMax - xMin;

		horizontal_dcam_x = (cam_x_right - cam_x_left) / line_width;
		cam_x_value = cam_x_left;

		horizontal_dcam_y = (cam_y_right - cam_y_left) / line_width;
		cam_y_value = cam_y_left;

		horizontal_dcam_z = (cam_z_right - cam_z_left) / line_width;
		cam_z_value = cam_z_left;

		horizontal_dnorm_x = (norm_x_right - norm_x_left) / line_width;
		norm_x_value = norm_x_left;

		horizontal_dnorm_y = (norm_y_right - norm_y_left) / line_width;
		norm_y_value = norm_y_left;

		horizontal_dnorm_z = (norm_z_right - norm_z_left) / line_width;
		norm_z_value = norm_z_left;

		xPos = xMin;
		while (xPos < xMax) {
			/*
			interpolateNormal(bottom, xLeft, (float)xPos, (float)yPos, xRight, mid, top, &surface_normal);
			cam_x = interpolateComponent(bottom->getX(), mid->getX(), top->getX(),
					bottom->screen_y, (float)yPos, mid->screen_y, top->screen_y,
					xLeft, (float)xPos, xRight);
			cam_y = interpolateComponent(bottom->getY(), mid->getY(), top->getY(),
					bottom->screen_y, (float)yPos, mid->screen_y, top->screen_y,
					xLeft, (float)xPos, xRight);
			cam_z = interpolateComponent(bottom->getZ(), mid->getZ(), top->getZ(),
					bottom->screen_y, (float)yPos, mid->screen_y, top->screen_y,
					xLeft, (float)xPos, xRight);

			drawPoint2iWithShading(xPos, yPos,
					cam_x, cam_y, cam_z,
					&surface_normal,
					color3f
					);
					*/

			VECTOR3D_INITXYZ(&surface_normal, norm_x_value, norm_y_value, norm_z_value);

			drawPoint2iWithShading(xPos, yPos,
					cam_x_value, cam_y_value, cam_z_value,
					&surface_normal,
					color3f
					);

			++xPos;

			cam_x_value += horizontal_dcam_x;
			cam_y_value += horizontal_dcam_y;
			cam_z_value += horizontal_dcam_z;

			norm_x_value += horizontal_dnorm_x;
			norm_y_value += horizontal_dnorm_y;
			norm_z_value += horizontal_dnorm_z;
		}

		--yPos;

		cam_x_left += left_dcam_x;
		cam_x_right += right_dcam_x;
		cam_y_left += left_dcam_y;
		cam_y_right += right_dcam_y;
		cam_z_left += left_dcam_z;
		cam_z_right += right_dcam_z;

		norm_x_left += left_dnorm_x;
		norm_x_right += right_dnorm_x;
		norm_y_left += left_dnorm_y;
		norm_y_right += right_dnorm_y;
		norm_z_left += left_dnorm_z;
		norm_z_right += right_dnorm_z;
	}
}

void fillFlatBottom(RzVertex3f *top, RzVertex3f *mid, RzVertex3f *bottom, RzVertex3f *left, RzVertex3f *right,
		float leftSlope, float rightSlope, RzColor3f *color3f) {

	float bLeft, bRight;

	if (isfinite(leftSlope)) {
		bLeft = (float)intify(left->screen_y) - leftSlope * (float)intify(left->screen_x);
	}
	if (isfinite(rightSlope)) {
		bRight = (float)intify(right->screen_y) - rightSlope * (float)intify(right->screen_x);
	}

	int yPos = intify(top->screen_y);
	int yEnd = intify(mid->screen_y);
	int xPos;
	float xLeft, xRight;
	int xMin, xMax;
	VECTOR3D surface_normal;
	float cam_x, cam_y, cam_z;

	float leftHeight, rightHeight;
	leftHeight = intify(left->screen_y) - intify(top->screen_y);
	rightHeight = intify(right->screen_y) - intify(top->screen_y);

	/*
	if (height < 1.0f) {
		return;
	}
	*/

	//float y_bump = yPos - top->screen_y;
	//float x_left = top->screen_x + left_dx * y_bump;
	//float x_right = top->screen_x + right_dx * y_bump;

	float left_dcam_x = (left->getX() - top->getX()) / leftHeight;
	float right_dcam_x = (right->getX() - top->getX()) / rightHeight;
	//float cam_x_left = top->getX() + left_dcam_x * y_bump;
	//float cam_x_right = top->getX() + right_dcam_x * y_bump;
	float cam_x_left = top->getX();
	float cam_x_right = top->getX();

	float left_dcam_y = (left->getY() - top->getY()) / leftHeight;
	float right_dcam_y = (right->getY() - top->getY()) / rightHeight;
	//float cam_y_left = top->getY() + left_dcam_y * y_bump;
	//float cam_y_right = top->getY() + right_dcam_y * y_bump;
	float cam_y_left = top->getY();
	float cam_y_right = top->getY();

	float left_dcam_z = (left->getZ() - top->getZ()) / leftHeight;
	float right_dcam_z = (right->getZ() - top->getZ()) / rightHeight;
	//float cam_z_left = top->getZ() + left_dcam_z * y_bump;
	//float cam_z_right = top->getZ() + right_dcam_z * y_bump;
	float cam_z_left = top->getZ();
	float cam_z_right = top->getZ();

	float left_dnorm_x = (left->getOrthoX() - top->getOrthoX()) / leftHeight;
	float right_dnorm_x = (right->getOrthoX() - top->getOrthoX()) / rightHeight;
	//float norm_x_left = top->getOrthoX() + left_dnorm_x * y_bump;
	//float norm_x_right = top->getOrthoX() + right_dnorm_x * y_bump;
	float norm_x_left = top->getOrthoX();
	float norm_x_right = top->getOrthoX();

	float left_dnorm_y = (left->getOrthoY() - top->getOrthoY()) / leftHeight;
	float right_dnorm_y = (right->getOrthoY() - top->getOrthoY()) / rightHeight;
	//float norm_y_left = top->getOrthoY() + left_dnorm_y * y_bump;
	//float norm_y_right = top->getOrthoY() + right_dnorm_y * y_bump;
	float norm_y_left = top->getOrthoY();
	float norm_y_right = top->getOrthoY();

	float left_dnorm_z = (left->getOrthoZ() - top->getOrthoZ()) / leftHeight;
	float right_dnorm_z = (right->getOrthoZ() - top->getOrthoZ()) / rightHeight;
	//float norm_z_left = top->getOrthoZ() + left_dnorm_z * y_bump;
	//float norm_z_right = top->getOrthoZ() + right_dnorm_z * y_bump;
	float norm_z_left = top->getOrthoZ();
	float norm_z_right = top->getOrthoZ();

	float line_width;
	float cam_x_value, cam_y_value, cam_z_value,
			norm_x_value, norm_y_value, norm_z_value;
	float horizontal_dcam_x, horizontal_dcam_y, horizontal_dcam_z,
			horizontal_dnorm_x, horizontal_dnorm_y, horizontal_dnorm_z;

	//xMin = xMaxLeft = xRight = (float)intify(bottom->screen_x);

	//RzColor3f color;
	//color.setRed(1.0);
	//drawPoint2i(intify(bottom->screen_x), intify(bottom->screen_y), bottom->getZ(), &color);

	while (yPos <= yEnd) {
		// fill in the line

		if (isfinite(leftSlope)) {
			xLeft = ((float)yPos - bLeft) / leftSlope;
		} else {
			xLeft = (float)intify(left->screen_x);
		}

		if (isfinite(rightSlope)) {
			xRight = ((float)yPos - bRight) / rightSlope;
		} else {
			xRight = (float)intify(right->screen_x);
		}

		xMin = intify(xLeft);
		xMax = intify(xRight);

//		if (xMin < xMax) {
//			if (numScanLines >= maxScanLines) {
//				return;
//			} else {
//				++numScanLines;
//			}
//		}
		line_width = xMax - xMin;

		horizontal_dcam_x = (cam_x_right - cam_x_left) / line_width;
		cam_x_value = cam_x_left;

		horizontal_dcam_y = (cam_y_right - cam_y_left) / line_width;
		cam_y_value = cam_y_left;

		horizontal_dcam_z = (cam_z_right - cam_z_left) / line_width;
		cam_z_value = cam_z_left;

		horizontal_dnorm_x = (norm_x_right - norm_x_left) / line_width;
		norm_x_value = norm_x_left;

		horizontal_dnorm_y = (norm_y_right - norm_y_left) / line_width;
		norm_y_value = norm_y_left;

		horizontal_dnorm_z = (norm_z_right - norm_z_left) / line_width;
		norm_z_value = norm_z_left;

		xPos = xMin;
		while (xPos < xMax) {
			/*
			interpolateNormal(top, xLeft, (float)xPos, (float)yPos, xRight, mid, bottom, &surface_normal);
			cam_x = interpolateComponent(top->getX(), mid->getX(), bottom->getX(),
					top->screen_y, (float)yPos, mid->screen_y, bottom->screen_y,
					xLeft, (float)xPos, xRight);
			cam_y = interpolateComponent(top->getY(), mid->getY(), bottom->getY(),
					top->screen_y, (float)yPos, mid->screen_y, bottom->screen_y,
					xLeft, (float)xPos, xRight);
			cam_z = interpolateComponent(top->getZ(), mid->getZ(), bottom->getZ(),
					top->screen_y, (float)yPos, mid->screen_y, bottom->screen_y,
					xLeft, (float)xPos, xRight);

			drawPoint2iWithShading(xPos, yPos,
					cam_x, cam_y, cam_z,
					&surface_normal,
					color3f
			);
			*/
			VECTOR3D_INITXYZ(&surface_normal, norm_x_value, norm_y_value, norm_z_value);

			drawPoint2iWithShading(xPos, yPos,
					cam_x_value, cam_y_value, cam_z_value,
					&surface_normal,
					color3f
					);

			++xPos;

			cam_x_value += horizontal_dcam_x;
			cam_y_value += horizontal_dcam_y;
			cam_z_value += horizontal_dcam_z;

			norm_x_value += horizontal_dnorm_x;
			norm_y_value += horizontal_dnorm_y;
			norm_z_value += horizontal_dnorm_z;
		}

		++yPos;

		cam_x_left += left_dcam_x;
		cam_x_right += right_dcam_x;
		cam_y_left += left_dcam_y;
		cam_y_right += right_dcam_y;
		cam_z_left += left_dcam_z;
		cam_z_right += right_dcam_z;

		norm_x_left += left_dnorm_x;
		norm_x_right += right_dnorm_x;
		norm_y_left += left_dnorm_y;
		norm_y_right += right_dnorm_y;
		norm_z_left += left_dnorm_z;
		norm_z_right += right_dnorm_z;
	}
}

string getModelPath(int modelIndex) {
	return "data/" + modelFiles[modelIndex];
}

void loadModel(int modelIndex) {
	if (collection == NULL) {

	} else {
		delete collection;
	}
	collection = parser.parseFile(getModelPath(modelIndex));
}

void main_loop_function()
{
	//vector<MATRIX4X4> matrices;
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
	vector<RzTriangle> triangles;
	float slopeTopMid;
	float slopeTopBottom;
	float slopeMidBottom;

	// figure out which side is left and right
	// compare deltas
	float deltaTopMid;
	float deltaTopBottom;
	float deltaMidBottom;
	float scale_factor = 1.0f;
	float scale_amount = 0.05f;
	VECTOR4D vertex_vector, result_vector, normal_vector;
	MATRIX4X4 normal_transform_matrix,
		model_transform_matrix,
		result_4x4,
		transform_matrix,
		perspective_transform_matrix,
		light_transform_matrix;

	bool draw_triangles = false;

	//int lastNumScanLines = 0;

	float move_amount = 0.5f;
	float rotation = 0.0f;
	float rotation_amount = 0.05f;
	float cos_rot;
	float sin_rot;

	float angle_amount = 0.05f;

	float translate_x = 0.0f;
	float translate_y = 0.0f;
	float translate_z = 0.0f;
	float translate_amount = 0.5;

	//int mycount = RAND_RANGE(0, 30);

	light_color.setRed(1.0f);
	light_color.setGreen(1.0f);
	light_color.setBlue(1.0f);

	ambient_light.setRed(0.4f);
	ambient_light.setGreen(0.4f);
	ambient_light.setBlue(0.5f);

	specular_highlight.setRed(1.0f);
	specular_highlight.setGreen(1.0f);
	specular_highlight.setBlue(1.0f);

	while(events())
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);
		//glLoadIdentity();
		//glTranslatef(0,0, -10);
		//glRotatef(angle, 0, 0, 1);
		//glBegin(GL_POINTS);
		//glBegin(GL_TRIANGLES);
//		glBegin(GL_QUADS);
//		glColor3ub(255, 000, 000); glVertex2f(200, 300);
//		glColor3ub(000, 255, 000); glVertex2f(500,  300);
//		glColor3ub(000, 000, 255); glVertex2f(500, 100);
//		glColor3ub(255, 255, 000); glVertex2f(200, 100);
//		glEnd();
//
//		glBegin(GL_TRIANGLES);
//		glColor3f(1.0, 0, 0);
//		glVertex2f(200, 200);
//		glColor3f(0, 1.0, 0);
//		glVertex2f(500, 300);
//		glColor3f(0, 0, 1.0);
//		glVertex2f(300, 600);
//		glEnd();
//
//		SDL_GL_SwapBuffers();
//
//		continue;


		//numPointPlots = 0;
		//numScanLines = 0;

		// initialize the buffers
		for (x = 0; x < WINDOW_WIDTH; ++x) {
			for (y = 0; y < WINDOW_HEIGHT; ++y) {
				zBufferSet[x][y] = false;
				colorBuffer[x][y].setRed(0.6f);
				colorBuffer[x][y].setGreen(0.6f);
				colorBuffer[x][y].setBlue(1.0f);
			}
		}

		// create the perspectified version of the model
		perspectified = new RzPolygonGroupCollection(*collection);

		//matrices.clear();
		// model transform init
		MAT_IDENTITY_4X4(&model_transform_matrix);
		// normal transform init
		MAT_IDENTITY_4X4(&normal_transform_matrix);
		// light transform init
		MAT_IDENTITY_4X4(&light_transform_matrix);
		// perspective transform matrix
		MAT_IDENTITY_4X4(&perspective_transform_matrix);

		// scale transform
		// model scale
		Mat_Init_4X4(&transform_matrix,
				scale_factor, 0, 0, 0,
				0, scale_factor, 0, 0,
				0, 0, scale_factor, 0,
				0, 0, 0, 1
				);

		Mat_Mul_4X4(&transform_matrix, &model_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &model_transform_matrix);

		// don't scale the normals

		// rotation
		// model rotation
		cos_rot = cos(rotation);
		sin_rot = sin(rotation);

		Mat_Init_4X4(&transform_matrix,
				cos_rot, 0, sin_rot, 0,
				0, 1, 0, 0,
				-sin_rot, 0, cos_rot, 0,
				0, 0, 0, 1);

		Mat_Mul_4X4(&transform_matrix, &model_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &model_transform_matrix);

		// normal rotation
		Mat_Mul_4X4(&transform_matrix, &normal_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &normal_transform_matrix);

		// translation
		// translate model
		Mat_Init_4X4(&transform_matrix,
				1, 0, 0, translate_x,
				0, 1, 0, translate_y,
				0, 0, 1, translate_z,
				0, 0, 0, 1
				);

		Mat_Mul_4X4(&transform_matrix, &model_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &model_transform_matrix);

		// don't translate normals

		// view transformations
		// translate eye to origin
		Mat_Init_4X4(&transform_matrix,
				1, 0, 0, -camera_position.x,
				0, 1, 0, -camera_position.y,
				0, 0, 1, -camera_position.z,
				0, 0, 0, 1);

		Mat_Mul_4X4(&transform_matrix, &model_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &model_transform_matrix);

		// translate light sources
		Mat_Mul_4X4(&transform_matrix, &light_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &light_transform_matrix);

		// calculate u, v, w vectors
		VECTOR3D w = VECTOR3D_Sub(&camera_position, &camera_target);
		VECTOR3D_Normalize(&w);

		VECTOR3D_Normalize(&camera_up);
		VECTOR3D u = VECTOR3D_Cross(&camera_up, &w);
		VECTOR3D_Normalize(&u);

		VECTOR3D v = VECTOR3D_Cross(&w, &u);
		VECTOR3D_Normalize(&v);

		// perform change of basis
		Mat_Init_4X4(&transform_matrix,
					u.x, u.y, u.z, 0,
					v.x, v.y, v.z, 0,
					w.x, w.y, w.z, 0,
					0, 0, 0, 1);

		Mat_Mul_4X4(&transform_matrix, &model_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &model_transform_matrix);

		// for normals too
		Mat_Mul_4X4(&transform_matrix, &normal_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &normal_transform_matrix);

		// for light source too
		Mat_Mul_4X4(&transform_matrix, &light_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &light_transform_matrix);


		// perspective transform
		Mat_Init_4X4(&transform_matrix,
				-near_z, 0, 0, 0,
				0, near_z, 0, 0,
				//0, 0, near_z + far_z, -(far_z * near_z),
				0, 0, 0, 0,
				0, 0, 1, 0);
		Mat_Mul_4X4(&transform_matrix, &perspective_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &perspective_transform_matrix);

		// not for normals or light source

		// calculate canonical view boundaries
		float can_top = tan(field_of_view / 2.0f) * near_z;
		float can_bottom = -can_top;
		float can_left = can_bottom;
		float can_right = can_top;

		// canonical view transform
		Mat_Init_4X4(&transform_matrix,
				2.0f / (can_right - can_left), 0, 0, -((can_right + can_left) / (can_right - can_left)),
				0, 2.0f / (can_top - can_bottom), 0, -((can_top + can_bottom) / (can_top - can_bottom)),
				//0, 0, 2.0f / (near_z - far_z), -((near_z + far_z) / (near_z - far_z)),
				0, 0, 0, 0,
				0, 0, 0, 1);
		Mat_Mul_4X4(&transform_matrix, &perspective_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &perspective_transform_matrix);

		// viewport transform
		Mat_Init_4X4(&transform_matrix,
				(float)WINDOW_WIDTH / 2.0f, 0, 0, (float)(WINDOW_WIDTH - 1) / 2.0f,
				0, (float)WINDOW_HEIGHT / 2.0f, 0, (float)(WINDOW_HEIGHT - 1) / 2.0f,
				0, 0, 1, 0,
				0, 0, 0, 1);
		Mat_Mul_4X4(&transform_matrix, &perspective_transform_matrix, &result_4x4);
		MAT_COPY_4X4(&result_4x4, &perspective_transform_matrix);

		// end matrix chains

		//matrices.push_back(translate_matrix);

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

		// transform the light source
		VECTOR4D_INITXYZ(&vertex_vector, light_pos.x,
				light_pos.y, light_pos.z);
		Mat_Mul_4X4_VECTOR4D(&light_transform_matrix, &vertex_vector, &relative_light_pos);

		/*
		// draw the light source in the view
		// perform perspective transform on vertices
		VECTOR4D_INITXYZ(&vertex_vector, relative_light_pos.x,
				relative_light_pos.y, relative_light_pos.z);
		Mat_Mul_4X4_VECTOR4D(&perspective_transform_matrix, &vertex_vector, &result_vector);

		// homogeneous divide
		VECTOR4D_DIV_BY_W(&result_vector);

		drawPoint2iForReal(intify(result_vector.x), intify(result_vector.y), relative_light_pos.z,
				&light_color);
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

				// perform transformations
				for (vertexIndex = 0; vertexIndex < polygon->vertices.size(); ++vertexIndex) {
					vertex3f = &polygon->vertices[vertexIndex];

					// perform matrix transforms on vertices (not including perspective)
					VECTOR4D_INITXYZ(&vertex_vector, vertex3f->getX(), vertex3f->getY(), vertex3f->getZ());
					Mat_Mul_4X4_VECTOR4D(&model_transform_matrix, &vertex_vector, &result_vector);

					vertex3f->setX(result_vector.x);
					vertex3f->setY(result_vector.y);
					vertex3f->setZ(result_vector.z);

					// transform the normal vectors
					VECTOR4D_INITXYZ(&normal_vector, vertex3f->getOrthoX(), vertex3f->getOrthoY(), vertex3f->getOrthoZ());
					Mat_Mul_4X4_VECTOR4D(&normal_transform_matrix, &normal_vector, &result_vector);

					// normalize normal vectors
					VECTOR4D_Normalize(&normal_vector);

					vertex3f->setOrthoX(result_vector.x);
					vertex3f->setOrthoY(result_vector.y);
					vertex3f->setOrthoZ(result_vector.z);

					// perform perspective transform on vertices (but keep original camera space coordinates)
					VECTOR4D_INITXYZ(&vertex_vector, vertex3f->getX(), vertex3f->getY(), vertex3f->getZ());
					Mat_Mul_4X4_VECTOR4D(&perspective_transform_matrix, &vertex_vector, &result_vector);

					// homogeneous divide
					VECTOR4D_DIV_BY_W(&result_vector);

					vertex3f->screen_x = result_vector.x;
					vertex3f->screen_y = result_vector.y;

					//drawPoint2i(vertex3f->screen_x, vertex3f->screen_y, vertex3f->getZ(), color3f);
				}

				triangles = polygon->getTriangles();

				// get the triangles
				for (triangleIndex = 0; triangleIndex < triangles.size(); ++triangleIndex) {

						// draw the triangle vertices
					triangle = &triangles[triangleIndex];
					// scan line convert

//					glBegin(GL_TRIANGLES);
//					glColor3f(1.0, 0, 0);
//					glVertex2f(intify(triangle->vertices[0].screen_x),
//							intify(triangle->vertices[0].screen_y));
//					glColor3f(0, 1.0, 0);
//					glVertex2f(intify(triangle->vertices[1].screen_x),
//							intify(triangle->vertices[1].screen_y));
//					glColor3f(0, 0, 1.0);
//					glVertex2f(intify(triangle->vertices[2].screen_x),
//							intify(triangle->vertices[2].screen_y));
//					glEnd();
//
//					break;


					// get bottom, mid, top vertices
					bottomVertex = &triangle->vertices[0];
					midVertex = &triangle->vertices[1];
					topVertex = &triangle->vertices[2];

					if (midVertex->screen_y > bottomVertex->screen_y) {
						swapPointers((void**)&midVertex, (void**)&bottomVertex);
					}
					if (topVertex->screen_y > midVertex->screen_y) {
						swapPointers((void**)&topVertex, (void**)&midVertex);

						if (midVertex->screen_y > bottomVertex->screen_y) {
							swapPointers((void**)&midVertex, (void**)&bottomVertex);
						}
					}

					if (topVertex->screen_y >= WINDOW_HEIGHT) {
						continue;
					}
					if (bottomVertex->screen_y < 0) {
						continue;
					}
					if (topVertex->screen_x < 0 &&
							midVertex->screen_x < 0 &&
							bottomVertex->screen_x < 0) {
						continue;
					}
					if (topVertex->screen_x >= WINDOW_WIDTH &&
							midVertex->screen_x >= WINDOW_WIDTH &&
							bottomVertex->screen_x >= WINDOW_WIDTH) {
						continue;
					}


					// sort vertices with same y
					if (intify(topVertex->screen_y) == intify(bottomVertex->screen_y)) {
						// perfectly flat line
						// sort by x value
						if (intify(midVertex->screen_x) > intify(bottomVertex->screen_x)) {
							swapPointers((void**)&midVertex, (void**)&bottomVertex);
						}
						if (intify(topVertex->screen_x) > intify(midVertex->screen_x)) {
							swapPointers((void**)&topVertex, (void**)&midVertex);

							if (intify(midVertex->screen_x) > intify(bottomVertex->screen_x)) {
								swapPointers((void**)&midVertex, (void**)&bottomVertex);
							}
						}
					} else if (intify(topVertex->screen_y) == intify(midVertex->screen_y)) {
						// flat on top
						if (intify(topVertex->screen_x) > intify(midVertex->screen_x)) {
							swapPointers((void**)&topVertex, (void**)&midVertex);
						}
					} else if (intify(midVertex->screen_y) == intify(bottomVertex->screen_y)) {
						// flat on bottom
						if (intify(bottomVertex->screen_x) > intify(midVertex->screen_x)) {
							swapPointers((void**)&bottomVertex, (void**)&midVertex);
						}
					}

					if (draw_triangles) {
						glBegin(GL_TRIANGLES);
						glColor3f(1.0f, 0.0f, 0.0f);
						glVertex2f(intify(bottomVertex->screen_x),
								intify(bottomVertex->screen_y));
						glColor3f(0, 1.0, 0);
						glVertex2f(intify(midVertex->screen_x),
								intify(midVertex->screen_y));
						glColor3f(0, 0, 1.0);
						glVertex2f(intify(topVertex->screen_x),
								intify(topVertex->screen_y));
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
							if (intify(topVertex->screen_x) < intify(midVertex->screen_x)) {
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
							if (intify(bottomVertex->screen_x) > intify(midVertex->screen_x)) {
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
								if (intify(bottomVertex->screen_x) > intify(midVertex->screen_x)) {
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
							if (intify(bottomVertex->screen_x) > intify(midVertex->screen_x)) {
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

//							bool display = false;
//							if ((numScanLines + 1) == maxScanLines && lastNumScanLines != numScanLines) {
//								ss.clear();
//								display = true;
//								ss << "deltaTopMid: " << deltaTopMid << " deltaTopBottom: " << deltaTopBottom;
//								ss << " top: " << intify(topVertex->screen_x) << ", " << intify(topVertex->screen_y);
//								ss << " mid: " << intify(midVertex->screen_x) << ", " << intify(midVertex->screen_y);
//								ss << " bottom: " << intify(bottomVertex->screen_x) << ", " << intify(bottomVertex->screen_y);
//								ss << endl;
//								Debugger::getInstance().print(ss.str());
//								lastNumScanLines = numScanLines;
//							}

							if (deltaTopMid < deltaTopBottom) {
//								if (display) {
//									ss.clear();
//									ss << "deltaTopMid < deltaTopBottom" << endl;
//									Debugger::getInstance().print(ss.str());
//								}
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
//							if (display) {
//								ss.clear();
//								ss << "isfinite(bottomRightSlope): " << isfinite(bottomRightSlope);
//								ss << " ";
//								ss << "isfinite(bottomLeftSlope): " << isfinite(bottomLeftSlope);
//								ss << endl;
//							}

							fillFlatTop(topVertex, midVertex, bottomVertex, topLeft, topRight,
									topLeftSlope, topRightSlope, color3f);
							fillFlatBottom(topVertex, midVertex, bottomVertex, bottomLeft, bottomRight,
									bottomLeftSlope, bottomRightSlope, color3f);
						}
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

				//glVertex3f((float)x, (float)y, 0.5);
				glVertex2f((float)x, (float)y);

			}
		}
		glEnd();

		SDL_GL_SwapBuffers();
		// Check keypresses
		if(key[SDLK_RIGHT]) {
			//++maxPointPlots;
			//++maxScanLines;
			//angle-=0.5;

			// move camera around origin
			//angle += angle_amount;

			cos_rot = cos(angle_amount);
			sin_rot = sin(angle_amount);

			MATRIX3X3 rotate_matrix = {
					cos_rot, 0, sin_rot,
					0, 1, 0,
					-sin_rot, 0, cos_rot};

			VECTOR3D result_3d;

			Mat_Mul_3X3_VECTOR3D(&rotate_matrix, &camera_position, &result_3d);
			VECTOR3D_COPY(&camera_position, &result_3d);
		}
		if(key[SDLK_LEFT]) {
			//--maxPointPlots;
			//--maxScanLines;
			//angle+=0.5;
			//rotation += 0.05;

			// move camera around origin
			//angle -= angle_amount;

			cos_rot = cos(-angle_amount);
			sin_rot = sin(-angle_amount);

			MATRIX3X3 rotate_matrix = {
					cos_rot, 0, sin_rot,
					0, 1, 0,
					-sin_rot, 0, cos_rot};

			VECTOR3D result_3d;

			Mat_Mul_3X3_VECTOR3D(&rotate_matrix, &camera_position, &result_3d);
			VECTOR3D_COPY(&camera_position, &result_3d);
		}
		if (key[SDLK_UP]) {
			//zAdd += 10.0;
			//Debugger::getInstance().print("up key");

			// move the camera toward the camera target
			VECTOR3D target_vector = { camera_target.x - camera_position.x,
					camera_target.y - camera_position.y,
					camera_target.z - camera_position.z };
			VECTOR3D_Normalize(&target_vector);
			VECTOR3D_Scale(move_amount, &target_vector);
			VECTOR3D_Add(&target_vector, &camera_position, &camera_position);
		}
		if (key[SDLK_DOWN]) {
			//zAdd -= 10.0;
			//Debugger::getInstance().print("down key");
			VECTOR3D target_vector = { camera_target.x - camera_position.x,
					camera_target.y - camera_position.y,
					camera_target.z - camera_position.z };
			VECTOR3D_Normalize(&target_vector);
			VECTOR3D_Scale(-move_amount, &target_vector);
			VECTOR3D_Add(&target_vector, &camera_position, &camera_position);
		}
		if (key[SDLK_a]) {
			rotation += rotation_amount;
		}
		if (key[SDLK_s]) {
			rotation -= rotation_amount;
		}
		if (key[SDLK_z]) {
			scale_factor -= scale_amount;
		}
		if (key[SDLK_x]) {
			scale_factor += scale_amount;
		}
		if (key[SDLK_u]) {
			/*
			ss.clear();
			ss << "translate toward negative x: " << translate_x << endl;
			Debugger::getInstance().print(ss.str());
			*/
			translate_x -= translate_amount;
		}
		if (key[SDLK_j]) {
			/*
			ss.clear();
			ss << "translate toward positive x: " << translate_x << endl;
			Debugger::getInstance().print(ss.str());
			*/
			translate_x += translate_amount;
		}
		if (key[SDLK_k]) {
			translate_y -= translate_amount;
		}
		if (key[SDLK_i]) {
			translate_y += translate_amount;
		}
		if (key[SDLK_o]) {
			translate_z -= translate_amount;
		}
		if (key[SDLK_l]) {
			translate_z += translate_amount;
		}
		if (key[SDLK_t]) {
			draw_triangles = !draw_triangles;
		}
		if (key[SDLK_n]) {
			if (++currentModelIndex > numModels - 1) {
				currentModelIndex = 0;
			}
			loadModel(currentModelIndex);
		}
		if (key[SDLK_b]) {
			if (--currentModelIndex < 0) {
				currentModelIndex = numModels - 1;
			}
			loadModel(currentModelIndex);
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
	unsigned int polygonGroupIndex;
	unsigned int polygonIndex;
	unsigned int vertexIndex;
	RzPolygonGroup *polygonGroup;
	RzPolygon *polygon;
	RzVertex3f *vertex3f;
	float boundingXMin, boundingXMax, boundingYMin, boundingYMax;
	float boundingWidth;
	float boundingHeight;
	float scaleFactor;
	stringstream ss;
	bool first;
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

	loadModel(currentModelIndex);

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

	boundingXMin -= 0.05 * (boundingXMax - boundingXMin);
	boundingXMax += 0.05 * (boundingXMax - boundingXMin);

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

	if (collection == NULL) {

	} else {
		delete collection;
	}

	return 0;
}
