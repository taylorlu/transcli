/*
 *Author: Abdul Bezrati
 *Email : abezrati@hotmail.com
 */

#ifndef VECTOR2_H
#define VECTOR2_H

template <typename T2>
class Vector2
{
  public:
    Vector2(T2 X = 0, T2 Y = 0)
    {
      x = X;
      y = Y;
    }

    Vector2(const Vector2<T2> &source)
    {
      x = source.x;
      y = source.y;
    }

    Vector2<T2> &operator = (const Vector2 &right){
      x = right.x;
      y = right.y;
      return *this;
    }

    inline Vector2<T2> operator + (const Vector2<T2> &right)
    {
      return Vector2<T2>(right.x + x, right.y + y);
    }

    inline Vector2<T2> operator - (const Vector2<T2> &right){
      return Vector2<T2>(-right.x + x, -right.y + y);
    }


    inline Vector2<T2> operator * (const T2 scale){
      return Vector2<T2>(x*scale, y*scale);
    }

    inline Vector2<T2>  operator / (const T2 scale){
      if(scale) 
        return Vector2<T2>(x/scale, y/scale);
      return Vector2<T2>(0, 0);
    }

    inline Vector2<T2> &operator += (const Vector2<T2> &right)
    {
      x+=right.x;
      y+=right.y;
      return *this;
    }

    inline Vector2<T2> &operator -= (const Vector2<T2> &right)
    {
      x-=right.x;
      y-=right.y;
      return *this;
    }

    inline Vector2<T2> &operator *= (const T2 scale)
    {
      x*=scale;
      y*=scale;
      return *this;
    }

    inline Vector2<T2> &operator /= (const T2 scale){
      if(scale)
      {
        x /=scale;
        y /=scale;
      }
      return *this;
    }

    inline operator const T2*() const { return &x; }
    inline operator T2*()             { return &x; }   

	  inline const T2  operator[](int i) const { return ((T2*)&x)[i]; }
	  inline       T2 &operator[](int i)       { return ((T2*)&x)[i]; }

    bool operator == (const Vector2<T2> &right)
    {
      return (x == right.x &&
              y == right.y);
    }

    bool operator != (const Vector2<T2> &right)
    {
      return !(x == right.x &&
               y == right.y );
    }

    void set(T2 nx, T2 ny)
    {
      x = nx;
      y = ny;
    }
	inline const T2 getDistance(const Vector2<T2> &p) const    //newly add (change)
	{
		return sqrtf((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y));
	}
    inline void clamp(T2 min, T2 max)
    {
      x = x > max ? max : x < min ? min  : x;
      y = y > max ? max : y < min ? min  : y;
    }

    T2 x, y;
};

typedef Vector2<int   > Vector2i;
typedef Vector2<unsigned> Vector2ui;
typedef Vector2<float > Vector2f;
typedef Vector2<double> Vector2d;

#endif
