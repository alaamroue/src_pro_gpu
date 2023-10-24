/*
 * HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 * Copyright (C) 2023 Luke S. Smith, Qiuhua Liang.
 * This code is licensed under GPLv3. See LICENCE for more information.
 * This file has been modified by Alaa Mroue on 04.2023-12.2023.
 * See OriginalSourceCode.zip for changes. (Based on 1e62acf6b9b480e08646b232361b68c1827d91ae from https://github.com/lukeshope/hipims-ocl )
 */


// Includes
#include <cmath>
#include <math.h>
#include "common.h"
#include "CExecutorControlOpenCL.h"
#include "CDomainManager.h"
#include "CDomain.h"
#include "CScheme.h"

using std::min;
using std::max;

/*
 *  Constructor
 */
CModel::CModel(void)
{
	this->log = nullptr;

	this->execController	= NULL;
	this->domains			= new CDomainManager();
	this->mpiManager		= NULL;

	this->dCurrentTime		= 0.0;
	this->dSimulationTime	= 60;
	this->dOutputFrequency	= 60;
	this->bDoublePrecision	= true;

	this->pProgressCoords.sX = -1;
	this->pProgressCoords.sY = -1;

	this->ulRealTimeStart = 0;
}

/*
 *  Destructor
 */
CModel::~CModel(void)
{
	if (this->domains != NULL)
		delete this->domains;
	if ( this->execController != NULL )
		delete this->execController;
	this->log->logInfo("The model engine is completely unloaded.");
	if (this->log != NULL)
		delete this->log;
	if (this->profiler != NULL)
		delete this->profiler;
}

/*
 *  Set the type of executor to use for the model
 */
bool CModel::setExecutor(CExecutorControl* pExecutorControl)
{
	// TODO: Has the value actually changed?

	// TODO: Delete the old executor controller

	this->execController = static_cast<CExecutorControlOpenCL*>(pExecutorControl);

	if (!this->execController->isReady())
	{
		this->log->writeError(
			"The executor is not ready. Model cannot continue.",
			model::errorCodes::kLevelFatal
		);
		return false;
	}

	return true;
}

/*
 *  Returns a pointer to the execution controller currently in use
 */
CExecutorControlOpenCL* CModel::getExecutor(void)
{
	return this->execController;
}

/*
 *  Returns a pointer to the domain class
 */
CDomainManager* CModel::getDomainSet(void)
{
	return this->domains;
}

/*
*  Returns a pointer to the MPI manager class
*/
CMPIManager* CModel::getMPIManager(void)
{
	// Pass back the pointer - will be NULL if not using MPI version
	return this->mpiManager;
}

void CModel::setSelectedDevice(unsigned int id) {
	this->selectedDevice = id;
	this->getExecutor()->selectDevice(id);
}

unsigned int CModel::getSelectedDevice() {
	return this->selectedDevice;
}

/*
 *  Log the details for the whole simulation
 */
void CModel::logDetails()
{
	unsigned short wColour = model::cli::colourInfoBlock;

	this->log->writeDivide();
	this->log->logInfo("SIMULATION CONFIGURATION");
	this->log->logInfo("  Name:               " + this->sModelName);
	this->log->logInfo("  Simulation length:  " + Util::secondsToTime(this->dSimulationTime));
	this->log->logInfo("  Output frequency:   " + Util::secondsToTime(this->dOutputFrequency));
	this->log->logInfo("  Floating-point:     " + (std::string)(this->getFloatPrecision() == model::floatPrecision::kDouble ? "Double-precision" : "Single-precision"));
	this->log->writeDivide();
}

/*
 *  Execute the model
 */
bool CModel::runModel(void)
{
	this->log->logInfo("Verifying the required data before model run...");

	if (!this->domains || !this->domains->isSetReady())
	{
		model::doError(
			"The domain is not ready.",
			model::errorCodes::kLevelModelStop,
			"bool CModel::runModel(void)",
			"Please restart the program and try again."
		);
		return false;
	}
	if (!this->execController || !this->execController->isReady())
	{
		model::doError(
			"The executor is not ready.",
			model::errorCodes::kLevelModelStop,
			"bool CModel::runModel(void)",
			"Please restart the program and try again."
		);
		return false;
	}

	this->log->logInfo("Verification is complete.");

	this->log->writeDivide();
	this->log->logInfo("Starting a new simulation...");

	this->runModelPrepare();
	//this->runModelMain();

	return true;
}

