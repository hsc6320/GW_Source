/*
 * Vector_SocketQueue.h
 *
 *  Created on: 2021. 12. 15.
 *      Author: hong
 */

#ifndef VECTOR_SOCKETQUEUE_H_
#define VECTOR_SOCKETQUEUE_H_

#include "pch.h"

template <typename T>

class VectorSocket {
	/*T* data;
	int capacity;
	int length;*/

public:
	VectorSocket(int n =1) /*: data(new T[n]), capacity(n), length(0)*/{}
	
	void InsertArray(int idx, T sz, T* ar)
	{
		BYTE arr[4096];
		memcpy(arr, ar, 4096);
		int size = (sizeof(arr)/sizeof(*arr));
		memmove(ar+idx+1, ar+idx, size-idx+1);
		ar[idx] = sz;
		printf("Insert %x\n", ar[idx]);
	}
	
	void deleteArray(int idx, int size, T* ar)
	{
		memmove(ar+idx, ar+idx+1, size-idx);
	}
	
	void AppendArray(BYTE sz, int size1, T* ar)
	{
		InsertArray(size1, sz, ar);
	}
	
	int GetSizeArray (T* ar)
	{
		int ret =0;
		BYTE arr[4096];
		memcpy(arr, ar, 4096);
		int size = (sizeof(arr)/sizeof(*arr));
		
		for(int i=0; i< size; i++) {
			if(ar[i] > 0) {
				ret++;
			}
		}
		return ret;
	}
	
	void PrintArray(T* ar, int size)
	{
		for(int i=0; i< size; i++) {
		//	if(ar[i] > 0)		
				printf("%x ", ar[i]);
		}	
		printf("\n\n");
	}

};
#endif /* VECTOR_SOCKETQUEUE_H_ */
