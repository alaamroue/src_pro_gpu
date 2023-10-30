/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */


// Includes
#include <string>
#include <locale>
#include <math.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include "common.h"

namespace Util 
{
	/*
	 *  Convert a double with seconds to a meaningful time format
	 */
	std::string		secondsToTime( double dTime )
	{
		std::string sTime;

		// No negative times...
		if ( dTime < 0 ) dTime = 0.0;

		// Identify the different components of the time
		float		  fFractionSeconds	= static_cast<float>( std::fmod( dTime, 1 ) );
		unsigned char cSeconds			= static_cast<unsigned char>( std::floor( std::fmod( dTime, 60      ) ) );
		unsigned char cMinutes			= static_cast<unsigned char>( std::floor( std::fmod( dTime, 3600    )/60 ) );
		unsigned char cHours			= static_cast<unsigned char>( std::floor( std::fmod( dTime, 86400   )/3600 ) );
		unsigned int  uiDays			= static_cast<unsigned int>(  std::floor( dTime / 86400 ) );

		char	cTime[50]			 = "";
		char	cFractionSeconds[50] = "";

		if ( uiDays > 0 )
		{
			sTime = toStringExact( uiDays ) + " d ";
		}

		if ( dTime > 1 )
		{
			// Normal format including miliseconds when less than 10 minutes
			sprintf( cTime,			"%02d:%02d:%02d", cHours, cMinutes, cSeconds );
			sprintf( cFractionSeconds, ".%.4f",	  fFractionSeconds );
			sTime += std::string( cTime );
			if ( fFractionSeconds > 0.0 && cMinutes < 10 && cHours < 1 && uiDays < 1 ) 
				sTime += std::string( cFractionSeconds ).substr(2);
		} else {
			sprintf( cTime,				"%01d",		cSeconds );
			sprintf( cFractionSeconds,  ".%.5f",	fFractionSeconds );
			sTime += std::string( cTime );
			sTime += std::string( cFractionSeconds ).substr(2) + "s";
		}

		return sTime;
	}

	/*
	 *  Round a number to set decimal places
	 */
	double	round( double dValue, unsigned int ucPlaces )
	{
		unsigned int	uiMultiplier		= (unsigned int) std::pow( 10.0, ucPlaces );
		double			dMultipliedValue	= dValue * uiMultiplier;
		double			dRemainder			= std::fmod( dMultipliedValue, 1 );

		if ( dRemainder >= 0.5 )
		{
			dMultipliedValue = std::ceil( dMultipliedValue );
		} else {
			dMultipliedValue = std::floor( dMultipliedValue );
		}

		return dMultipliedValue / uiMultiplier;
	}




	std::string to_string_exact(double x) {
		std::ostringstream os;
		os << std::setprecision(std::numeric_limits<double>::max_digits10) << x;
		return os.str();
	}

}

