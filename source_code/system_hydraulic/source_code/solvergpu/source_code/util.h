/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

#ifndef HIPIMS_UTIL_H_
#define HIPIMS_UTIL_H_

// Includes
#include <sstream>
#include "CL/opencl.h"

// Structure definitions
struct cursorCoords {
	int	sX;
	int	sY;
};

/*
 *  FUNCTIONS
 */
namespace Util
{
	// String conversions
	std::string		secondsToTime(double);

	// Resource handling
	char*			getFileResource(const char*, const char*);
	//cursorCoords	getCursorPosition();
	void			getHostname(char*);
	//void			setCursorPosition(cursorCoords);
	double			round(double, unsigned int);
	std::string     to_string_exact(double);

}

#endif
