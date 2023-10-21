/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */

// Includes
#include "common.h"
#include <sstream>

/*
 *  Constructor
 */
CLog::CLog(void)
{
	this->setDir();
	this->setPath();
	this->openFile();
	//this->writeHeader();
	this->uiDebugFileID = 1;
	this->uiLineCount = 0;
	setlocale( LC_ALL, "" );

	this->writeLine( "Log component fully loaded." );
}

/*
 *  Destructor
 */
CLog::~CLog(void)
{
	this->closeFile();
	delete[] this->logDir;
	delete[] this->logPath;
}

/*
 *  Open the file for write access
 */
void CLog::openFile()
{
	if ( this->isFileAvailable() )
		return;

	try 
	{
		this->logStream.open( this->logPath );
	} 
	catch ( char * cError )
	{
		this->writeError( std::string( cError ), model::errorCodes::kLevelWarning );
	}
}

/*
 *  Is the log available for write
 */
bool CLog::isFileAvailable()
{
	// Nothing fancy, just abstracted for portability
	return this->logStream.is_open();
}

/*
 *  Close the file safelty again
 */
void CLog::closeFile()
{
	if ( !this->isFileAvailable() ) 
		return;

	this->logStream.flush();
	this->logStream.close();
}

/*
 *  Erase the contents of the log file (to start again)
 */
void CLog::clearFile()
{
	// Erase the file
	// ...
}

/*
 *  OVERLOAD FUNCTIONS
 *  Write a single line to the log with no 2nd parameter
 */
// Include timestamp by default
void CLog::writeLine( std::string sLine ) { this->writeLine( sLine, true ); }
void CLog::writeLine( std::string sLine, bool bTimestamp ) { this->writeLine( sLine, bTimestamp, model::cli::colourMain ); }

/*
 *  Write a single line to the log
 */
void CLog::writeLine(std::string sLine, bool bTimestamp, unsigned short wColour)
{
	time_t tNow;
	time(&tNow);

	char caTimeBuffer[50];
	strftime(caTimeBuffer, sizeof(caTimeBuffer), "%H:%M:%S", localtime(&tNow));

	std::stringstream ssLine;

	if (bTimestamp)
	{
		//this->setColour(model::cli::colourTimestamp);

		ssLine << "[" << std::string(caTimeBuffer) << "] ";
		
		std::cout << ssLine.str();
	}

	ssLine << sLine << std::endl;

	//this->resetColour();
	//this->setColour(wColour);

	std::cout << sLine << std::endl;

	sLine = ssLine.str();

	if (this->isFileAvailable())
	{
		this->logStream << sLine;
	}

	//this->resetColour();
}

/*
 *  Write details of an error that's occured 
 *  Actually handling the error is conducted in the main
 *  sub-procedure.
 */
void CLog::writeError( std::string sError, unsigned char cError )
{
	std::string sErrorPrefix = "UNKNOWN";

	if ( cError & model::errorCodes::kLevelFatal ) { 
		sErrorPrefix = "FATAL ERROR"; 
	} else if ( cError & model::errorCodes::kLevelModelStop	) { 
		sErrorPrefix = "MODEL FAILURE"; 
	} else if ( cError & model::errorCodes::kLevelModelContinue	) { 
		sErrorPrefix = "MODEL WARNING"; 
	} else if ( cError & model::errorCodes::kLevelWarning ) { 
		sErrorPrefix = "WARNING"; 
	} else if ( cError & model::errorCodes::kLevelInformation ) { 
		sErrorPrefix = "INFO"; 
	}

	this->writeLine( "---------------------------------------------", false, model::cli::colourError );
	this->writeLine( sErrorPrefix + ": " + sError, true, model::cli::colourError );
	this->writeLine( "---------------------------------------------", false, model::cli::colourError );
}

/*
 *  Write the header output
 */
void CLog::writeHeader(void)
{
	std::stringstream		ssHeader;

	time_t tNow;
	time( &tNow );
	localtime( &tNow );

	ssHeader << "---------------------------------------------" << std::endl;
	ssHeader << " " << model::appName << std::endl;
	ssHeader << " v" << model::appVersionMajor << "." << model::appVersionMinor << "." << model::appVersionRevision;
	ssHeader << std::endl << "---------------------------------------------" << std::endl;
	ssHeader << " " << model::appAuthor << std::endl;
	ssHeader << " " << model::appUnit << std::endl;
	ssHeader << " " << model::appOrganisation << std::endl;
	ssHeader << std::endl << " Contact:     " << model::appContact << std::endl;
	ssHeader << "---------------------------------------------" << std::endl;

	std::string sLogPath = this->getPath();

	if ( sLogPath.length() > 25 ) 
		sLogPath = "..." + sLogPath.substr( sLogPath.length() - 25, 25 );

	ssHeader << " Started:     " << ctime( &tNow );
	ssHeader << " Log file:    " << sLogPath << std::endl;
	ssHeader << " Platform:    " << model::env::platformName << std::endl;
	ssHeader << "---------------------------------------------";

	this->writeLine( ssHeader.str(), false, model::cli::colourHeader );
	ssHeader.clear();
}


#include <stdio.h>
#include <string.h>

/*
 *  Set the log path to the default
 */
void CLog::setPath()
{
	std::string sPath = "promaidesMultiScheme.log";
	char*		cPath = new char[ sPath.length() + 1 ];

	strcpy( cPath, sPath.c_str() );
	this->setPath( cPath, sPath.length() );

	delete[] cPath;
}

/*
 *  Set the log path to the given value
 */
void CLog::setPath( char* sPath, size_t uiLength )
{
	this->logPath = new char[ uiLength + 1 ];
	strcpy( this->logPath, sPath );

	// Is it already open? Swap if so
	// TODO: Implement this
	// ...
}

/*
 *  Return the path back
 */
std::string CLog::getPath()
{
	return this->logPath;
}

/*
 *  Set the log directory to the default
 */
void CLog::setDir()
{
	std::string sDir = "/";

	char*		cDir = new char[ sDir.length() + 1 ];
	strcpy( cDir, sDir.c_str() );

	this->setDir( cDir, sDir.length() );
	delete[] cDir;
}

/*
 *  Set the log directory to the given value
 */
void CLog::setDir( char* sDir, size_t stLength )
{
	this->logDir = new char[ stLength + 1 ];
	strcpy( this->logDir, sDir );
}

/*
 *  Return the directory back
 */
std::string CLog::getDir()
{
	return this->logDir;
}

/*
 *  Write a line to divide up the output, purely superficial
 */
void CLog::writeDivide()
{
	this->writeLine( "---------------------------------------------                           ", false );
}

/*
 *  Write a debug file to the log directory
 */
void CLog::writeDebugFile( char** cContents, unsigned int uiSegmentCount )
{
	std::ofstream ofsDebug;
	std::string sFilePath = this->getDir() + "_debug" + toStringExact( this->uiDebugFileID ) + ".log";

	try 
	{
		ofsDebug.open( sFilePath.c_str() );
	} 
	catch ( char * cError )
	{
		this->writeError( std::string( cError ), model::errorCodes::kLevelWarning );
		return;
	}

	for( unsigned int i = 0; i < uiSegmentCount; ++i )
		ofsDebug << cContents[ i ];

	ofsDebug.close();

	++this->uiDebugFileID;
}




