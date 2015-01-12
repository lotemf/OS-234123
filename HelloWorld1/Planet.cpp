/*
 * Planet.cpp
 *
 *  Created on: Jan 7, 2015
 *      Author: Lotem
 */

#include "Planet.h"

/*******************************************************************************
 *  Planet's Constructor
 ******************************************************************************/
Planet::Planet(int n) :
	numOfCities(n),
	citiesArray(new int[n]),
	Kingdoms(n, ReverseCompareCitiesByCitizens(citiesArray, n)),
	Cities(CompareCitiesByCitizens(citiesArray, n) ) {

	int* numbersArray=new int[n];
	for (int i=0; i<n; i++) {
		citiesArray[i]=0;
		numbersArray[i]=i;
	}
	Cities.newTreeFromArray(numbersArray, n);
	delete[] numbersArray;
}
/*******************************************************************************
 *  checkLegalCity - Auxilary function to check input values
 ******************************************************************************/
bool Planet::checkLegalCity(int i) {
	if ((i<0) || (i>=numOfCities)) {
		return false;
	}
	return true;
}
/*******************************************************************************
 *  checkIsCapital - Auxilary function that checks if the city provided
 *  				 is the capital of it's kingdom
 ******************************************************************************/
bool Planet::checkIsCapital(int city, int kingdom) {
	if (city == Kingdoms.getMax(kingdom)){
		return true;
	}
	return false;
}
/*******************************************************************************
  addCitizen  - Receives A citizen's ID that we want to add to the planet.
	  	  	    Checks if the citizen ID already exists, if not, adds it to the
	  	  	  	Hash Table of citizens.


			*Throws Failure() Exception if what we are trying to add is
			 already in the system.
			*Throws InvalidInput() Exception if the input is illegal
******************************************************************************/
void Planet::addCitizen(int citizenId){
	if (citizenId<0) {
		throw InvalidInput();
	}
	Citizen tempCitizen(citizenId);
	if (Citizens.member(tempCitizen)) {
		throw Failure();
	}
	try{
	Citizens.insert(tempCitizen);
	}
	catch (const std::exception& e){
		throw Failure();
	}
}
/*******************************************************************************
  moveToCity  - Receives A citizen's ID that we want to add to the city.
  	  	  		Checks if the citizen ID already exists in the planet, if it
  	  	  		does, we then check if it's already in any city, and
  	  	  		only if it's not in any other city already we add it to the
  	  	  		city that we got in the input.


		*Throws a Failure() Exception if what we are trying to add is
		 already in the system, or if there was a problem with the other
		 data structures functions.
 ******************************************************************************/
void Planet::moveToCity(int citizenId,int city){
	if ((citizenId<0) || (city<0) || (city>numOfCities)) {
		throw InvalidInput();
	}
	Citizen JustACitizen(citizenId);
	if (!Citizens.member(JustACitizen)){
		throw Failure();
	};

	if (Citizens.getMember(citizenId).getCity() != NOT_YET_INITIALIZED){
	    if (Citizens.getMember(citizenId).getCity() != city){
		throw Failure();
	    }
	    return;
	}
	this->Citizens.getMember(citizenId).setCity(city);

	if (Cities.remove(city) != AVL_SUCCESS) {
		throw Failure();
	}
	citiesArray[city]++;
	if (Cities.insert(city) != AVL_SUCCESS) {
		throw Failure();
	}
	Kingdoms.refreshGroup(city);
}
/*******************************************************************************
  joinKingdoms  - Receives two cities, and verifies that they are in the planet,
  	  	  	  	  then - makes sure they are different and checks that each one
  	  	  	  	  is the capital city of it's kingdom.  (A pre-condition)
  	  	  	  	  Only then makes the merge using the union function of
  	  	  	  	  the Union/Find Kingdoms data structure.


		*Throws a Failure() Exception if what we are trying to add is
		 already in the system, or if there was a problem with the other
		 data structures functions.
 ******************************************************************************/
void Planet::joinKingdoms(int city1, int city2) {
	if (!checkLegalCity(city1) || !checkLegalCity(city2)) {
		throw InvalidInput();
	}
	int kingdom1=Kingdoms.find(city1);
	int kingdom2=Kingdoms.find(city2);

	if (kingdom1==kingdom2) {
		throw Failure();
	}
	if (!checkIsCapital(city1, kingdom1) ||
			!checkIsCapital(city2, kingdom2)) {
		throw Failure();
	}
	Kingdoms.Union(kingdom1,kingdom2);
}
/*******************************************************************************
  getCapital  -   Receives a citizen's ID number, and checks if it's in the
  	  	  	      system, and in which city he lives.
  	  	  	      Only then, checks in the Kingdoms union/find for that city's
  	  	  	      capital city in the same kingdom, and returns it.

		*Throws a Failure() Exception if what we are trying to add is
		 already in the system, or if there was a problem with the other
		 data structures functions.
 ******************************************************************************/
void Planet::getCapital(int citizenId, int* capitalCity){
	if ((citizenId<0) || (!capitalCity)) {
		throw InvalidInput();
	}
	Citizen JustACitizen(citizenId);
	if (!Citizens.member(JustACitizen)){
		throw Failure();
	};
	int cityToCheck = Citizens.getMember(JustACitizen).getCity();

	int kingdom=Kingdoms.find(cityToCheck);
	if (kingdom==-1) {
		throw Failure();
	}
	int result=Kingdoms.getMax(kingdom);
	if (result==-1) {
		throw Failure();
	}
	*capitalCity=result;
}
/*******************************************************************************
  SelectCity  - Receives an index of the k-sized city in the planet,
  	  	  	  	and a pointer to store it in.

		*Throws a Failure() Exception if what we are trying to add is
		 already in the system, or if there was a problem with the other
		 data structures functions.
 ******************************************************************************/
void Planet::selectCity(int k,int* city){
	if ((k<0) ||  (!city)) {
		throw InvalidInput();
	}
	if (k>numOfCities-1){
		throw Failure();
	}
	*city = Cities.select(k,citiesArray,numOfCities);
}
/*******************************************************************************
  getCitiesBySize  - Recieves an array and inserts the cities to it sorted
  	  	  	  	  	 by citizen amount.

		*Throws a Failure() Exception if what we are trying to add is
		 already in the system, or if there was a problem with the other
		 data structures functions.
 ******************************************************************************/
void Planet::getCitiesBySize(int* cities){
	if (!cities) {
		throw InvalidInput();		//TODO - check this in the main file
	}
	Cities.inOrderToArray(cities,numOfCities);
}


Planet::~Planet(){
	delete[] citiesArray;
}


