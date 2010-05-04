/*
 * RZVertex3f.cpp
 *
 *  Created on: Apr 29, 2010
 *      Author: ross
 */

#include "RzVertex3f.h"

RzVertex3f::RzVertex3f() {
	// TODO Auto-generated constructor stub
	x = &coordinates[0];
	y = &coordinates[1];
	z = &coordinates[2];
}

RzVertex3f::~RzVertex3f() {
	// TODO Auto-generated destructor stub
}

float RzVertex3f::getX() {
	return coordinates[0];
}

float RzVertex3f::getY() {
	return coordinates[1];
}

float RzVertex3f::getZ() {
	return coordinates[2];
}

void RzVertex3f::setX(float p_x) {
	coordinates[0] = p_x;
}

void RzVertex3f::setY(float p_y) {
	coordinates[1] = p_y;
}

void RzVertex3f::setZ(float p_z) {
	coordinates[2] = p_z;
}
