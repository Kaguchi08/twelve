#include "Primitive.h"

Primitive::Primitive() :
	vertex_buffer_(nullptr),
	index_buffer_(nullptr),
	vertex_num_(0),
	index_num_(0)
{
}

Primitive::~Primitive()
{
}
