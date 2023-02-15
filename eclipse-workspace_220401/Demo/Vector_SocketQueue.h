/*
 * Vector_SocketQueue.h
 *
 *  Created on: 2021. 12. 15.
 *      Author: hong
 */

#ifndef VECTOR_SOCKETQUEUE_H_
#define VECTOR_SOCKETQUEUE_H_

#include "pch.h"
#include <vector>
#if 0
template <typename T>

class VectorSocket {
	T* data;
	int capacity;
	int length;

public:
	VectorSocket(int n =1) : data(new T[n]), capacity(n), length(0){}

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

	~VectorSocket() {
		if(data ) {
			delete[] data;
		}
	}

};
#else
#endif

#endif /* VECTOR_SOCKETQUEUE_H_ */
