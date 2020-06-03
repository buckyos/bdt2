#ifndef __BFX_VECTOR_H__
#define __BFX_VECTOR_H__

//typedef struct BfxVector {
//	void* data; 
//	size_t size; 
//	size_t allocsize; 
//} uivector;

#define BFX_VECTOR_DEFINE_CLEANUP(type, elemtype, data, size, allocsize) \
static void bfx_vector_##type##_cleanup(type* p) { \
	p->size = p->allocsize = 0;\
	free(p->data);\
	p->data = NULL;\
}

/*returns 1 if success, 0 if failure ==> nothing done*/
#define BFX_VECTOR_DEFINE_RESERVE(type, elemtype, data, size, allocsize) \
static unsigned bfx_vector_##type##_reserve(type* p, size_t _allocsize) {\
	if (_allocsize > p->allocsize) {\
		size_t newsize = (_allocsize > p->allocsize * 2) ? _allocsize : (_allocsize * 3 / 2);\
		elemtype* _data = realloc(p->data, newsize * sizeof(elemtype));\
		if (_data) {\
			p->allocsize = newsize;\
			p->data = _data;\
		}\
		else return 0; \
	}\
	return 1;\
}

/*returns 1 if success, 0 if failure ==> nothing done*/
#define BFX_VECTOR_DEFINE_RESIZE(type, elemtype, data, size, allocsize) \
static unsigned bfx_vector_##type##_resize(type* p, size_t _size) {\
	if (!bfx_vector_##type##_reserve(p, _size)) return 0;\
	p->size = _size;\
	return 1; \
}

/*resize and give all new elements the value*/
#define BFX_VECTOR_DEFINE_RESIZEV(type, elemtype, data, size, allocsize) \
static unsigned bfx_vector_##type##_resizev(type* p, size_t _size, elemtype value) {\
	size_t oldsize = p->size, i;\
	if (!bfx_vector_##type##_resize(p, _size)) return 0;\
	for (i = oldsize; i < _size; ++i) p->data[i] = value;\
	return 1;\
}

#define BFX_VECTOR_DEFINE_INIT(type, elemtype, data, size, allocsize) \
static void bfx_vector_##type##_init(type * p) {\
	p->data = NULL;\
	p->size = p->allocsize = 0;\
}

/*returns 1 if success, 0 if failure ==> nothing done*/
#define BFX_VECTOR_DEFINE_PUSH_BACK(type, elemtype, data, size, allocsize) \
static unsigned bfx_vector_##type##_push_back(type * p, elemtype c) {\
	if (!bfx_vector_##type##_resize(p, p->size + 1)) return 0;\
	p->data[p->size - 1] = c;\
	return 1;\
}

#define BFX_VECTOR_DEFINE_CLONE(type, elemtype, data, size, allocsize) \
static void bfx_vector_##type##_clone(const type* src, type* dest) {\
	if(src->size > dest->allocsize) {\
		bfx_vector_##type##_resize(dest,src->size); \
     } \
     for(size_t i=0;i<src->size;++i) { dest->data[i] = src->data[i]; }\
	 dest->size = src->size;\
}

#define BFX_VECTOR_DEFINE_FUNCTIONS(type, elemtype, data, size, allocsize) \
BFX_VECTOR_DEFINE_CLEANUP(type, elemtype, data, size, allocsize)\
BFX_VECTOR_DEFINE_RESERVE(type, elemtype, data, size, allocsize)\
BFX_VECTOR_DEFINE_RESIZE(type, elemtype, data, size, allocsize)\
BFX_VECTOR_DEFINE_RESIZEV(type, elemtype, data, size, allocsize)\
BFX_VECTOR_DEFINE_INIT(type, elemtype, data, size, allocsize)\
BFX_VECTOR_DEFINE_PUSH_BACK(type, elemtype, data, size, allocsize)\
BFX_VECTOR_DEFINE_CLONE(type, elemtype, data, size, allocsize) 

#define BFX_VECTOR_DECLARE_FUNCTIONS(type, elemtype, data, size, allocsize) \
static void bfx_vector_##type##_cleanup(type* p);\
static unsigned bfx_vector_##type##_reserve(type* p, size_t _allocsize);\
static unsigned bfx_vector_##type##_resize(type* p, size_t _size);\
static unsigned bfx_vector_##type##_resizev(type* p, size_t _size, elemtype value);\
static void bfx_vector_##type##_init(type * p);\
static unsigned bfx_vector_##type##_push_back(type * p, elemtype c);\
static void bfx_vector_##type##_clone(const type* src, type* dest);\

#endif //__BFX_VECTOR_H__