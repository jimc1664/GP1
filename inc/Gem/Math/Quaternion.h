#ifndef GEM_MATH_QUATERNION_H
#define GEM_MATH_QUATERNION_H

#include "../Basic/Math.h"
#include "../Basic/Tuple.h"

namespace Gem {

template<class typ> class quat_T : public tuple<typ, typ, typ, typ>, public Convertable<quat_T<typ>>  {
	typedef quat_T<typ> quat;
public:
	quat() {}
	quat( Identity ) : tuple(identity()) {}
	Template4 quat( T &&a, T2 &&b, T3 &&c, T4 &&d ) : tuple(a,b,c,d) {}
	Template1 quat( const T &a ) : tuple( convert<quat,T>(a) ) {}
		
	Template1 quat_T<T> cast() const { return quat_T<T>((T)x, (T)y, (T)z, (T)w); }
	quat operator -() const { return quat( -x, - y, - z, -w ); }

	quat operator *(const quat &a) const {
		return quat( w*a.x + x*a.w + z*a.y - y*a.z,
					 w*a.y + y*a.w + x*a.z - z*a.x,
					 w*a.z + z*a.w + y*a.x - x*a.y,
					 w*a.w - x*a.x - y*a.y - z*a.z );
	}
	quat &operator *=(const quat &a) { *this = *this * a; return *this; }
	quat &operator *=(const f32 &a) { ((tuple)(*this))*=a; return *this; }

	quat& normalise() {
		typ lenInv = (typ)invSqrt(w*w + x*x + y*y + z*z);
		((tuple)(*this)) *= lenInv;
		return *this;
	}
	quat getNormal() const { quat q(*this); return q.normalise(); }
	quat getInverse() const { return quat( -x, -y, -z, w); }

	static quat identity() { return quat((typ)0.0, (typ)0.0,(typ) 0.0, (typ)1.0); }

	//T GetRotationAngle() const { return acos(w)*(T)2.0; }
	//vec3<T> GetRotationAxis() const { return Axis*invSqrt((T)1.0 - w*w); }


};

Template1 T dot( const quat_T<T> &a, const quat_T<T> &b ) { return a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z; }
Template2 T dot(const quat_T<T> &a, const T2 &b) { return dot<T>(a, quat_T<T>(b) ); }

/*
Template1 quat_T<T> nLerp( const quat_T<T> &a, const quat_T<T> &b, f32 d2 ) {
	T cosTheta = dot(a, b);
	T d1((T)1.0-d2);
	if( cosTheta < 0 )  d2 = -d2;
	return quat_T<T>( d1*a.x + d2*b.x, d1*a.y + d2*b.y, d1*a.z + d2*b.z, d1*a.w + d2*b.w ).getNormal();
}*/
Template1 quat_T<T> nLerp(const quat_T<T> &a, const quat_T<T> &c, T d2) {
	T cosTheta = dot(a, c);
	quat_T<T> b;
	if( cosTheta < 0 ) { 
		b = -c;
		cosTheta = -cosTheta;
	} else
		b = c;
	T d1((T)1.0-d2);
	return quat_T<T>( d1*a.x + d2*b.x, d1*a.y + d2*b.y, d1*a.z + d2*b.z, d1*a.w + d2*b.w ).getNormal();
}



template <class T> quat_T<T> sLerp(const quat_T<T> &a, const quat_T<T> &c, const T &t ) {

	T cosTheta = dot(a, c);

	quat_T<T> b;
	if( cosTheta < 0 ) {
		b = -c;
		cosTheta = -cosTheta;
	} else
		b = c;

	T d1, d2;
	if (cosTheta > 0.999999f) {
		d1 = (T)1.0-t;
		d2 = t;
	} else {
		T sinTheta = sqrt(((T)1.0) - cosTheta*cosTheta);
		T theta = atan2(sinTheta, cosTheta);
		T invSinTheta = (T)1.0 / sinTheta;
		d1 = sin(((T)1.0 - t) * theta) *invSinTheta;
		d2 = sin(t * theta) *invSinTheta;
	}

	return quat_T<T>( d1*a.x + d2*b.x, d1*a.y + d2*b.y, d1*a.z + d2*b.z, d1*a.w + d2*b.w );
}

template <class T> quat_T<T> cLerp(const quat_T<T> &a, const quat_T<T> &c, const f32 &t ) { //sLerps at a constant speed towards c, ie t radians

	T cosTheta = dot(a, c);

	quat_T<T> b;
	if( cosTheta < 0 ) {
		b = -c;
		cosTheta = -cosTheta;
	} else
		b = c;

	if (cosTheta > 0.999999f) return c;

	T sinTheta = sqrt(((T)1.0) - cosTheta*cosTheta);
	T theta = atan2(sinTheta, cosTheta);

	if( theta < t ) return c; 

	T invSinTheta = (T)1.0 / sinTheta;
	T d1 = sin(theta - t) *invSinTheta;
	T d2 = sin(t) *invSinTheta;
	

	return quat_T<T>( d1*a.x + d2*b.x, d1*a.y + d2*b.y, d1*a.z + d2*b.z, d1*a.w + d2*b.w );
}


typedef quat_T<f32> quatF;
typedef quat_T<f64> quatD;

template<> quatD Gem::convert<quatD, quatF>(const quatF &a) { return a.cast<f64>();  }
template<> quatF Gem::convert<quatF, quatD>(const quatD &a) { return a.cast<f32>();  }

}