/*
 *  Sets a short name for the model
 */
void	CModel::setName(std::string sName)
{
	this->sModelName = sName;
}

/*
 *  Sets a short name for the model
 */
void	CModel::setDescription(std::string sDescription)
{
	this->sModelDescription = sDescription;
}

/*
 *  Sets the total length of a simulation
 */
void	CModel::setSimulationLength(double dLength)
{
	this->dSimulationTime = dLength;
}

/*
 *  Gets the total length of a simulation
 */
double	CModel::getSimulationLength()
{
	return this->dSimulationTime;
}

/*
 *  Set the frequency of outputs
 */
void	CModel::setOutputFrequency(double dFrequency)
{
	this->dOutputFrequency = dFrequency;
}

/*
 *  Get the frequency of outputs
 */
double	CModel::getOutputFrequency()
{
	return this->dOutputFrequency;
}

/*
 *  Set floating point precision
 */
void	CModel::setFloatPrecision(unsigned char ucPrecision)
{
	if (!this->getExecutor()->getDevice()->isDoubleCompatible())
		ucPrecision = model::floatPrecision::kSingle;

	this->bDoublePrecision = (ucPrecision == model::floatPrecision::kDouble);
}

/*
 *  Get floating point precision
 */
unsigned char	CModel::getFloatPrecision()
{
	return (this->bDoublePrecision ? model::floatPrecision::kDouble : model::floatPrecision::kSingle);
}

/*
 *  Write details of where model execution is currently at
 */
