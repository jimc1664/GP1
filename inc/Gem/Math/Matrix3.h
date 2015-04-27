#ifndef GEM_MATH_MATRIX3_H
#define GEM_MATH_MATRIX3_H

#include "Vec3.h"

namespace Gem {

template<class typ> class mat3_T : public tuple<vec3_T<typ>, vec3_T<typ>, vec3_T<typ>>, public Convertable<mat3_T<typ>> {
	typedef mat3_T<typ> m3;
public:
	m3() {}
	Template1 m3( const T &a ) : tuple( convert<m3,T>(a) ) {}

};

}
typedef mat3_T<f32> mat3f;
typedef mat3_T<f64> mat3d;

#ifdef GEM_MATH_QUATERNION_H
#include "Quaternion.h"
#endif //GEM_MATH_QUATERNION_H

#endif //GEM_MATH_MATRIX3_H