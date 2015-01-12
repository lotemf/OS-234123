/*
 * HashTable.cpp
 *
 *  Created on: Jan 11, 2015
 *      Author: Lotem
 */
#include "HashTable.h"

HashTable::HashTable():size(0), table(NULL), modulo(TABLE_SIZE) {
	Citizen* arr = new Citizen[modulo];
	for(int i=0; i < modulo; i++) {
		arr[i] = Citizen();
	}
	table = arr;
}
/*******************************************************************************
 * hash - a scrambling function that uses a changing modulo operator,
 * 		  to calculate an insertion point to the hashTable's array.
 ******************************************************************************/
int HashTable::hash(const Citizen& x) const {
	return((x.getId())%modulo);
}
/*******************************************************************************
 * reHash - once the amount of members reaches the size of the table
 	 	 	creates a new table, twice the size and copies all of the old
 	 	 	table's value into it.
 ******************************************************************************/
void HashTable::reHash() {
	if(size < modulo) {
		return;
	}
	modulo*=2;
	Citizen* formerTable = table;
	Citizen* newTable = new Citizen[modulo];
	for(int i=0; i < modulo; i++) {
		newTable[i] = Citizen();
	}
	table = newTable;
	size = 0;
	for(int i=0; i < modulo/2; i++) {
		if(formerTable[i] != Citizen()) {
			insert(formerTable[i]);
		}
	}
	delete[] formerTable;
}
/*******************************************************************************
 * residue - calculates a "step" residue for the hash table
 * 			 receives as input the Citizen class and uses an overloaded %
 * 			 operator to make the calculation
 ******************************************************************************/
int HashTable::resedue(const Citizen& x) const {
		int n = x%(modulo-3);
		n = (n%2 == 0)? n + 1 :n;
		n = (n%TABLE_SIZE == 0)? n + 2: n;
		return n;
}
/*******************************************************************************
 * insert - calculates an insertion index using the hash method,
 	 	 	and inserts the new value in the next open spot in the array
 	 	 	according to the hash and residue calculations.
 ******************************************************************************/
void HashTable::insert(const Citizen& data) {
	if(member(data)) {
		throw AlreadyExists();
	}
	reHash();
	int i = hash(data);
	while (table[i] != Citizen()) {
		i =  hash(i + resedue(data));
	}
	table[i] = data;
	size++;
}
/*******************************************************************************
 * getMember - returns the Data of the searched input from the hashTable
 ******************************************************************************/
Citizen& HashTable::getMember(const Citizen& data) {
	int i = hash(data);
	int first = i;
	while ((table[i] != Citizen()) && (table[i]!=data)) {
		i =  hash(i + resedue(data));
		if(i == first) {
			break;
		}
	}
	return table[i];
}
/*******************************************************************************
 * member - checks if the data is already inside hashTable using getMember method
 ******************************************************************************/
bool HashTable::member(const Citizen& data) {
	return (getMember(data) == data);
}

HashTable::~HashTable(){
	if(table != NULL) {
		delete[] table;
	}
}

