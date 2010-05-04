/*
 * RZVertex3f.h
 *
 *  Created on: Apr 29, 2010
 *      Author: ross
 */

#ifndef RZVERTEX3F_H_
#define RZVERTEX3F_H_

class RzVertex3f {
public:
	RzVertex3f();
	virtual ~RzVertex3f();
	float *x, *y, *z;

	float getX();
	float getY();
	float getZ();

	void setX(float);
	void setY(float);
	void setZ(float);

	float coordinates[6];
};

#endif /* RZVERTEX3F_H_ */
