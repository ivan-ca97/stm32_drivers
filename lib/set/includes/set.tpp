#pragma once

#include <algorithm>

#include "set.hpp"

template <typename ElementType, size_t BufferSize>
void StaticSet<ElementType, BufferSize>::add(const ElementType& element)
{
    if(count >= BufferSize)
        throw std::overflow_error("Set is full.");

    if(isFound(element))
        throw std::logic_error("Element already present.");

    buffer[count++] = element;
}

template <typename ElementType, size_t BufferSize>
bool StaticSet<ElementType, BufferSize>::remove(const ElementType& element)
{
    for(std::size_t i = 0; i < count; ++i)
    {
        if (buffer[i] == element)
        {
            count--;
            // Move all elements to the left to fill out the space.
            for (std::size_t j = i; j < count; j++)
                buffer[j] = buffer[j + 1];
            buffer[count] = nullptr;

            return true;
        }
    }
    return false;
}

template <typename ElementType, size_t BufferSize>
bool StaticSet<ElementType, BufferSize>::isFound(const ElementType& element)
{
    return std::find(buffer.begin(), buffer.end(), element) != buffer.end();
}

template <typename ElementType, size_t BufferSize>
ElementType StaticSet<ElementType, BufferSize>::pop()
{
    if(count == 0)
        throw std::underflow_error("Set is empty.");

    count--;
    return buffer[count];
}

template <typename ElementType, size_t BufferSize>
size_t StaticSet<ElementType, BufferSize>::getLength()
{
    return count;
}