void	CModel::logProgress(CBenchmark::sPerformanceMetrics* sTotalMetrics)
{
	char	cTimeLine[70] = "                                                                    X";
	char	cCellsLine[70] = "                                                                    X";
	char	cTimeLine2[70] = "                                                                    X";
	char	cCells[70] = "                                                                    X";
	char	cProgressLine[70] = "                                                                    X";
	char	cBatchSizeLine[70] = "                                                                    X";
	char	cProgress[57] = "                                                      ";
	char	cProgessNumber[7] = "      ";

	unsigned short wColour = model::cli::colourInfoBlock;

	double		  dCurrentTime = (this->dCurrentTime > this->dSimulationTime ? this->dSimulationTime : this->dCurrentTime);
	double		  dProgress = dCurrentTime / this->dSimulationTime;

	// TODO: These next bits will need modifying for when we have multiple domains
	unsigned long long	ulCurrentCellsCalculated = 0;
	unsigned int		uiBatchSizeMax = 0, uiBatchSizeMin = 9999;
	double				dSmallestTimestep = 9999.0;

	// Get the total number of cells calculated
	for (unsigned int i = 0; i < domains->getDomainCount(); ++i)
	{
		if (domains->isDomainLocal(i))
		{
			// Get the number of cells calculated (for the rate mainly)
			// TODO: Deal with this for MPI...
			ulCurrentCellsCalculated += domains->getDomain(i)->getScheme()->getCellsCalculated();
		}

		CDomainBase::mpiSignalDataProgress pProgress = domains->getDomain(i)->getDataProgress();

		if (uiBatchSizeMax < pProgress.uiBatchSize)
			uiBatchSizeMax = pProgress.uiBatchSize;
		if (uiBatchSizeMin > pProgress.uiBatchSize)
			uiBatchSizeMin = pProgress.uiBatchSize;
		if (dSmallestTimestep > pProgress.dBatchTimesteps)
			dSmallestTimestep = pProgress.dBatchTimesteps;
	}

	unsigned long ulRate = static_cast<unsigned long>(ulCurrentCellsCalculated / sTotalMetrics->dSeconds);

	// Make a progress bar
	for (unsigned char i = 0; i <= floor(55.0f * dProgress); i++)
		cProgress[i] = (i >= (floor(55.0f * dProgress) - 1) ? '>' : '=');

	// String padding stuff
	sprintf(cTimeLine, " Simulation time:  %-15sLowest timestep: %15s", Util::secondsToTime(dCurrentTime).c_str(), Util::secondsToTime(dSmallestTimestep).c_str());
	sprintf(cCells, "%I64u", ulCurrentCellsCalculated);
	sprintf(cCellsLine, " Cells calculated: %-24s  Rate: %13s/s", cCells, toStringExact(ulRate).c_str());
	sprintf(cTimeLine2, " Processing time:  %-16sEst. remaining: %15s", Util::secondsToTime(sTotalMetrics->dSeconds).c_str(), Util::secondsToTime(min((1.0 - dProgress) * (sTotalMetrics->dSeconds / dProgress), 31536000.0)).c_str());
	sprintf(cBatchSizeLine, " Batch size:       %-16s                                 ", toStringExact(uiBatchSizeMin).c_str());
	sprintf(cProgessNumber, "%.1f%%", dProgress * 100);
	sprintf(cProgressLine, " [%-55s] %7s", cProgress, cProgessNumber);


	model::log->writeDivide();																						// 1
	model::log->logInfo("                                                                  ");	// 2
	model::log->logInfo(" SIMULATION PROGRESS                                              ");	// 3
	model::log->logInfo("                                                                  ");	// 4
	model::log->logInfo(std::string(cTimeLine));	// 5
	model::log->logInfo(std::string(cCellsLine));	// 6
	model::log->logInfo(std::string(cTimeLine2));	// 7
	model::log->logInfo(std::string(cBatchSizeLine));	// 8
	model::log->logInfo("                                                                  ");	// 9
	model::log->logInfo(std::string(cProgressLine));	// 10
	model::log->logInfo("                                                                  ");	// 11

	model::log->logInfo("             +----------+----------------+------------+----------+");	// 12
	model::log->logInfo("             |  Device  |  Avg.timestep  | Iterations | Bypassed |");	// 12
	model::log->logInfo("+------------+----------+----------------+------------+----------|");	// 13

	for (unsigned int i = 0; i < domains->getDomainCount(); i++)
	{
		char cDomainLine[70] = "                                                                    X";
		CDomainBase::mpiSignalDataProgress pProgress = domains->getDomain(i)->getDataProgress();

		// TODO: Give this it's proper name...
		std::string sDeviceName = "REMOTE";

		if (domains->isDomainLocal(i))
		{
			sDeviceName = domains->getDomain(i)->getDevice()->getDeviceShortName();
		}

		sprintf(
			cDomainLine,
			"| Domain #%-2s | %8s | %14s | %10s | %8s |",
			toStringExact(i + 1).c_str(),
			sDeviceName.c_str(),
			Util::secondsToTime(pProgress.dBatchTimesteps).c_str(),
			toStringExact(pProgress.uiBatchSuccessful).c_str(),
			toStringExact(pProgress.uiBatchSkipped).c_str()
		);

		model::log->logInfo(std::string(cDomainLine));	// ++
	}

	model::log->logInfo("+------------+----------+----------------+------------+----------+");	// 14
	model::log->writeDivide();																						// 15

	//this->pProgressCoords = Util::getCursorPosition();
	//if (this->dCurrentTime < this->dSimulationTime) 
	//{
	//	this->pProgressCoords.sY = max(0, this->pProgressCoords.sY - (16 + (cl_int)domains->getDomainCount()));
	//	Util::setCursorPosition(this->pProgressCoords);
	//}
}

/*
 *  Update the visualisation by sending domain data over to the relevant component
 */
void CModel::visualiserUpdate()
{
	if (this->dCurrentTime >= this->dSimulationTime - 1E-5)
		return;

}

/*
 *  Memory read should have completed, so provided the simulation isn't over - read it back again
 */
void CL_CALLBACK CModel::visualiserCallback(cl_event clEvent, cl_int iStatus, void* vData)
{
	model::CallBackData* callBackData = (model::CallBackData*)vData;
	//TODO: The visualizer won't work becuase this is commented out
	callBackData->cModel->visualiserUpdate();
	clReleaseEvent(clEvent);
}

