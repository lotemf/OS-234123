/*
 * Planet.h
 *
 *  Created on: Jan 10, 2015
 *      Author: Lotem
 */
#ifndef PLANET_H_
#define PLANET_H_

#include "UnionFind.h"
#include "AVLTree.h"
#include "HashTable.h"


class Planet {
	/*Test*/
	int numOfCities;
	HashTable Citizens;
	int* citiesArray;
	UnionFind<ReverseCompareCitiesByCitizens> Kingdoms;
	AvlTree<int, CompareCitiesByCitizens> Cities;

	bool checkLegalCity(int i);
	bool checkIsCapital(int city, int kingdom);

public:
	Planet(int n);
	void addCitizen(int citizenId);
	void moveToCity(int citizenId,int city);
	void joinKingdoms(int city1,int city2);
	void getCapital(int citizenId, int* capitalCity);
	void selectCity(int k,int* city);
	void getCitiesBySize(int* cities);
	~Planet();
};

#endif

