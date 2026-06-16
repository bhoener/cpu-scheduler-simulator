#include <iostream>
#include <cstring>
#include "vector.h"
using namespace std;

/**
 * @brief doubles size of array capacity
 * reallocates and moves data
 */
template <typename T>
void Vector<T>::reallocate()
{
    capacity *= 2;
    T *newArray = new T[capacity]();
    for (int i = 0; i < size; i++)
    {
        newArray[i] = this->data[i];
    }
    delete[] this->data;
    this->data = newArray;
}

/**
 * @brief capacity constructor
 * @param capacity the starting capacity of the vector
 */
template <typename T>
Vector<T>::Vector(int capacity)
{
    this->capacity = capacity;
    this->data = new T[capacity]();
    this->size = 0;
}

/**
 * @brief default constructor
 */
template <typename T>
Vector<T>::Vector() : Vector<T>(Vector<T>::DEFAULT_CAPACITY) {};

/**
 * @brief copy constructor
 * @param other the vector to copy
 */
template <typename T>
Vector<T>::Vector(const Vector<T> &other)
{
    this->size = other.size;
    delete[] this->data;
    this->data = new T[other.capacity]();
    for (int i = 0; i < other.size; i++)
    {
        this->data[i] = other.data[i];
    };
}

/**
 * @brief inserts an element to the back of the array
 * @param data the element to insert
 */
template <typename T>
void Vector<T>::add(const T &data)
{
    if (this->size >= capacity)
    {
        this->reallocate();
    }
    this->data[this->size++] = data;
}

/**
 * @brief removes from the back of the array
 * @return the element removed
 */
template <typename T>
T Vector<T>::remove()
{
    if (this->size == 0)
    {
        throw exception();
    }
    this->size--;
    return this->data[this->size];
}

/**
 * @brief removes given object
 * @param obj the object to remove
 * @return whether removed
 */
template <typename T>
bool Vector<T>::remove(const T &obj)
{
    for (int i = 0; i < this->size; i++)
    {
        if (this->data[i] == obj)
        {
            for (int j = i; j < this->size - 1; j++)
            {
                this->data[j] = this->data[j + 1];
            }
            this->size--;
            return true;
        }
    }
    return false;
}

/**
 * @brief gets element at given index
 * @param idx the index to look at
 * @return the element
 */
template <typename T>
T Vector<T>::get(int idx)
{
    if (0 <= idx && idx < this->size)
    {
        return this->data[idx];
    }
    else
    {
        throw out_of_range("list index out of range");
    }
}

/**
 * @brief "removes" all elements
 */
template <typename T>
void Vector<T>::clear()
{
    this->size = 0;
}

/**
 * @brief getter method for size attribute
 */
template <typename T>
int Vector<T>::getSize()
{
    return this->size;
}

/**
 * @brief destructor to deallocate used memory
 */
template <typename T>
Vector<T>::~Vector()
{
    delete[] this->data;
}

/**
 * @brief stream insertion operator
 */
template <typename T>
ostream &operator<<(ostream &os, const Vector<T> &al)
{
    os << "[";
    for (int i = 0; i < al.size; i++)
    {
        os << al.data[i] << (i < al.size - 1 ? ", " : "]");
    }
    return os;
}