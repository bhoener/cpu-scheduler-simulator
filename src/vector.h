#ifndef VECTOR_H
#define VECTOR_H
#include "process.h"
#include <iostream>
using namespace std;

/**
 * @class Vector
 * same as ArrayList implementation
 */
template <typename T>
class Vector
{
private:
    T *data;
    int size;
    int capacity;
    static const int DEFAULT_CAPACITY = 10;
    void reallocate();

public:
    Vector();
    Vector(int);
    ~Vector();
    Vector(const Vector&);
    void add(const T &);

    T remove();

    bool remove(const T &);

    T get(int);

    void clear();

    int getSize();

    template <typename Type>
    friend std::ostream &operator<<(std::ostream &, const Vector<Type> &);
};

template class Vector<Process>;
template class Vector<pair<int, int>>;
template class Vector<pair<int, bool >>;
template class Vector<int>;

#endif