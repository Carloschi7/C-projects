#pragma once

//CustomHeapv1.0
//Code written by carloschi7, first upload on 28/03/2022

#include <algorithm>
#include <iostream>
#include <functional>
#include <vector>

#define LOG_AND_RET(str, ret)std::cout << str; return ret

class CustomHeap;

struct Block
{
	uint8_t* ptr;
	std::size_t size;
	bool bConstructed;
	std::function<void(void*)> destroy_func;
	const char* type = nullptr;
};

//Custom allocator for Blocks
template<typename T>
struct LocalAllocator
{
	typedef T value_type;

	LocalAllocator() {};
	template <class U> constexpr LocalAllocator(const LocalAllocator<U>&) noexcept {}

	_NODISCARD T* allocate(std::size_t n) {
		return (T*)std::malloc(n * sizeof(T));
	}

	void deallocate(T* p, std::size_t) noexcept {
		std::free(p);
	}

};

//Virtual heap memory handled by the class
//returns pointers to any type of object.
//Data gets deleted automatically when
//the CustomHeap object dies
class CustomHeap
{
public:
	CustomHeap();
	CustomHeap(const CustomHeap&) = delete;
	CustomHeap(CustomHeap&& rhs) noexcept;

	CustomHeap& operator=(CustomHeap&& rhs) noexcept;

	~CustomHeap();

	_NODISCARD void* HeapAllocate(std::size_t bytes, bool automatic_block_alloc = true);
	
	//Allocates and constructs a single object
	template<typename T, typename... Args>
	_NODISCARD T* HeapAllocateAndConstruct(Args&&... args);

	//Allocates and constructs an array of objects with the same parameters
	template<typename T, uint32_t count, typename... Args>
	_NODISCARD T* HeapAllocateAndConstructArray(Args&&... args);

	void HeapFree(void* ptr);
	//Moves local obj pointer on standard heap and returns it
	template<typename T>
	_NODISCARD T* DetachConstructed(T* ptr);

	//Moves a simple allocated space on standard heap
	_NODISCARD void* DetachRaw(void* ptr);

private:
	template<typename T>
	void _ConstructedBlockDetails(T* ptr);
	uint32_t FindBlockWithPtr(void* ptr);

	//Local heap memory
	std::vector<uint8_t> m_Mem;
	//Keeps track of allocations
	std::vector<Block, LocalAllocator<Block>> m_AllocationList;
	static constexpr std::size_t MAX_HEAP_SIZE = 0x1000000;
};


template<typename T, typename ...Args>
inline T* CustomHeap::HeapAllocateAndConstruct(Args&&... args)
{
	static_assert(std::is_constructible_v<T> && std::is_move_assignable_v<T>, "T must be constructible and move assignable");
	T* ptr = (T*)HeapAllocate(sizeof(T));

	if (!ptr)
	{
		LOG_AND_RET("Not enough local memory available\n", nullptr);
	}
	

	T constructed{args...};
	*ptr = std::move(constructed);
		
	_ConstructedBlockDetails(ptr);

	return (T*)ptr;
}

template<typename T, uint32_t count, typename... Args>
inline T* CustomHeap::HeapAllocateAndConstructArray(Args&&... args)
{
	uint8_t* ptr = (uint8_t*)HeapAllocate(sizeof(T) * count, false);
	if (!ptr)
	{
		LOG_AND_RET("Not enough local memory available\n", );
	}

	//Allocate contiguous elements
	uint8_t* iter = ptr;
	for (uint32_t i = 0; i < count; i++, iter += sizeof(T))
	{
		T* cur = (T*)iter;
		T constructed{ args... };

		*cur = std::move(constructed);

		m_AllocationList.push_back({ (uint8_t*)cur, sizeof(T), false });
		_ConstructedBlockDetails((T*)cur);
	}

	std::sort(m_AllocationList.begin(), m_AllocationList.end(), [](const Block& r1, const Block& r2)
		{return r1.ptr < r2.ptr; });

	return (T*)ptr;
}

template<typename T>
inline T* CustomHeap::DetachConstructed(T* ptr)
{
	static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
	uint32_t index = FindBlockWithPtr(ptr);

	if (index == static_cast<uint32_t>(-1))
	{
		LOG_AND_RET("Block with address " << ptr << " not found\n", nullptr);
	}

	Block& b = m_AllocationList[index];

	if (!b.bConstructed)
	{
		LOG_AND_RET("The obj is not constructed\n", nullptr);
	}

	if (std::strcmp(typeid(T).name(), b.type) != 0)
	{
		LOG_AND_RET("Ptr type mismatch\n", nullptr);
	}

	T* newptr = new T(std::move(*(T*)b.ptr));
	b.destroy_func(b.ptr);
	std::memset(b.ptr, 0, b.size);
	m_AllocationList.erase(m_AllocationList.begin() + index);

	return newptr;
}

template<typename T>
inline void CustomHeap::_ConstructedBlockDetails(T* ptr)
{
	uint32_t index = FindBlockWithPtr(ptr);

	if (index == static_cast<uint32_t>(-1))
	{
		LOG_AND_RET("Block with address " << ptr << " not found\n", );
	}

	Block& b = m_AllocationList[index];
	b.bConstructed = true;
	b.destroy_func = [](void* ptr) { T* des = (T*)ptr; des->~T(); };
	b.type = typeid(T).name();
}
