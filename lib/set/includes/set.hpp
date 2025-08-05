#pragma once

#include <array>
#include <stdexcept>

template <typename ElementType>
class Set
{
    public:
        virtual void add(const ElementType& element) = 0;

        virtual bool remove(const ElementType& element) = 0;

        virtual bool isFound(const ElementType& element) = 0;

        virtual ElementType pop() = 0;

        virtual size_t getLength() = 0;
};

template <typename ElementType, size_t BufferSize>
class StaticSet : public Set<ElementType>
{
    private:
        std::array<ElementType, BufferSize> buffer = {};
        size_t count = 0;

    public:
        void add(const ElementType& element);

        bool remove(const ElementType& element);

        bool isFound(const ElementType& element);

        ElementType pop();

        size_t getLength();
};
#include "set.tpp"