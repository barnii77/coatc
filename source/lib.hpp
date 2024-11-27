#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <vector>

template<typename T>
struct SizedArray {
    T *m_data;
    size_t m_size;

    T &front() const;
    T &consumeFront();
    size_t size() const;
    SizedArray(std::vector<T> vec);
};

template<typename T>
T &SizedArray<T>::front() const {
    if (!m_size)
        throw std::runtime_error("called SizedArray::front() on empty SizedArray");
    return m_data[0];
}

template<typename T>
T &SizedArray<T>::consumeFront() {
    if (!m_size)
        throw std::runtime_error("called SizedArray::front() on empty SizedArray");
    m_size--;
    return *(m_data++);
}

template<typename T>
size_t SizedArray<T>::size() const {
    return m_size;
}

template<typename T>
SizedArray<T>::SizedArray(std::vector<T> vec) : m_data(vec.data()), m_size(vec.size())
{}

typedef struct StringRef {
    char const *start;
    uint32_t length;
} StringRef;

typedef struct LocationInfo {
    uint32_t line;
    uint32_t column;
    StringRef file;
} LocationInfo;

enum class OptLevel {
    O0,
    O1,
    Os,
    Oz,
    O2,
    O3,
};

enum class LTOKind {
    none,
    thin,
    full,
};
