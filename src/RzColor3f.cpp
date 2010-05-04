/*
 * RzColor3f.cpp
 *
 *  Created on: Apr 29, 2010
 *      Author: ross
 */

#include "RzColor3f.h"
#include <cmath>

RzColor3f::RzColor3f() {
	// TODO Auto-generated constructor stub
	red = &components[0];
	green = &components[1];
	blue = &components[2];
}

RzColor3f::~RzColor3f() {
	// TODO Auto-generated destructor stub
}

float RzColor3f::getRed() {
	return components[0];
}

float RzColor3f::getGreen() {
	return components[1];
}

float RzColor3f::getBlue() {
	return components[2];
}

unsigned int RzColor3f::getScaledRed() {
	return floor(0.5 + (getRed() * 255.0));
}

unsigned int RzColor3f::getScaledGreen() {
	return floor(0.5 + (getGreen() * 255.0));
}

unsigned int RzColor3f::getScaledBlue() {
	return floor(0.5 + (getBlue() * 255.0));
}

void RzColor3f::setRed(float f) {
	components[0] = f;
}

void RzColor3f::setGreen(float g) {
	components[1] = g;
}

void RzColor3f::setBlue(float b) {
	components[2] = b;
}
