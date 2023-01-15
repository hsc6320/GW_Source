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

template <typename T>
class DataArrayVector
{
public:
	DataArrayVector(unsigned int maxSizeX
    		, unsigned int maxSizeY
			, std::vector<T> dataArray)
        : m_isInit(false)
        , m_dataArray(dataArray)
        , m_maxSizeX(maxSizeX)
        , m_maxSizeY(maxSizeY)
    {
        if (m_dataArray.size() == (m_maxSizeX * m_maxSizeY))
            m_isInit = true;
    }
    ~DataArrayVector()
    {
        m_isInit = false;
    }

    // 제대로 세팅 되었는지 여부^M
    bool IsInit() { return m_isInit; }
    const int GetMaxSizeX() { return m_maxSizeX; }
    const int GetMaxSizeY() { return m_maxSizeY; }

    T operator()(unsigned int x, unsigned int y)
    {
        return m_dataArray.at((y * m_maxSizeX) + x);
    }
    /*const T operator()(unsigned int x, unsigned int y) const
    {
        return m_dataArray.at((y * m_maxSizeX) + x);
    }*/


public:
    bool m_isInit;

	const unsigned int m_maxSizeX;
	const unsigned int m_maxSizeY;
    std::vector<BYTE>& m_dataArray;

};
#endif

#endif /* VECTOR_SOCKETQUEUE_H_ */
