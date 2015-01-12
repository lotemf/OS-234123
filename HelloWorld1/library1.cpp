/*
 * library1.cpp
 *
 *  Created on: Jan 11, 2015
 *      Author: Lotem
 */

#include "library2.h"
#include "Planet.h"

#define TRY_CATCH_SUCCESS(command) \
	try { \
		((Planet*)DS)->command; \
	} catch (InvalidInput& e) { \
		return e.what(); \
	} \
	catch (AllocationError& e) { \
		return e.what(); \
	}\
	catch (Failure& e){ \
		return e.what(); \
	}\
	catch (const std::bad_alloc& e){ \
	return ALLOCATION_ERROR; \
	}\
	return SUCCESS; \

#define CHECK_DS_NOT_NULL \
	if(!DS){ \
		return INVALID_INPUT; \
	}

void* Init(int n) {
	if (n<2){
		return NULL;
	}
	Planet* DS = new Planet(n);
	return (void*)DS;
}

StatusType AddCitizen(void* DS, int citizenId){
	CHECK_DS_NOT_NULL;
	TRY_CATCH_SUCCESS(addCitizen(citizenId));
}

StatusType MoveToCity(void* DS, int citizenId,int city){
	CHECK_DS_NOT_NULL;
	TRY_CATCH_SUCCESS(moveToCity(citizenId,city));
}

StatusType JoinKingdoms(void* DS,int city1,int city2){
	CHECK_DS_NOT_NULL;
	TRY_CATCH_SUCCESS(joinKingdoms(city1,city2));
}

StatusType GetCapital(void* DS,int citizenId,int* capitalCity){
	CHECK_DS_NOT_NULL;
	TRY_CATCH_SUCCESS(getCapital(citizenId,capitalCity));
}

StatusType SelectCity(void* DS,int k,int* city){
	CHECK_DS_NOT_NULL;
	TRY_CATCH_SUCCESS(selectCity(k,city));
}

StatusType GetCitiesBySize(void* DS,int* cities){
	CHECK_DS_NOT_NULL;
	TRY_CATCH_SUCCESS(getCitiesBySize(cities));
}

void Quit(void** DS){
	if (!DS){
		return;
	}
	delete ((Planet*)*DS);
	*DS = NULL;
}
