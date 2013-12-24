/*
*Author: Abdul Bezrati
*Email : abezrati@hotmail.com
*/

#ifndef MATHUTILS
#define MATHUTILS

#include <float.h>	 
#include <math.h>
#include <ctime>
#include <iostream>
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4.h"
#include "Quaternion.h"


#define TWO_PI          6.2831853f
#define EPSILON         0.0001f
#define EPSILON_SQUARED EPSILON*EPSILON
#define RAD2DEG         57.2957f
#define DEG2RAD         0.0174532f


template <class T>
inline void swapValue(T a, T b)
{
	T temp;
	temp = a; a = b; b = temp;
}
template <class T>
inline T lerp(T a, T b, float aRatio)
{
	return a*aRatio + b*(1-aRatio);
}
template <class T>
inline T clamp(T x, T min, T max)
{ 
	return (x < min) ? min : (x > max) ? max : x;
}

inline int round(float f)
{
	return int(f + 0.5f);
}

template <class T>
inline bool equalFloat(T x, T y, float epsilon = EPSILON)
{ 
	if(x-y < epsilon && x-y > -epsilon) {
		return true;
	} 
	return false;
}

inline float getNextRandom(){
	return (float)rand()/(RAND_MAX + 1);
}

inline int getClosest(int arg, int firstVal, int secondVal)
{
	int difference1 = 0,
		difference2 = 0;

	difference1 = abs(arg - firstVal);
	difference2 = abs(arg - secondVal);
	if(difference1 < difference2)
		return firstVal;

	return secondVal;
}
inline int  power(int x,int y)
{
	int temp=1;
	for(int i=0;i<y;i++)
		temp*=x;
	return temp;
}
inline int getClosestPowerOfTwo(int digit)
{
	if(!digit)
		return 1;

	double log2  = log(double(abs(digit)))/log(2.0),
		flog2 = floor(log2),
		frac  = log2 - flog2;

	return (frac < 0.5) ? (int)pow(2.0, flog2) : (int)pow(2.0, flog2 + 1.0);
}

inline float fastSquareRoot(float x)
{
	__asm{
		fld x;
		fsqrt;
		fstp x;
	}
	return x;
}

inline float fastCos(float x)
{
	__asm{
		fld x;
		fcos;
		fstp x;
	}
	return x;
}

inline float fastSin(float x)
{
	__asm{
		fld x;
		fsin;
		fstp x;
	}
	return x;
}
//////////////////////////////////////////////////////////////////////////
//	新增数学函数
//////////////////////////////////////////////////////////////////////////

/*已知一点，射线角度，距离，求另一点*/
inline void ppad2D(const Vector2f& p,float angle,float dist,Vector2f& retP)
{
	retP.x=p.x+dist*fastCos(DEG2RAD*angle);
	retP.y=p.y+dist*fastSin(DEG2RAD*angle);
}

/*已知线段两点，距离端点长度，求另一点*/
inline void pppd3D(const Vector3f& p1,const Vector3f& p2,float dist,Vector3f& retP)
{
	float aspect=dist/p1.getDistance(p2);
	//retP=p1+(p2-p1)*aspect (由于该类不支持左操作符，要分开写)
	retP=p2; retP-=p1;
	retP*=aspect; retP+=p1;
}

/*已知一点，射线角度，距离，求另一点*/
inline float app2D(const Vector2f& p1,const Vector2f& p2)
{
	float dist=fastSquareRoot((float)((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y)));
	double dltx=p2.x-p1.x;
	//	if(dist<=0) return Logger::writeErrorLog("两点距离为零");
	if(p2.y>=p1.y) return (float)acos(dltx/dist);
	else return (float)(TWO_PI-acos(dltx/dist));
}
/*判断某点是否在一平面三角形内*/
inline bool pointInTriangle(const Vector2f tri[3],const Vector2f& p)
{
	float u,v;
	Vector2f vec1=tri[1]; vec1-=tri[0];
	Vector2f vec2=tri[2]; vec2-=tri[0];
	Vector2f vecResult=p; vecResult-=tri[0];
	float det=vec1.x*vec2.y-vec2.x*vec1.y;
	if(-EPSILON<=det && det<=EPSILON)  return false;   //行列式为0，无解
	v=(vecResult.y*vec1.x-vecResult.x*vec1.y)/det;
	u=(vecResult.x*vec2.y-vecResult.y*vec2.x)/det;
	if(0<v && v<1 && 0<u && u<1 && 0<u+v && u+v<1)
		return true;
	return false;
}
/*inline float fastSquareRootSSE(float f)	{
  __asm    {    
   MOVSS xmm2,f     
   SQRTSS xmm1, xmm2     
   MOVSS f,xmm1   
  }    
  return f;
}*/

#endif
