/*
 * AVLTree.h
 *
 *  Created on: Dec 4, 2014
 *      Author: Lotem
 */

#ifndef AVLTREE_H_
#define AVLTREE_H_
#define AVL11 1

#include <ostream>
#include <iostream>
#include "stdlib.h"
using std::ostream;
using std::endl;
using std::cout;

#define	EQUAL 0

typedef enum {
	AVL_SUCCESS,
	AVL_MEMORY_ERROR,
	AVL_INVALID_INPUT,
	AVL_ELEMENT_EXISTS,
	AVL_ELEMENT_NOT_FOUND
} AvlResult;

template<class T, class Compare>
class AvlTree;

template<class T, class Compare>
class TreeNode {
	T data;
	TreeNode* source;
	TreeNode* left;
	TreeNode* right;
	int rankL;
	int rankR;
	int tempRank;
	int heightL;
	int heightR;
	int tempHeight;
	AvlTree<T, Compare> *tree;
	Compare compare;

	AvlResult search(const T& searchKey, TreeNode** source) {
		this->tempHeight = heightCalc();
		int result = compare(searchKey, data);
		*source = this;
		if (result == EQUAL) {
			return AVL_ELEMENT_EXISTS;
		} else if ((result > 0) && (right)) {
			return right->search(searchKey, source);
		} else if ((result < 0) && (left)) {
			return left->search(searchKey, source);
		}
		return AVL_ELEMENT_NOT_FOUND;
	}

	AvlResult replaceSon(TreeNode* oldSon, TreeNode* newSon) {
		if (!oldSon) {
			return AVL_INVALID_INPUT;
		}
		if (right == oldSon) {
			right = newSon;
		} else if (left == oldSon) {
			left = newSon;
		} else
			return AVL_ELEMENT_NOT_FOUND;
		return AVL_SUCCESS;
	}

	int heightCalc() {
		if (heightL >= heightR)
			return heightL + 1;
		return heightR + 1;
	}

	int sonsAmount() {
		int sum = 0;
		if (left) {
			sum++;
		}
		if (right) {
			sum++;
		}
		return sum;
	}


	void swapNodes(TreeNode* otherNode) {
		TreeNode* tempLeftBranch = left;
		TreeNode* tempRightBranch = right;
		TreeNode* tempSource = source;
		int tempRightHeight = heightR;
		int tempLeftHeight = heightL;
		int oldTempHeight = tempHeight;


		if (source) {
			source->replaceSon(this, otherNode);
		} else {
			tree->getNewRoot(otherNode);
		}
		if (otherNode->source) {
			otherNode->source->replaceSon(otherNode, this);
		}

		left = otherNode->left;
		right = otherNode->right;
		if ((otherNode->source)!=this) {
			source = otherNode->source;
		} else {
			source=otherNode;
		}
		heightR = otherNode->heightR;
		heightL = otherNode->heightL;
		tempHeight = otherNode->tempHeight;

		if (tempLeftBranch!=otherNode) {
			otherNode->left = tempLeftBranch;
			if (tempLeftBranch) {
				tempLeftBranch->source=otherNode;
			}
		} else {
			otherNode->left=this;
		}

		if (tempRightBranch!=otherNode) {
			otherNode->right = tempRightBranch;
			if (tempRightBranch) {
				tempRightBranch->source=otherNode;
			}
		} else {
			otherNode->right=this;
		}
		otherNode->source = tempSource;
		otherNode->heightR = tempRightHeight;
		otherNode->heightL = tempLeftHeight;
		otherNode->tempHeight = oldTempHeight;
	}

public:
	void calcHeights() {
		if (right) {
			heightR = right->heightCalc();
		} else {
			heightR = -1;
		}
		if (left) {
			heightL = left->heightCalc();
		} else {
			heightL = -1;
		}
		return;
	}

	TreeNode* getMinNode() {
		if (left) {
			return left->getMinNode();
		}
		return this;
	}


	TreeNode* getMaxNode() {
		if (right) {
			return right->getMaxNode();
		}
		return this;
	}

	int BF() {
		return heightL - heightR;
	}

