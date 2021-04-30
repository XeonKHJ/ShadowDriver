#pragma once

void* operator new(size_t);
void operator delete(void*) noexcept;
void operator delete(void*, size_t);

void* operator new[](size_t);
void operator delete[](void*);
void operator delete[](void*, size_t);

