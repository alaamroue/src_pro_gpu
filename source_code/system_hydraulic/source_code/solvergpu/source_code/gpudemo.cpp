/*
 * Copyright (C) 2023 Alaa Mroue
 * This code is licensed under GPLv3. See LICENCE for more information.
 */

// Includes
#include "gpudemo.h"
#include "CModel.h"
#include "COCLDevice.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CMultiGpuManager.h"
#include "CDomainCartesian.h"
#include "CScheme.h"
#include "CProfiler.h"

// Globals
CLog*					model::log;

/*
 *  Application entry-point. 
 */
int main()
{
	CMultiGpuManager cMultiGpuManager;
	cMultiGpuManager.initManager();
	

	model::doPause();
	// Default configurations

	//int iReturnCode = model::loadConfiguration();
	//if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;
	//iReturnCode = model::commenceSimulation();
	//if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;
	//iReturnCode = model::closeConfiguration();
	//if ( iReturnCode != model::appReturnCodes::kAppSuccess ) return iReturnCode;

	//return iReturnCode;
}


/*
 *  Load the specified model config file and probe for devices etc.
 */
int model::loadConfiguration()
{
	CModel* pManager = new CModel();
	if (model::log == nullptr) {
		CLog* cLog = new CLog();
		model::log = cLog;
	}

	pManager->setLogger(model::log);

	CExecutorControl* pExecutor = CExecutorControl::createExecutor(model::executorTypes::executorTypeOpenCL, pManager);
	//pExecutor->setDeviceFilter(model::filters::devices::devicesCPU);
	pExecutor->setDeviceFilter(model::filters::devices::devicesGPU);
	//pExecutor->setDeviceFilter(model::filters::devices::devicesAPU);
	pExecutor->createDevices();
	pManager->setExecutor(pExecutor);

	pManager->setSelectedDevice(1);
	pManager->setName("Name");
	pManager->setDescription("Desc");
	pManager->setSimulationLength(3600.0);
	pManager->setOutputFrequency(3600.0);
	//pManager->setFloatPrecision(model::floatPrecision::kSingle);
	pManager->setFloatPrecision(model::floatPrecision::kDouble);


	pManager->getDomainSet()->setSyncMethod(model::syncMethod::kSyncTimestep);
	pManager->getDomainSet()->setSyncMethod(model::syncMethod::kSyncForecast);
	//pManager->getDomainSet()->setSyncBatchSpares(10);


	CDomainBase* pDomainNew;
	pDomainNew = CDomainBase::createDomain(model::domainStructureTypes::kStructureCartesian);
	static_cast<CDomain*>(pDomainNew)->setDevice(pManager->getExecutor()->getDevice(1));
	CDomainCartesian* ourCartesianDomain = (CDomainCartesian*) pDomainNew;

	ourCartesianDomain->setCellResolution(1,1);
	ourCartesianDomain->setCols(100);
	ourCartesianDomain->setRows(100);

	CScheme* pScheme;
	model::schemeTypes::schemeTypes mst = model::schemeTypes::kPromaidesScheme;
	pScheme = CScheme::createScheme(mst);

	pScheme->setQueueMode(model::queueMode::kAuto);
	pScheme->setQueueSize(1);

	model::SchemeSettings schemeSettings;
	schemeSettings.CourantNumber = 0.5;
	schemeSettings.DryThreshold = 1e-10;
	schemeSettings.Timestep = 0.01;
	schemeSettings.ReductionWavefronts = 200;
	schemeSettings.FrictionStatus = false;
	schemeSettings.CachedWorkgroupSize[0] = 8;
	schemeSettings.CachedWorkgroupSize[1] = 8;
	schemeSettings.NonCachedWorkgroupSize[0] = 8;
	schemeSettings.NonCachedWorkgroupSize[1] = 8;
	if (mst == model::schemeTypes::kGodunov) {
		schemeSettings.CacheMode = model::schemeConfigurations::godunovType::kCacheNone;
		schemeSettings.CacheConstraints = model::cacheConstraints::godunovType::kCacheActualSize;
	}
	else if (mst == model::schemeTypes::kMUSCLHancock) {
		schemeSettings.CacheMode = model::schemeConfigurations::musclHancock::kCacheNone;
		schemeSettings.CacheConstraints = model::cacheConstraints::musclHancock::kCacheActualSize;
	}
	else if (mst == model::schemeTypes::kInertialSimplification) {
		schemeSettings.CacheMode = model::schemeConfigurations::inertialFormula::kCacheNone;
		schemeSettings.CacheConstraints = model::cacheConstraints::inertialFormula::kCacheActualSize;
	}
	else if (mst == model::schemeTypes::kPromaidesScheme) {
		schemeSettings.CacheMode = model::schemeConfigurations::promaidesFormula::kCacheNone;
		schemeSettings.CacheConstraints = model::cacheConstraints::promaidesFormula::kCacheActualSize;
	}
	else {
		std::cout << "Error: Scheme not chosen!" << std::endl;
	}
	schemeSettings.ExtrapolatedContiguity = true;

	pScheme->setupScheme(schemeSettings, pManager);
	pScheme->setDomain(ourCartesianDomain);			// Scheme allocates the memory and thus needs to know the dimensions
	pScheme->prepareAll();							//Needs Dimension data to alocate memory
	ourCartesianDomain->setScheme(pScheme);

	unsigned long ulCellID;
	unsigned char	ucRounding = 5;
	for (unsigned long iRow = 0; iRow < ourCartesianDomain->getRows(); iRow++) {
		for (unsigned long iCol = 0; iCol < ourCartesianDomain->getCols(); iCol++) {
			ulCellID = ourCartesianDomain->getCellID(iCol, ourCartesianDomain->getRows() - iRow - 1);
			//Elevations
			ourCartesianDomain->handleInputData(ulCellID, pow(iRow* iRow + iCol * iCol,0.5), model::rasterDatasets::dataValues::kBedElevation, ucRounding);
			//Manning Coefficient
			ourCartesianDomain->handleInputData(ulCellID, 0.03, model::rasterDatasets::dataValues::kManningCoefficient, ucRounding);
			//Depth
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kDepth, ucRounding);
			//VelocityX
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityX, ucRounding);
			//VelocityY
			ourCartesianDomain->handleInputData(ulCellID, 0.0, model::rasterDatasets::dataValues::kVelocityY, ucRounding);
			//Boundary Condition
			ourCartesianDomain->setBoundaryCondition(ulCellID, 0.0001);
			//Poleni Condition
			ourCartesianDomain->setPoleniConditionX(ulCellID, 0.0001);
			//Coupling Condition
			//ourCartesianDomain->setCouplingCondition(ulCellID, 0.0);

		}
	}

	pDomainNew->setID(1);	// Should not be needed, but somehow is?
	pManager->getDomainSet()->getDomainBaseVector()->push_back(pDomainNew);

	return model::appReturnCodes::kAppSuccess;
}





/*
 *  Model is complete.
 */
int model::doClose( int iCode )
{
	model::doPause();

	return iCode;
}

/*
 *  Suspend the application temporarily pending the user
 *  pressing return to continue.
 */
void model::doPause()
{
	std::cout << std::endl << "Press any key to close." << std::endl;
	std::getchar();
}

/*
 *  Raise an error message and deal with it accordingly.
 */
void model::doError( std::string sError, unsigned char cError )
{
	model::log->writeError( sError, cError );
	if ( cError & model::errorCodes::kLevelModelStop )
		std::cout << "model forceAbort was requested by a function." << std::endl;
	if ( cError & model::errorCodes::kLevelFatal )
	{
		model::doPause();
		exit( model::appReturnCodes::kAppFatal );

		
	}
}