/*
*  Prepare for a new simulation, which may follow a failed simulation so states need to be reset.
*/
void	CModel::runModelPrepare()
{
	// Can't have timestep sync if we've only got one domain
	if (this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncTimestep &&
		this->getDomainSet()->getDomainCount() <= 1)
		this->getDomainSet()->setSyncMethod(model::syncMethod::kSyncForecast);

	this->runModelPrepareDomains();

	bSynchronised = true;
	bAllIdle = true;
	dTargetTime = 0.0;
	dLastSyncTime = -1.0;
	dLastOutputTime = 0.0;
}

/*
*  Prepare domains for a new simulation.
*/
void	CModel::runModelPrepareDomains()
{
	for (unsigned int i = 0; i < domains->getDomainCount(); ++i)
	{
		if (!domains->isDomainLocal(i))
			continue;

		domains->getDomain(i)->getScheme()->prepareSimulation();
		domains->getDomain(i)->setRollbackLimit();		// Auto calculate from links...

		if (domains->getDomainCount() > 1)
		{
			model::log->logInfo("Domain #" + toStringExact(i + 1) + " has rollback limit of " +
				toStringExact(domains->getDomain(i)->getRollbackLimit()) + " iterations.");
		}
		else {
			model::log->logInfo("Domain #" + toStringExact(i + 1) + " is not constrained by " +
				"overlapping.");
		}
	}
}

/*
*  Assess the current state of each domain.
*/
void	CModel::runModelDomainAssess(bool* bIdle)
{
	bRollbackRequired = false;
	dEarliestTime = 0.0;
	bWaitOnLinks = false;

	// Minimum time
	dCurrentTime = domains->getDomain(0)->getScheme()->getCurrentTime();

	// Either we're not ready to sync, or we were still synced from the last run
	if (domains->getDomain(0)->getScheme()->isRunning() || domains->getDomain(0)->getDevice()->isBusy()) {
		*bIdle = false;
	}
	else {
		*bIdle = true;
	}
}

/*
 *  Exchange data across domains where necessary.
 */
void	CModel::runModelDomainExchange()
{
	#ifdef DEBUG_MPI
	model::log->logInfo("[DEBUG] Exchanging domain data NOW... (" + Util::secondsToTime(this->dEarliestTime) + ")");
	#endif

	// Swap sync zones over
	for (unsigned int i = 0; i < domains->getDomainCount(); ++i)		// Source domain
	{
		if (domains->isDomainLocal(i))
		{
			domains->getDomain(i)->getScheme()->importLinkZoneData();
			// TODO: Above command does not actually cause import -- next line can be removed?
			domains->getDomain(i)->getDevice()->flushAndSetMarker();
		}
	}

	this->runModelBlockNode();
}

/*
*  Synchronise the whole model across all domains.
*/
void	CModel::runModelUpdateTarget(double dTimeBase)
{
	// Identify the smallest batch size associated timestep
	double dEarliestSyncProposal = this->dSimulationTime;

	// Only bother with all this stuff if we actually need to synchronise,
	// otherwise run free, for as long as possible (i.e. until outputs needed)
	if (domains->getDomainCount() > 1 &&
		this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncForecast)
	{
		for (unsigned int i = 0; i < domains->getDomainCount(); ++i)
		{
			// TODO: How to calculate this for remote domains etc?
			if (domains->isDomainLocal(i))
				dEarliestSyncProposal = min(dEarliestSyncProposal, domains->getDomain(i)->getScheme()->proposeSyncPoint(dCurrentTime));
		}
	}

	// Don't exceed an output interval if required
	if (floor(dEarliestSyncProposal / dOutputFrequency) > floor(dLastSyncTime / dOutputFrequency))
	{
		dEarliestSyncProposal = (floor(dLastSyncTime / dOutputFrequency) + 1) * dOutputFrequency;
	}

	// Work scheduler within numerical schemes should identify whether this has changed
	// and update the buffer if required only...
	// Alaa: We  don't need this anymore. Promaides should do the proposal
	//dTargetTime = dEarliestSyncProposal;

}

