/*
 * HashTable.h
 *
 *  Created on: Jan 6, 2015
 *      Author: Lotem
 */
#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include "Planet-Utils.h"

class AlreadyExists : public std::exception {};

const int TABLE_SIZE = 11;
class HashTable {
private:
	int size;
	Citizen* table;
	int modulo;
	int hash(const Citizen& x) const;
	int resedue(const Citizen& x) const;
	void reHash();

public:
	HashTable();
	void insert(const Citizen& data);
	Citizen& getMember(const Citizen& data);
	bool member(const Citizen& data);
	~HashTable();
};


#endif /* HASHTABLE_H_ */