	TreeNode(const T data, AvlTree<T, Compare>* containingTree = NULL,
			Compare newCompare=Compare()) :
			data(data), source(NULL),  left(NULL), right(NULL), rankL(0),
			rankR(0), tempRank(0),heightL(-1),heightR(-1), tempHeight(0),
			tree(containingTree),compare(newCompare) {
	};

	TreeNode* find(const T& searchKey) {
		TreeNode* foundNode;
		AvlResult searchResult = search(searchKey, &foundNode);
		if (searchResult == AVL_ELEMENT_EXISTS) {
			return foundNode;
		}
		return NULL;
	}

	bool isIn(const T& searchKey) {
		TreeNode* foundNode;
		AvlResult searchResult = search(searchKey, &foundNode);
		if (searchResult == AVL_ELEMENT_EXISTS) {
			return true;
		}
		return false;
	}


	TreeNode* RightRoll() {
		TreeNode* OriginalLeft = left;
		if (source) {
			if (source->replaceSon(this, OriginalLeft) != AVL_SUCCESS) {
				return NULL;
			}
		} else {
			tree->getNewRoot(OriginalLeft);
		}
		OriginalLeft->source = source;
		source = OriginalLeft;
		left = OriginalLeft->right;
		if (left) {
			left->source = this;
		}
		OriginalLeft->right = this;
		heightL = OriginalLeft->heightR;
		rankL = OriginalLeft->rankR;
		OriginalLeft->heightR = this->heightCalc();
		return OriginalLeft;
	}

	TreeNode* LeftRoll() {
		TreeNode* OriginalRight = right;
		if (source) {
			if (source->replaceSon(this, OriginalRight)
					!= AVL_SUCCESS) {
				return NULL;
			}
		} else {
			tree->getNewRoot(OriginalRight);
		}
		OriginalRight->source = source;
		source = OriginalRight;
		right = OriginalRight->left;
		if (right) {
			right->source = this;
		}
		OriginalRight->left = this;
		heightR = OriginalRight->heightL;
		OriginalRight->heightL = this->heightCalc();
		return OriginalRight;
	}

	TreeNode* LL() {
		return RightRoll();
	}

	TreeNode* RR() {
		return LeftRoll();
	}

	TreeNode* LR() {
		if (left->LeftRoll()) {
			return RightRoll();
		} else
			return NULL;
	}

	TreeNode* RL() {
		if (right->RightRoll()) {
			return LeftRoll();
		} else
			return NULL;
	}


	void Balance(bool removing, bool onlyUpdateHeights) {
		int oldHeight = tempHeight;
		TreeNode* newRoot = this;
		calcHeights();
		if (!onlyUpdateHeights) {
			int curBF = BF();
			if (curBF == 2) {
				if (left->BF() >= 0) {
					newRoot = LL();
				} else {
					newRoot = LR();
				}
				if (!removing) {
					onlyUpdateHeights = true;
				}

			} else if (curBF == (-2)) {
				if (right->BF() <= 0) {
					newRoot = RR();
				} else {
					newRoot = RL();
				}
				if (!removing) {
					onlyUpdateHeights = true;
				}

			}
			if (oldHeight == newRoot->heightCalc()) {
				onlyUpdateHeights = true;
			}
		}

		if (newRoot->source) {
			newRoot->source->Balance(removing, onlyUpdateHeights);
		}
		return;
	}

	AvlResult insert(TreeNode* newNode) {
		TreeNode* source;
		AvlResult searchResult = search(newNode->data, &source);
		if (searchResult == AVL_ELEMENT_EXISTS) {
			return searchResult;
		}
		if (compare(newNode->data, source->data) > 0) {
			source->right = newNode;
			newNode->source = source;
			source->heightR++;
		} else {
			source->left = newNode;
			newNode->source = source;
			source->heightL++;
		}
		source->Balance(false, false);
		return AVL_SUCCESS;
	}