/*
*  Synchronise the whole model across all domains.
*/
void	CModel::runModelSync()
{
	if (bRollbackRequired ||
		!bSynchronised)
		return;

	// No rollback required, thus we know the simulation time can now be increased
	// to match the target we'd defined
	this->dCurrentTime = dEarliestTime;
	dLastSyncTime = this->dCurrentTime;

	// TODO: Review if this is needed? Shouldn't earliest time get updated anyway?
	if (domains->getDomainCount() <= 1)
		this->dCurrentTime = dEarliestTime;

	// Calculate a new target time to aim for
	//this->runModelUpdateTarget( dCurrentTime );

	// ---
	// TODO: In MPI implementation, we need to invoke a reduce operation here to identify the lowest
	// sync proposal across all domains in the Comm...
	// ---

	// ---
	// TODO: All of this block of code needs to be removed ultimately. Domain links should constantly
	// be prepared to download data when a simulation reaches its target time. Domain exchange should
	// therefore need only to take data already held locally and impose it on the main domain buffer.

	// Let each domain know the goalposts have proverbially moved
	// Read back domain data incase we need to rollback
	for (unsigned int i = 0; i < domains->getDomainCount(); ++i)
	{
		if (domains->isDomainLocal(i))
		{
			// Update with the new target time
			// Can no longer do this here - may have to wait for MPI to return with the value
			//domains->getDomain(i)->getScheme()->setTargetTime(dTargetTime);

			// Save the current state back to host memory, but only if necessary
			// for either domain sync/rollbacks or to write outputs
			if (
				(domains->getDomainCount() > 1 && this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncForecast) ||
				(fabs(this->dCurrentTime - dLastOutputTime - this->getOutputFrequency()) < 1E-5 && this->dCurrentTime > dLastOutputTime)
				)
			{
				#ifdef DEBUG_MPI
				model::log->logInfo("[DEBUG] Saving domain state for domain #" + toStringExact(i));
				#endif
				domains->getDomain(i)->getScheme()->saveCurrentState();
			}
		}
	}

	// Let devices finish
	this->runModelBlockNode();

	// Exchange domain data
	this->runModelDomainExchange();

	// Wait for all nodes and devices
	// TODO: This should become global across all nodes 
	this->runModelBlockNode();
	//this->runModelBlockGlobal();
}

/*
*  Block execution across all domains which reside on this node only
*/
void	CModel::runModelBlockNode()
{
	for (unsigned int i = 0; i < domains->getDomainCount(); i++)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getDevice()->blockUntilFinished();
	}
}

/*
 *  Block execution across all domains until every single one is ready
 */
void	CModel::runModelBlockGlobal()
{
	this->runModelBlockNode();
}

/*
*  Schedule new work in the simulation.
*/
void	CModel::runModelSchedule(CBenchmark::sPerformanceMetrics* sTotalMetrics, bool* bIdle)
{

	// if ALL our domains are idle
	if (bIdle[0]) {
		domains->getDomain(0)->getScheme()->runSimulation(dTargetTime, sTotalMetrics->dSeconds);
	}


	//domains->getDomain(i)->getScheme()->forceTimestep(dGlobalTimestep);

			// Run a batch


	// Wait if we're syncing timesteps?
	//if ( this->getDomainSet()->getSyncMethod() == model::syncMethod::kSyncTimestep )
	//	this->runModelBlockGlobal();
}

/*
*  Update UI elements (progress bars etc.)
*/
void	CModel::runModelUI(CBenchmark::sPerformanceMetrics* sTotalMetrics)
{
	dProcessingTime = sTotalMetrics->dSeconds;
	if (sTotalMetrics->dSeconds - dLastProgressUpdate > 0.85)
	{
		this->logProgress(sTotalMetrics);
		dLastProgressUpdate = sTotalMetrics->dSeconds;
	}
}

