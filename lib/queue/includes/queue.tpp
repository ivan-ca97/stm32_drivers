#include <stdexcept>

#include "queue.hpp"

template <typename ElementType, size_t BufferSize>
void StaticQueue<ElementType, BufferSize>::enqueue(const ElementType& element)
{
    if (isFull())
        throw std::overflow_error("Queue is full.");

    buffer[rear] = element;
    rear = (rear + 1) % BufferSize;
    ++count;
}

template <typename ElementType, size_t BufferSize>
ElementType StaticQueue<ElementType, BufferSize>::dequeue()
{
    if(isEmpty())
        throw std::underflow_error("Queue is empty.");

    size_t position = front;
    front = (front + 1) % BufferSize;
    --count;

    return buffer[position];
}

template <typename ElementType, size_t BufferSize>
ElementType StaticQueue<ElementType, BufferSize>::dequeue(uint16_t i)
{
    if (i >= count)
        throw std::out_of_range("Queue dequeue index out of range");

    auto index = (front + i) % BufferSize;
    auto element = buffer[index];

    for(uint16_t j = i; j < count - 1; j++)
    {
        auto overwritePosition = (front + j) % BufferSize;
        auto copyPosition = (front + j + 1) % BufferSize;

        buffer[overwritePosition] = buffer[copyPosition];
    }

    rear = (rear + BufferSize - 1) % BufferSize;
    --count;
    return element;
}

template <typename ElementType, size_t BufferSize>
ElementType* StaticQueue<ElementType, BufferSize>::peek(uint16_t i)
{
    if (i >= count)
        throw std::out_of_range("Queue peek index out of range");

    auto index = (front + i) % BufferSize;
    return &buffer[index];
}

template <typename ElementType, size_t BufferSize>
ElementType* StaticQueue<ElementType, BufferSize>::peek()
{
    if(isEmpty())
        return nullptr;

    return &buffer[front];
}

template <typename ElementType, size_t BufferSize>
bool StaticQueue<ElementType, BufferSize>::isEmpty() const
{
    return count == 0;
}

template <typename ElementType, size_t BufferSize>
bool StaticQueue<ElementType, BufferSize>::hasData() const
{
    return count > 0;
}

template <typename ElementType, size_t BufferSize>
bool StaticQueue<ElementType, BufferSize>::isFull() const
{
    return count == BufferSize;
}

template <typename ElementType, size_t BufferSize>
size_t StaticQueue<ElementType, BufferSize>::size() const
{
    return count;
}