	TreeNode* remove(const T& oldData) {

		TreeNode* target = NULL;
		TreeNode* subtitute = NULL;
		AvlResult searchResult = search(oldData, &target);
		if (searchResult == AVL_ELEMENT_NOT_FOUND) {
			return NULL;
		}

		if (target->sonsAmount() == 2) {
			TreeNode* successor = target->right->getMinNode();
			target->swapNodes(successor);
		}
		if (target->sonsAmount() == 0) {
			subtitute = target->source;
			if (subtitute) {
				subtitute->replaceSon(target, NULL);
			} else {
				tree->getNewRoot(NULL);
			}
		} else {
			if (target->right) {
				subtitute = target->right;
			} else {
				subtitute = target->left;
			}
			if (target->source) {
				target->source->replaceSon(target, subtitute);
			} else {
				tree->getNewRoot(subtitute);
			}
			subtitute->source = target->source;
			subtitute->tempHeight = target->tempHeight;
		}

		if (subtitute) {
			subtitute->Balance(true, false);
		}
		return target;
	}

	const T& getData() {
		return data;
	}

	void setData(const T& newData) {
		data=newData;
	}

	int buildEmptyCompleteTree(int maxDepth, int curDepth) {
		if (curDepth>=maxDepth) {
			return 1;
		}

		left=new TreeNode(T(), this->tree, compare);
		right=new TreeNode(T(), this->tree, compare);
		left->source=right->source=this;
		int count1=left->buildEmptyCompleteTree(maxDepth, curDepth+1);
		int count2=right->buildEmptyCompleteTree(maxDepth, curDepth+1);
		return count1+count2+1;
	}

	TreeNode* dropLeaves(int wantedSize, int& curSize) {
		TreeNode* toDrop=NULL;
		if (right) {
			toDrop=right->dropLeaves(wantedSize, curSize);
			if (toDrop) {
				delete(toDrop);
				right=NULL;
			}
		}

		if ((!right) && (!left) && (curSize>wantedSize)) {
			curSize--;
			return this;
		}

		if (left) {
			toDrop=left->dropLeaves(wantedSize, curSize);
			if (toDrop){
				delete(toDrop);
				left=NULL;
			}
		}
		return NULL;
	}

	template<class Action>
	void inOrder(Action& Do) {
		if (left) {
			left->inOrder(Do);
		}
		Do(this);
		if (right) {
			right->inOrder(Do);
		}
		return;
	}

	template<class Action>
	void postOrder(Action& Do) {
		if (left) {
			left->postOrder(Do);
		}
		if (right) {
			right->postOrder(Do);
		}
		Do(this);
		return;
	}

	int rankCheck(int k){
			if (rankL == k-1){
				return -1;
			}
			if (rankL > k-1){
				return -2;
			}
			return -3;
	}
};

template<class T, class Compare>
class AvlTree {
	TreeNode<T, Compare>* root;
	int size;
	bool empty;
	TreeNode<T, Compare>* maxNode;
	Compare compare;

	class Deleter {
	public:
		void operator()(TreeNode<T, Compare>* node) {
			delete (node);
		}
	};

	class Freer {
	public:
		void operator()(TreeNode<T, Compare>* node) {
			delete (node->getData());
		}
	};

	class HeightsUpdater {
	public:
		void operator()(TreeNode<T, Compare>* node) {
			node->calcHeights();
		}
	};

	class copierToArray {
		T*& arr;
		int size;
		int& index;
	public:
		copierToArray(T*& newArr, int newSize, int& newIndex) :
				arr(newArr), size(newSize), index(newIndex) {
			index=0;
		}
		void operator()(TreeNode<T, Compare>* node) {
			if (index < size) {
				arr[index] = node->getData();
			}
			index++;
		}
	};

	int calcWantedHeight(int treeSize) {
		if (treeSize==1) {
			return 0;
		}
		return 1+calcWantedHeight(treeSize/2);
	}


public:

	class Printer {
		public:
			void operator()(TreeNode<T, Compare>* node) {
				cout << node->getData();
				cout << " ";
			}
	};



	void printInOrder() {
		Printer printer;
		this->inOrder(printer);
	}

	class DataUpdater {
		T*& arr;
		int& index;
	public:
		DataUpdater(T*& sourceArray, int& newIndex) :
			arr(sourceArray), index(newIndex) {
			index=0;
		};
		void operator()(TreeNode<T, Compare>* node) {
			node->setData(arr[index]);
			index++;
		}
	};