/*
*  Rollback simulation states to a previous recorded state.
*/
void	CModel::runModelRollback()
{
	if (!bRollbackRequired ||
		!bAllIdle)
		return;

	model::doError(
		"Rollback invoked - code not yet ready",
		model::errorCodes::kLevelModelStop,
		"void	CModel::runModelRollback()",
		"EXPERIMENTAL"
	);

	// Now sync'd again and ready to continue
	bRollbackRequired = false;
	bSynchronised = false;

	// Use the data from the last run to work out how long we can run 
	// the batch for. Same function as normal but relative to the last sync time instead.
	this->runModelUpdateTarget(dLastSyncTime);
	model::log->logInfo("Simulation rollback at " + Util::secondsToTime(this->dCurrentTime) + "; revised sync point is " + Util::secondsToTime(dTargetTime) + ".");

	// ---
	// TODO: Do we need to do an MPI reduce here...?
	// ---

	// ---
	// TODO: Notify all nodes of the rollback requirements...?
	// ---

	// Let each domain know the goalposts have proverbially moved
	// Write the last full set of domain data back to the GPU
	dEarliestTime = dLastSyncTime;
	dCurrentTime = dLastSyncTime;
	for (unsigned int i = 0; i < domains->getDomainCount(); i++)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getScheme()->rollbackSimulation(dLastSyncTime, dTargetTime);
	}

	// Global block across all nodes is required for rollbacks
	runModelBlockGlobal();
}


/*
 *  Clean things up after the model is complete or aborted
 */
void	CModel::runModelCleanup()
{
	// Note these will not return until their threads have terminated
	for (unsigned int i = 0; i < domains->getDomainCount(); ++i)
	{
		if (domains->isDomainLocal(i))
			domains->getDomain(i)->getScheme()->cleanupSimulation();
	}
}

/*
 *  Run the actual simulation, asking each domain and schemes therein in turn etc.

void	CModel::runModelMain()
{
	bool*							bSyncReady				= new bool[ domains->getDomainCount() ];
	bool*							bIdle					= new bool[domains->getDomainCount()];
	double							dCellRate				= 0.0;
	CBenchmark::sPerformanceMetrics *sTotalMetrics;
	CBenchmark						*pBenchmarkAll;

	// Write out the simulation details
	this->logDetails();

	// Track time for the whole simulation
	model::log->logInfo( "Collecting time and performance data..." );
	pBenchmarkAll = new CBenchmark( true );
	sTotalMetrics = pBenchmarkAll->getMetrics();

	// Track total processing time
	dProcessingTime = sTotalMetrics->dSeconds;
	dVisualisationTime = dProcessingTime;

	// ---------
	// Run the main management loop
	// ---------
	// Even if user has forced abort, still wait until all idle state is reached
	while ( ( this->dCurrentTime < dSimulationTime - 1E-5 ) || !bAllIdle )
	{
		// Assess the overall state of the simulation at present
		this->runModelDomainAssess(
			bSyncReady,
			bIdle
		);


		// Perform a rollback if required
		this->runModelRollback();

		// Perform a sync if possible
		this->runModelSync();

		// Don't proceed beyond this point if we need to rollback and we're just waiting for
		// devices to finish first...
		if (bRollbackRequired)
			continue;

		// Schedule new work
		this->runModelSchedule(
			sTotalMetrics,
			bIdle
		);

		// Update progress bar after each batch, not every time
		sTotalMetrics = pBenchmarkAll->getMetrics();
		this->runModelUI(
			sTotalMetrics
		);
	}

	// Update to 100% progress bar
	pBenchmarkAll->finish();
	sTotalMetrics = pBenchmarkAll->getMetrics();
	this->runModelUI(
		sTotalMetrics
	);

	// Get the total number of cells calculated
	unsigned long long	ulCurrentCellsCalculated = 0;
	double				dVolume = 0.0;
	for( unsigned int i = 0; i < domains->getDomainCount(); ++i )
	{
		if (!domains->isDomainLocal(i))
			continue;

		ulCurrentCellsCalculated += domains->getDomain(i)->getScheme()->getCellsCalculated();
		dVolume += abs( domains->getDomain(i)->getVolume() );
	}
	unsigned long ulRate = static_cast<unsigned long>(static_cast<double>(ulCurrentCellsCalculated) / sTotalMetrics->dSeconds);

	model::log->logInfo( "Simulation time:     " + Util::secondsToTime( sTotalMetrics->dSeconds ) );
	//model::log->logInfo( "Calculation rate:    " + toStringExact( floor(dCellRate) ) + " cells/sec" );
	//model::log->logInfo( "Final volume:        " + toStringExact( static_cast<int>( dVolume ) ) + "m3" );
	model::log->writeDivide();

	delete   pBenchmarkAll;
	delete[] bSyncReady;
	delete[] bIdle;
}
 */
 /*
  *  Run the actual simulation, asking each domain and schemes therein in turn etc.
  */
