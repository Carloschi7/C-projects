#CustomHeap

Virtual heap memory handler. Memory is allocated and managed by the class itself
Both raw memory and dynamically constructed objects can be allocated.
If dynamically allocated objects are retained within the custom heap,
they will be freed automatically in the destructor