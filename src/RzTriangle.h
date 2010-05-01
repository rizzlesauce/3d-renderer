/*
 * RzTriangle.h
 *
 *  Created on: May 1, 2010
 *      Author: ross
 */

#ifndef RZTRIANGLE_H_
#define RZTRIANGLE_H_

#include "RzVertex3f.h"

class RzTriangle {
public:
	RzTriangle();
	virtual ~RzTriangle();

	RzVertex3f vertices[3];
};

#endif /* RZTRIANGLE_H_ */