void	CModel::runNext(const double next_time_point)
{
	bool bIdle;
	double							dCellRate = 0.0;
	CBenchmark::sPerformanceMetrics* sTotalMetrics;
	CBenchmark* pBenchmarkAll;

	// Write out the simulation details
	//this->logDetails();

	// Track time for the whole simulation
	//model::log->logInfo("Collecting time and performance data...");
	pBenchmarkAll = new CBenchmark(true);
	sTotalMetrics = pBenchmarkAll->getMetrics();

	// Track total processing time
	dProcessingTime = sTotalMetrics->dSeconds;
	dVisualisationTime = dProcessingTime;

	//dSimulationTime = next_time_point;
	dTargetTime = next_time_point;
	// ---------
	// Run the main management loop
	// ---------
	// Even if user has forced abort, still wait until all idle state is reached
	while (this->dCurrentTime < dTargetTime)
	{
		// Assess the overall state of the simulation at present
		this->runModelDomainAssess(&bIdle);


		// Don't proceed beyond this point if we need to rollback
		if (bRollbackRequired) {
			std::cout << "Rollback Required...Simulation failed! Try a different sync step.";
			continue;
		}

		// Schedule new work
		if (bIdle) {
			domains->getDomain(0)->getScheme()->runSimulation(dTargetTime, sTotalMetrics->dSeconds);
		}

		// Update progress bar after each batch, not every time
		sTotalMetrics = pBenchmarkAll->getMetrics();
		if (showProgess) {
			this->runModelUI(sTotalMetrics);
		}

		if (this->dCurrentTime > next_time_point) {
			std::cout << "We are off, next point should " << next_time_point << " but we are at " << this->dCurrentTime << std::endl;
			break;
		}

	}

	/*
	// Update to 100% progress bar
	pBenchmarkAll->finish();
	sTotalMetrics = pBenchmarkAll->getMetrics();
	this->runModelUI(
		sTotalMetrics
	);
	*/

	// Get the total number of cells calculated
	unsigned long long	ulCurrentCellsCalculated = 0;
	double				dVolume = 0.0;
	for (unsigned int i = 0; i < domains->getDomainCount(); ++i)
	{
		if (!domains->isDomainLocal(i))
			continue;

		ulCurrentCellsCalculated += domains->getDomain(i)->getScheme()->getCellsCalculated();
		dVolume += abs(domains->getDomain(i)->getVolume());
	}
	unsigned long ulRate = static_cast<unsigned long>(static_cast<double>(ulCurrentCellsCalculated) / sTotalMetrics->dSeconds);

	//model::log->logInfo("Simulation time:     " + Util::secondsToTime(sTotalMetrics->dSeconds));
	//model::log->logInfo( "Calculation rate:    " + toStringExact( floor(dCellRate) ) + " cells/sec" );
	//model::log->logInfo( "Final volume:        " + toStringExact( static_cast<int>( dVolume ) ) + "m3" );
	//model::log->writeDivide();

	delete   pBenchmarkAll;
}

/*
 * Attached the logger class to the CModel
 */
void CModel::setLogger(CLog* cLog) {
	this->log = cLog;
}

/*
 * Attached the proifler class to the CModel
 */
void CModel::setProfiler(CProfiler* profiler) {
	this->profiler = profiler;
}


/*
 * Attached the logger class to the CModel
 */
void CModel::setUIStatus(bool status) {
	this->showProgess = status;
}
