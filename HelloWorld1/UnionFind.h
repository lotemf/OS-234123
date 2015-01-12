/*
 * UnionFind.h
 *
 *  Created on: Jan 7, 2015
 *      Author: Lotem
 */

#ifndef UNIONFIND_H_
#define UNIONFIND_H_

#include <iostream>

template<class Compare>
class UnionFind {
	class Group;
	class Member {
		int id;
		Member* base;
		Group* group;
	public:
		Member() :
			id(-1), base(NULL), group(NULL) {
		}
		void setId(int newId) {
			id=newId;
		}
		void setGroup(Group* newGroup) {
			group=newGroup;
		}
		void setParent(Member* newBase) {
			base=newBase;
		}

		Member* getParent() {
			return base;
		}

		Group* getGroup() {
			return group;
		}
	};
	class Group {
		int id;
		int size;
		Member* tree;
		int max;
		Compare compare;
	public:
		Group (int newId, Member* newTree, int newMax, Compare newCompare) :
			id(newId), size(1), tree(newTree), max(newMax), compare(newCompare) {
		}

		int getSize() {
			return size;
		}


		int getMax() {
			return max;
		}

		int getId() {
			return id;
		}


		void checkMax(int newMax) {
			if (compare(max,newMax)<0) {
				max=newMax;
			}
		}
		void increaseBy(int addition) {
			size+=addition;
		}
		void joinTo(Group* otherGroup) {
			tree->setParent(otherGroup->tree);
			tree->setGroup(NULL);
			otherGroup->checkMax(max);
			otherGroup->increaseBy(size);
			return;
		}
	};

	int size;
	Group** groupsArray;
	Member* itemsArray;
	Compare compare;

	bool checkIndex(int i) {
		if ((i<size) && (i>=0)) {
			return true;
		}
		return false;
	}

public:
	UnionFind(int newSize, Compare newCompare) :
		size(newSize), groupsArray(new Group*[newSize]),
		itemsArray(new Member[newSize]), compare(newCompare) {
			for (int i=0; i<size; i++) {
				groupsArray[i]=new Group(i, &(itemsArray[i]), i, compare);
				itemsArray[i].setId(i);
				itemsArray[i].setGroup(groupsArray[i]);
			}
	}

	int Union(int i, int j) {
		if (!checkIndex(i) || !checkIndex(j)
				|| !(groupsArray[i]) || !(groupsArray[j])) {
			return -1;
		}
		int smaller=j, larger=i;
		if (groupsArray[i]->getSize() < groupsArray[j]->getSize()) {
			smaller=i;
			larger=j;
		}
		groupsArray[smaller]->joinTo(groupsArray[larger]);
		delete groupsArray[smaller];
		groupsArray[smaller]=NULL;
		return smaller;

	}

	int getMax(int group) {
		if (!groupsArray[group]) {
			return -1;
		}
		return groupsArray[group]->getMax();
	}

	int getSize(int group) {
		if (!groupsArray[group]) {
			return -1;
		}
		return groupsArray[group]->getSize();
	}

	int find(int item) {
		if (!checkIndex(item)) {
			return -1;
		}

		Member* temp=&(itemsArray[item]);
		while (temp->getParent()) {
			temp=temp->getParent();
		}
		int result= temp->getGroup()->getId();
		Member* root=temp;
		temp=&(itemsArray[item]);
		while (temp->getParent()) {
			Member* tempParent=temp->getParent();
			temp->setParent(root);
			temp=tempParent;
		}
		return result;
	}

	void refreshGroup(int item) {
		int group=find(item);
		groupsArray[group]->checkMax(item);
	}

	~UnionFind() {
		for (int i=0; i<size; i++) {
			if (groupsArray[i]) {
				delete groupsArray[i];
			}
		}
		delete[] groupsArray;
		delete[] itemsArray;
	}
};

#endif /* UNIONFIND_H_ */
