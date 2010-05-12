/*
 * RZVertex3f.cpp
 *
 *  Created on: Apr 29, 2010
 *      Author: ross
 */

#include "RzVertex3f.h"

RzVertex3f::RzVertex3f() {
	// TODO Auto-generated constructor stub
}

RzVertex3f::RzVertex3f(const RzVertex3f& other) {
	deepCopy(other);
}

RzVertex3f::~RzVertex3f() {
	// TODO Auto-generated destructor stub
}

void RzVertex3f::deepCopy(const RzVertex3f& other) {

	for (unsigned int i = 0; i < 6; ++i) {
		coordinates[i] = other.coordinates[i];
	}
	/*
	setX(other.getX());
	setY(other.getY());
	setZ(other.getZ());
	setOrthoX(other.getOrthoX());
	setOrthoY(other.getOrthoY());
	setOrthoZ(other.getOrthoZ());
	*/
}

float RzVertex3f::getX() const {
	return coordinates[0];
}

float RzVertex3f::getY() const {
	return coordinates[1];
}

float RzVertex3f::getZ() const {
	return coordinates[2];
}

float RzVertex3f::getOrthoX() const {
	return coordinates[3];
}

float RzVertex3f::getOrthoY() const {
	return coordinates[4];
}

float RzVertex3f::getOrthoZ() const {
	return coordinates[5];
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

void RzVertex3f::setOrthoX(float p_x) {
	coordinates[3] = p_x;
}

void RzVertex3f::setOrthoY(float p_y) {
	coordinates[4] = p_y;
}

void RzVertex3f::setOrthoZ(float p_z) {
	coordinates[5] = p_z;
}
