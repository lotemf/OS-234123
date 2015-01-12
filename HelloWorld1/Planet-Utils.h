/*
 * Planet-Utils.h
 *
 *  Created on: Jan 8, 2015
 *      Author: Lotem
 */

#ifndef PLANET_UTILS_H_
#define PLANET_UTILS_H_

#include "library2.h"

#include <iostream>

#include "stdlib.h"
using std::exception;

/*******************************************************************************
 * Define Values
 ******************************************************************************/
#define NONE 0
#define NOT_INITIALIZED -1
#define NOT_YET_INITIALIZED -2


/******************************************************************************
	 	 The Exceptions for the Data Structure.
******************************************************************************/
class Failure: public std::exception {
public:
	const StatusType what() {
		return FAILURE;
	}
};

class AllocationError: public std::exception {
public:
	const StatusType what() {
		return ALLOCATION_ERROR;
	}
};

class InvalidInput: public std::exception {
public:
	const StatusType what() {
		return INVALID_INPUT;
	}
};

/*******************************************************************************
 * Citizen Class - Holds the ID number of the citizen and the city he lives in,
 * 				   default city value is -1, if he isn't in any city
 ******************************************************************************/
class Citizen{
	int Id;
	int city;

public:
	Citizen():Id(NOT_YET_INITIALIZED),city(NOT_YET_INITIALIZED){}  //TODO - what is it used for???
	Citizen (int id) : Id(id), city (NOT_YET_INITIALIZED) {}
	Citizen(const Citizen& citizen):Id(citizen.Id),city(citizen.city){}
	int getId() const{return Id;}
	int getCity() const{return city;}
	void setCity(int cityId){city=cityId;}
	int operator!=(Citizen otherCitizen){
		return (Id != otherCitizen.getId());
	}
	int operator%(int num) const{
		return (Id % num);
	}
	bool operator==(Citizen otherCitizen){
		return (Id == otherCitizen.getId());
	}
};
/*******************************************************************************
 * CitizenCompare - A compare class between two Citizen classes
 * 					Returns the difference between the two Id fields
 ******************************************************************************/
class CitizenCompare {
public:
	int operator()(Citizen citizen1, Citizen citizen2) {
		return (citizen1.getId() - citizen2.getId());
	}

};
/*******************************************************************************
 * CompareCitiesByCitizens - Compares two cities by going to the cities Array
 * 							 and checking which one has more citizens
 ******************************************************************************/
class CompareCitiesByCitizens {
	int* citiesArray;
	int size;
public:
	CompareCitiesByCitizens(int* newCitiesArray, int newSize) :
		citiesArray(newCitiesArray), size(newSize) {
	}
	int operator()(int i, int j) {
		if ((i<0) || (j<0) || (i>=size) || (j>=size)) {
			return 0;
		}
		int result = citiesArray[i]-citiesArray[j];
		if (result == 0) {
			result = i-j;
		}
		return result;
	}
};
/*******************************************************************************
 * ReverseCompareCitiesByCitizens - Compares two cities by going to the cities Array
 * 							 and checking which one has more citizens
 ******************************************************************************/
class ReverseCompareCitiesByCitizens {
	int* citiesArray;
	int size;
public:
	ReverseCompareCitiesByCitizens(int* newCitiesArray, int newSize) :
		citiesArray(newCitiesArray), size(newSize) {
	}
	int operator()(int i, int j) {
		if ((i<0) || (j<0) || (i>=size) || (j>=size)) {
			return 0;
		}
		int result = citiesArray[i]-citiesArray[j];
		if (result == 0) {
			result = j-i;
		}
		return result;
	}
};

#endif /* PLANET_UTILS_H_ */
