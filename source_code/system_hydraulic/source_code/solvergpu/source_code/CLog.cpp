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

// Constructor
CLog::CLog(CLoggingInterface* externalLogger_input){
	externalLogger = nullptr;

	//Check and attach to external logging functions
	if (externalLogger_input != nullptr) {
		default = false;
		externalLogger = externalLogger_input;
	}
	else {
		default = true;
	}

	setlocale( LC_ALL, "" );
	this->logInfo("Log component fully loaded.");
}

// Destructor
CLog::~CLog(void){
	if (this->externalLogger != NULL)
		delete this->externalLogger;
}

// Setup details of an error that's occurred and send them to the logger to be outputted. Actual handling the error is conducted in the main sub-procedure.
void CLog::writeError( std::string sError, unsigned char cError ){
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
	this->logError(sError, sErrorPrefix);
}

//Write a line to divide up the output, purely superficial
void CLog::writeDivide()
{
	this->logInfo( "---------------------------------------------                           " );
}

//Actual outputting of debug message to user
void CLog::logDebug(const std::string& message) {
	if (default) {
		std::cout << "[DEBUG]: " << message << std::endl;
	}
	else {
		externalLogger->logDebug(message);
	}
}

//Actual outputting of info message to user
void CLog::logInfo(const std::string& message) {
	if (default) {
		std::cout << "[INFO]: " << message << std::endl;
	}
	else {
		externalLogger->logInfo(message);
	}
}

//Actual outputting of warning message to user
void CLog::logWarning(const std::string& message) {
	if (default) {
		std::cout << "[WARN]: " << message << std::endl;
	}
	else {
		externalLogger->logWarning(message);
	}
}

//Actual outputting of error message to user
void CLog::logError(const std::string& message, const std::string& errPrefix) {
	if (default) {
		std::cout << "---------------------------------------------" << std::endl;
		std::cout << "[ERR]: " << "[" << errPrefix << "] " << message << std::endl;
		std::cout << "---------------------------------------------" << std::endl;
	}
	else {
		externalLogger->logError(message, errPrefix);
	}
}

//Actual outputting of error message to user
void CLog::writeCharToFile(char* code, const char* filename, bool addTime) {

	std::string fullFilename = filename;

	if (addTime) {
		time_t t = time(0);   // get time now
		struct tm* now = localtime(&t);

		char timeBuffer[80];
		strftime(timeBuffer, 80, "%Y-%m-%d-%H-%M-%S-", now);


		std::string fullFilename = std::string(timeBuffer) + filename;
	}

	std::ofstream outputFile(fullFilename.c_str());

	if (outputFile.is_open()) {
		outputFile << code << std::endl;
		outputFile.close();
	}
	else {
		std::cerr << "Failed to open the file for writing." << std::endl;
	}

}