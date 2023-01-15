/*
 * Vector_queue.h
 *
 *  Created on: 2021. 11. 29.
 *      Author: hong
 */

#ifndef VECTOR_QUEUE_H_
#define VECTOR_QUEUE_H_


#include "pch.h"

template <typename T>

class Vector {
	T* data;
	int capacity;
	int length;

public:
	Vector(int n =1) : data(new T[n]), capacity(n), length(0){}

	void push_back(T s) {
		if(capacity <= length) {
			T* temp = new T[capacity *2 ];
			for(int i=0; i< length; i++) {
				temp[i] = data[i];
			}
			delete[] data;
			data = temp;
			capacity *= 2;
		}

		data[length] = s;
		length++;
	}

	T operator[] (int i) {return data[i];}

	void remove(int x) {
		for(int i=x+1; i< length; i++) {
			data[i-1] = data[i];
		}
		length--;
	}

	int size() { return length;}

	~Vector() {
		if(data ) {
			delete[] data;
		}
	}

};

#endif /* VECTOR_QUEUE_H_ */