	AvlResult buildEmptyTree(int newSize) {
		if (newSize>=0) {
			return AVL_SUCCESS;
		}
		root=new TreeNode<T, Compare>(T(), this, compare);
		empty=false;
		int wantedHeight=calcWantedHeight(newSize);
		int curSize=root->buildEmptyCompleteTree(wantedHeight, 0);
		if (root->dropLeaves(newSize, curSize)) {
			return AVL_INVALID_INPUT;
		}
		HeightsUpdater updater;
		root->postOrder(updater);
		return AVL_SUCCESS;
	}

	AvlTree(Compare newComparer = Compare()) :
			root(NULL), size(0), empty(true), maxNode(NULL), compare(newComparer) {
	}
	;


	TreeNode<T, Compare>* getRoot() {
		return root;
	}

	void getNewRoot(TreeNode<T, Compare>* newRoot) {

		root = newRoot;
		if (!root) {
			empty = true;
		}
		return;
	}

	int selectKTree(int k,int* arr,int size){
		int * tempArr = new int[size];
		inOrderToArray(tempArr,size);
		int found = tempArr[k];
		delete[] tempArr;
		return found;
	}

	T find(const T& searchKey) {
		if (root) {
			TreeNode<T, Compare>* node = root->find(searchKey);
			if (node) {
				const T foundData = node->getData();
				return foundData;
			}
		}
		return NULL;
	}

	bool isIn(const T& searchKey) {
		if(root) {
			if (root->isIn(searchKey) == true) {
				return true;
			}
		}
		return false;
	}

	AvlResult insert(const T newData) {
		AvlResult result = AVL_SUCCESS;
		TreeNode<T, Compare>* tempNode = new TreeNode<T, Compare>(newData,
				this, compare);
		if (empty) {
			root = tempNode;
			empty = false;
			maxNode = tempNode;
		} else {
			result = root->insert(tempNode);
		}
		if (result == AVL_SUCCESS) {
			size++;
			if (compare(tempNode->getData(), maxNode->getData()) > 0) {
				maxNode = tempNode;
			}
		} else {
			delete tempNode;
		}
		return result;

	}

	void inorderUpdater(T* newArray, int newSize,DataUpdater updater) {
		for (int i=0;i<newSize;i++){
			insert(newArray[i]);
		}
	}

	AvlResult remove(const T& oldData) {
		TreeNode<T, Compare>* oldNode = NULL;
		if (root) {
			oldNode = root->remove(oldData);
		}
		if (oldNode) {
			if (oldNode == maxNode) {
				if (root) {
					maxNode = root->getMaxNode();
				} else {
					maxNode = NULL;
				}
			}
			delete (oldNode);
			size--;
			return AVL_SUCCESS;
		}

		return AVL_ELEMENT_NOT_FOUND;
	}

	template<class Action>
	void inOrder(Action& Do) {
		if (root) {
			root->inOrder(Do);
		}
		return;
	}

	template<class Action>
	void postOrder(Action& Do) {
		if (root) {
			root->postOrder(Do);
		}
		return;
	}

	void newTreeFromArray(T* newArray, int newSize) {
		buildEmptyTree(newSize);
		int index=0;
		DataUpdater updater(newArray, index);
		inorderUpdater(newArray,newSize,updater);
		size=newSize;
		if (root) {
			maxNode = root->getMaxNode();
		} else {
			maxNode = NULL;
		}
	}

	void freeData() {
		Freer freer;
		if (root) {
			root->postOrder<Freer>(freer);
		}
	}
	~AvlTree() {
		Deleter del;
		if (root) {
			root->postOrder<Deleter>(del);
		}
	}

	int select(int k,int* arr,int size){
		if (root->rankCheck(k) > 0){
				return root->getData();
		}
		return selectKTree(k,arr,size);
	}

	void inOrderToArray(T* arr,int size){
		int index = 0;
		copierToArray copier(arr, size, index);
		inOrder<copierToArray>(copier);
	}

};

#endif /* AVLTREE_H_ */
