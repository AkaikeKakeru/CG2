#pragma once
#include "Struct.h"

class Object
{
public:
	Object();
	Object(float x, float y, float z);
	~Object();
	Transform transform_;
private:
	//Direction direction_;
};