#endif //GEM_MATH_QUATERNION_H 

#if !defined(GEM_CONV_QUAT_MAT3) && defined(GEM_MATH_MATRIX3_H)
#define GEM_CONV_QUAT_MAT3

template<> mat3f Gem::convert<mat3f,quatF>( const quatF &b ) {
	typedef f32 T;
	mat3f m;
	T x2 = b.x+b.x, y2 = b.y+b.y,		z2 = b.z+b.z;
	T xx = x2*b.x,	xy = x2*b.y,		xz = x2*b.z,	xw = x2*b.w;
	T yy = y2*b.y,	yz = y2*b.z,		yw = y2*b.w;
	T zw = z2*b.w,	zz = z2*b.z;

	m.x.x = 1.0f - ( yy + zz );
	m.y.x = ( xy - zw );
	m.z.x = ( xz + yw );

	m.x.y = ( xy +  zw );
	m.y.y = 1.0f - ( xx + zz );
	m.z.y = ( yz - xw );

	m.x.z = ( xz - yw );
	m.y.z = ( yz + xw );
	m.z.z = 1.0f - ( xx + yy );

	return m; 
}

template<> quatF Gem::convert<quatF,mat3f>( const mat3f &b ) {
	typedef f32 T;
	quatF q;
/*
	T trace = m._00 + m._11 + m._22;
	if( trace > 0 ) {
		T s = (T)0.5 / sqrt(trace+ (T)1.0);
		w = (T)0.25 / s;
		x = ( m._21 - m._12 ) * s;
		y = ( m._02 - m._20 ) * s;
		z = ( m._10 - m._01 ) * s;
	} else {
		if ( m._00 > m._11 && m._00 > m._22 ) {
			T s = (T)0.5 / sqrt( (T)1.0 + m._00 - m._11 - m._22);
			w = (m._21 - m._12 ) * s;
			x = (T)0.25 / s;
			y = (m._01 + m._10 ) * s;
			z = (m._02 + m._20 ) * s;
		} else if (m._11 > m._22) {
			T s = (T)0.5 / sqrt( (T)1.0 + m._11 - m._00 - m._22);
			w = (m._02 - m._20 ) * s;
			x = (m._01 + m._10 ) * s;
			y = (T)0.25 / s;
			z = (m._12 + m._21 ) * s;
		} else {
			T s = (T)0.5 / sqrt( (T)1.0 + m._22 - m._00 - m._11 );
			w = (m._10 - m._01 ) * s;
			x = (m._02 + m._20 ) * s;
			y = (m._12 + m._21 ) * s;
			z = (T)0.25 / s;
		}
	}
	*/
	return q;
}
/*quat( mat3<T> m ) {
/*void CalculateRotation( Quaternion& q ) const { //http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
  f32 trace = a[0][0] + a[1][1] + a[2][2]; // I removed + 1.0f; see discussion with Ethan
  if( trace > 0 ) {// I changed M_EPSILON to 0
    f32 s = 0.5f / sqrtf(trace+ 1.0f);
    q.w = 0.25f / s;
    q.x = ( a[2][1] - a[1][2] ) * s;
    q.y = ( a[0][2] - a[2][0] ) * s;
    q.z = ( a[1][0] - a[0][1] ) * s;
  } else {
    if ( a[0][0] > a[1][1] && a[0][0] > a[2][2] ) {
      f32 s = 2.0f * sqrtf( 1.0f + a[0][0] - a[1][1] - a[2][2]);
      q.w = (a[2][1] - a[1][2] ) / s;
      q.x = 0.25f * s;
      q.y = (a[0][1] + a[1][0] ) / s;
      q.z = (a[0][2] + a[2][0] ) / s;
    } else if (a[1][1] > a[2][2]) {
      f32 s = 2.0f * sqrtf( 1.0f + a[1][1] - a[0][0] - a[2][2]);
      q.w = (a[0][2] - a[2][0] ) / s;
      q.x = (a[0][1] + a[1][0] ) / s;
      q.y = 0.25f * s;
      q.z = (a[1][2] + a[2][1] ) / s;
    } else {
      f32 s = 2.0f * sqrtf( 1.0f + a[2][2] - a[0][0] - a[1][1] );
      q.w = (a[1][0] - a[0][1] ) / s;
      q.x = (a[0][2] + a[2][0] ) / s;
      q.y = (a[1][2] + a[2][1] ) / s;
      q.z = 0.25f * s;
    }
  }
}* /
		T trace = m._00 + m._11 + m._22;
		if( trace > 0 ) {
			T s = (T)0.5 / sqrt(trace+ (T)1.0);
			w = (T)0.25 / s;
			x = ( m._21 - m._12 ) * s;
			y = ( m._02 - m._20 ) * s;
			z = ( m._10 - m._01 ) * s;
		} else {
			if ( m._00 > m._11 && m._00 > m._22 ) {
				T s = (T)0.5 / sqrt( (T)1.0 + m._00 - m._11 - m._22);
				w = (m._21 - m._12 ) * s;
				x = (T)0.25 / s;
				y = (m._01 + m._10 ) * s;
				z = (m._02 + m._20 ) * s;
			} else if (m._11 > m._22) {
				T s = (T)0.5 / sqrt( (T)1.0 + m._11 - m._00 - m._22);
				w = (m._02 - m._20 ) * s;
				x = (m._01 + m._10 ) * s;
				y = (T)0.25 / s;
				z = (m._12 + m._21 ) * s;
			} else {
				T s = (T)0.5 / sqrt( (T)1.0 + m._22 - m._00 - m._11 );
				w = (m._10 - m._01 ) * s;
				x = (m._02 + m._20 ) * s;
				y = (m._12 + m._21 ) * s;
				z = (T)0.25 / s;
			}
		}
	}
	operator mat3<T> () const { 
		mat3<T> m;
		T x2 = x+x,		y2 = y+y,		z2 = z+z;
		T xx = x2*x,	xy = x2*y,		xz = x2*z,	xw = x2*w;
		T yy = y2*y,	yz = y2*z,		yw = y2*w;
		T zw = z2*w,	zz = z2*z;


		m._00 = 1.0f - ( yy + zz );
		m._01 = ( xy - zw );
		m._02 = ( xz + yw );

		m._10 = ( xy +  zw );
		m._11 = 1.0f - ( xx + zz );
		m._12 = ( yz - xw );

		m._20 = ( xz - yw );
		m._21 = ( yz + xw );
		m._22 = 1.0f - ( xx + yy );

		return m; 
	}	
	*/
#endif //GEM_CONV_QUAT_MAT3