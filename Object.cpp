#include "Object.h"

Object::Object()
{
	transform_.direction_.x = 0.4;
	transform_.direction_.y = 0.7;
	transform_.direction_.z = 0.0;
}

Object::Object(float x,float y,float z)
{
	this->transform_.direction_.x = x;
	this->transform_.direction_.y = y;
	this->transform_.direction_.z = z;
}
Object::~Object()
{

}