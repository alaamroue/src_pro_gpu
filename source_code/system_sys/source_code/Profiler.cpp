#include "Profiler.h"

Profiler::Profiler() {
	activated = false;
}

Profiler::Profiler(bool state) {
	activated = state;
}

Profiler::~Profiler() {
	for (ProfiledElement* profiledElement : this->profiledElements) {
		delete profiledElement;
	}
	this->profiledElements.clear();
}

void Profiler::profile(std::string name, int flag) {
	if (!activated)
		return;

	//	Check if already exist
	if (doesntExists(name)) {
		createProfileElement(name);
		if (flag == profilerFlags::END_PROFILING) {
			std::cout << "Profiler was set to End profiling but it was never started" << std::endl;
		}
	}

	ProfiledElement* profiledElement = getProfileElement(name);
	if (profiledElement != nullptr) {

		if (flag == profilerFlags::START_PROFILING) {
			if (profiledElement->isStarted == true) {
				std::cout << "Profile Element " + name + " never ended. And is now requested to start" << std::endl;
			}
			profiledElement->isStarted = true;

			#ifndef __GNUC__
			QueryPerformanceFrequency(&profiledElement->frequency);
			QueryPerformanceCounter(&profiledElement->start);
			#else
			clock_gettime(CLOCK_REALTIME, &profiledElement->start);
			#endif

		}
		if (flag == profilerFlags::END_PROFILING) {
			if (profiledElement->isStarted == false) {
				std::cout << "Profile Element " + name + " never started. And is now requested to end" << std::endl;
			}
			profiledElement->isStarted = false;


			#ifndef __GNUC__
			QueryPerformanceCounter(&profiledElement->end);
			profiledElement->totalTicks += profiledElement->end.QuadPart - profiledElement->start.QuadPart;
			#else
			clock_gettime(CLOCK_REALTIME, &profiledElement->end);
			#endif
		}

	}
	else {
		std::cout << "Profiler couldn't find the element it was looking for" << std::endl;
	}
}

bool Profiler::doesntExists(std::string name) {
	for (ProfiledElement* profiledElement : this->profiledElements) {
		if (profiledElement->name == name) {
			return false;
		}
	}
	return true;
}

void Profiler::createProfileElement(std::string name) {
	ProfiledElement* profiledElement = new ProfiledElement();
	profiledElement->name = name;
	this->profiledElements.push_back(profiledElement);
}

Profiler::ProfiledElement* Profiler::getProfileElement(std::string name) {
	for (ProfiledElement* profiledElement : this->profiledElements) {
		if (profiledElement->name == name) {
			return profiledElement;
		}
	}
	std::cout << "Profiler never Found the element it was looking for" << std::endl;
	return nullptr;
}

void Profiler::logValues() {
	if (!activated) {
		return;
	}

	std::cout << "### PROFILE Results ###" << std::endl;

	int numberOfElements = this->profiledElements.size();

	std::string* names = new std::string[numberOfElements];
	double* times = new double[numberOfElements];

	for (int i = 0; i < numberOfElements; ++i) {
		names[i] = this->profiledElements[i]->name;
		#ifndef __GNUC__
		times[i] = static_cast<double>(this->profiledElements[i]->totalTicks) / this->profiledElements[i]->frequency.QuadPart;
		#else
		double dStart = static_cast<double>(this->profiledElements[i]->start.tv_sec) + static_cast<double>(this->profiledElements[i]->start.tv_nsec / 1000000000);
		double dEnd = static_cast<double>(this->profiledElements[i]->end.tv_sec) + static_cast<double>(this->profiledElements[i]->end.tv_nsec / 1000000000);
		times[i] = dEnd - dStart;
		#endif
	}

	std::cout << "----" << std::endl;
	for (int i = 0; i < numberOfElements; ++i) {
		std::cout << names[i] << " : " << times[i] << " s" << std::endl;
	}
	std::cout << "----" << std::endl;


	delete[] names;
	delete[] times;

}