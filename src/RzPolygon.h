/*
 * RzPolygon.h
 *
 *  Created on: Apr 29, 2010
 *      Author: ross
 */

#ifndef RZPOLYGON_H_
#define RZPOLYGON_H_

#include <vector>
#include "RzVertex3f.h"
using namespace std;

class RzPolygon {
public:
	RzPolygon();
	virtual ~RzPolygon();

	vector<RzVertex3f> vertices;
};

#endif /* RZPOLYGON_H_ */
