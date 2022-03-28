#include "CustomHeap.h"


CustomHeap::CustomHeap()
{
	m_Mem.reserve(MAX_HEAP_SIZE);
}

CustomHeap::CustomHeap(CustomHeap&& rhs) noexcept
{
	operator=(std::move(rhs));
}

CustomHeap& CustomHeap::operator=(CustomHeap&& rhs) noexcept
{
	m_Mem = std::move(rhs.m_Mem);
	m_AllocationList = std::move(rhs.m_AllocationList);
	return *this;
}

//When the heap dies, constructed objs need to be deleted manually
CustomHeap::~CustomHeap()
{
	for (auto& block : m_AllocationList)
		if (block.bConstructed)
			block.destroy_func(block.ptr);

	m_AllocationList.clear();
}

void* CustomHeap::HeapAllocate(std::size_t bytes, bool automatic_block_alloc)
{
	uint8_t* ptr = m_Mem.data();
	bool bSpaceFound = false;

	//Checking for allocation space between blocks
	for (uint32_t i = 0; i < m_AllocationList.size(); i++)
	{
		const Block& rec = m_AllocationList[i];
		if (static_cast<std::size_t>(rec.ptr - ptr) >= bytes)
		{
			bSpaceFound = true;
			break;
		}
		ptr = rec.ptr + rec.size;
	}

	//At the end we check space between the lasp block and the end of the heap
	if (!bSpaceFound)
	{
		if (static_cast<std::size_t>(m_Mem.data() + m_Mem.size() - ptr) >= bytes)
			bSpaceFound = true;
		else if (static_cast<std::size_t>(MAX_HEAP_SIZE - m_Mem.size()) >= bytes)
		{
			//If we have enough space, its convenient to allocate an amount of space
			//greater than the original size of the obj
			std::size_t incr = bytes * 4;
			if (static_cast<std::size_t>(MAX_HEAP_SIZE - m_Mem.size()) >= incr)
				m_Mem.resize(m_Mem.size() + incr);
			else
				m_Mem.resize(m_Mem.size() + bytes);

			bSpaceFound = true;
		}

	}
	

	if (!bSpaceFound)
	{
		LOG_AND_RET("Out of free memory, allocation failed", nullptr);
	}
	

	if (automatic_block_alloc)
	{
		m_AllocationList.push_back({ ptr, bytes, false });
		std::sort(m_AllocationList.begin(), m_AllocationList.end(), [](const Block& r1, const Block& r2)
			{return r1.ptr < r2.ptr; });
	}

	return ptr;
}

void CustomHeap::HeapFree(void* ptr)
{
	uint32_t index = FindBlockWithPtr(ptr);
	
	if (index == static_cast<uint32_t>(-1))
	{
		LOG_AND_RET("Block with address " << ptr << " not found\n", );
	}

	Block& b = m_AllocationList[index];
	if (b.bConstructed)
		b.destroy_func(ptr);

	std::memset(ptr, 0, b.size);

	//Erase block
	m_AllocationList.erase(m_AllocationList.begin() + index);
}


void* CustomHeap::DetachRaw(void* ptr)
{
	uint32_t index = FindBlockWithPtr(ptr);
	if (index == static_cast<uint32_t>(-1))
	{
		LOG_AND_RET("Block with address " << ptr << " not found\n", nullptr);
	}

	Block& b = m_AllocationList[index];
	
	if (b.bConstructed)
	{
		LOG_AND_RET("Constructed items " << ptr << " need to be detached via the DetachConstructed method\n", nullptr);
	}

	uint8_t* newptr = new uint8_t[b.size];
	std::memcpy(newptr, b.ptr, b.size);
	std::memset(b.ptr, 0, b.size);
	m_AllocationList.erase(m_AllocationList.begin() + index);
	return newptr;
}

uint32_t CustomHeap::FindBlockWithPtr(void* ptr)
{
	for (uint32_t i = 0; i < m_AllocationList.size(); i++)
		if (m_AllocationList[i].ptr == ptr)
		{
			return i;
		}


	return static_cast<uint32_t>(-1);
}
