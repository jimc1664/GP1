#ifndef GEM_ORG_ARY_H
#define GEM_ORG_ARY_H

#include "../Basic.h"

namespace Gem {
	
namespace Ctor {
	struct Standard { template<class Obj> struct Sub { 
					void construct( Obj &at )									{ new (&at) Obj(); }
		Template1	void construct( Obj &at, T &&o )								{ new (&at) Obj(o); }
					void mulConstruct( Obj* const &at, u32 c )					{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj();		if(!c) break; } }
//		Template1	void mulConstruct( Obj* const &at, u32 c, T &&o )				{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj(o);	if(!c) break; } }
		Template1	void mulConstruct( Obj* const &at, u32 c, T *const &o )		{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj(o[c]);	if(!c) break; } }
					void destruct( Obj &at )										{ at.~Obj(); }
					void mulDestruct( Obj* const &at, u32 c )						{ Assume(c!= 0); for(;;) { --c; at[c].~Obj();			if(!c) break; } }

		Template2	void mulConstruct( Obj* const &at, u32 c, T *const &o, const T2 &cpySpcf )		
		{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj(cpySpcf.copy(o[c]));	if(!c) break; } }
	};  Template1 static Sub<T> getCnType();  };
	struct Simple { template<class Obj> struct Sub {
					void construct( Obj &at )									{}
		Template1	void construct( Obj &at, T &&o )								{ new (&at) Obj(o); }
					void mulConstruct( Obj* const &at, u32 c )					{ }
//		Template1	void mulConstruct( Obj* const &at, u32 c, T &&o )				{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj(o);	if(!c) break; } }
		Template1	void mulConstruct( Obj* const &at, u32 c, T *const &o )		{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj(o[c]);	if(!c) break; } }
					void destruct( Obj &at )										{}
					void mulDestruct( Obj* const &at, u32 c )					{}

		Template2	void mulConstruct( Obj* const &at, u32 c, T *const &o, const T2 &cpySpcf )		
		{ Assume(c!= 0); for(;;) { --c; new (&at[c]) Obj(cpySpcf.copy(o[c]));	if(!c) break; } }
	}; Template1 static Sub<T> getCnType();  };

	struct SimpleZeroed { template<class Obj> struct Sub  : public Simple::Sub<Obj> {
					void construct( Obj &at )					{ memset( &at,0,sizeof(at) ); }
		Template1	void construct( Obj &at, T &&o )								{ new (&at) Obj(o); }
					void mulConstruct( Obj* const &at, u32 c )	{ memset( at,0,sizeof(at)*c );}
	}; Template1 static Sub<T> getCnType();  };
}

template<class Obj, class CnType = Ctor::Standard > class ary  {
#define DEFAULT_EXTENDBY 32
	typedef decltype( CnType::getCnType<Obj>() ) CnTyp; 
public:
	
	CnTyp Cn;
//	typedef typename CnType::Sub<Obj> Cn;  //todo --  why dafuq this ain't working no more
	
	ary() : Allocated(0), Count(0), Data(0)			{}
	ary( const u32 &a )								: Allocated(a), Count(0), Data( alloc(a) )	{}
	ary( const u32 &a, const u32 &c )				: Allocated(a), Count(c), Data( alloc(a) )	{ Cn.mulConstruct(Data,c); }
	ary( const u32 &a, const u32 &c, const Obj &o )	: Allocated(a), Count(c), Data( alloc(a) )	{ Cn.mulConstruct(Data,c,o); }

	void reserve( const u32 &amnt ) { Assume(Data == 0); Data = alloc(Allocated=amnt); }
	Template1 ary( const Steal_Struct<T> &s )		: Allocated( s->Allocated ), Count(s->Count), Data( s->Data ) {  s->Allocated = s->Count = 0; s->Data = 0; }
	Template1 ary( const Clone_Struct<T> &s )		: Allocated( s->Allocated ), Count(s->Count), Data( alloc(s->Count) ) { Cn.mulConstruct(Data,Count,s->Data,s); }
	~ary()											{ if(Allocated){ if(Count) Cn.mulDestruct(Data,Count);  free(Data);} }

//			operator Obj* &				()				{ return Data; }
			operator const Obj* const &	()		  const { return Data; }
	Obj&			operator[](const u32 &ind)			{ Assume(ind<Count); return Data[ind]; }
	const Obj&	operator[](const u32 &ind)	  const	{ Assume(ind<Count); return Data[ind]; }
	Obj&			operator[](const s32 &ind)			{ return (*this)[(u32)ind]; }
	const Obj&	operator[](const s32 &ind)	  const	{ return (*this)[(u32)ind]; }
	const u32&	count()					  const { return Count; }
	u32			sizeOf()					  const { return Count*sizeof(Obj); }

	Template1 Obj&	add( T &&a, const u32 &extendBy = DEFAULT_EXTENDBY ) {
		Assume( !Data || (((sizet)&a < (sizet)Data) || ((sizet)&a > (sizet)&Data[Allocated-1]))); //copying stored data could be errorous as a following realloc could therefore invalidate 'a'
		extendIfNeeded( Count+1, extendBy ); 
		Cn.construct( Data[Count], a );
		return Data[Count++];
	}
	Obj&	add() {
		extendIfNeeded( Count+1 ); 
		Cn.construct( Data[Count] );
		return Data[Count++];
	}
	void	push( const Obj &a ) { add(a); }
	u32		pushI( const Obj &a ) { u32 r = Count; add(a); return r;  }
	u32		pushI() { u32 r = Count; add(); return r;  }

	Obj pop() {
		Assume( Count );
		Obj ret = Data[Count-1];
		decCount();
		return ret;
	}
	void addMul( const u32 &amnt, const Obj a[], const u32 &extendBy ) { 
		Assume( amnt <= extendBy );
		extendIfNeeded( Count+amnt, extendBy ); 
		Cn.mulConstruct( &Data[Count], amnt, a );
		Count += amnt;
	}
	void addMul( const u32 &amnt, const Obj a[] ) { 
		extendIfNeeded( Count+amnt, amnt ); 
		Cn.mulConstruct( &Data[Count], amnt, a );
		Count += amnt;
	}
	void addMul( const u32 &amnt )	{ 
		extendIfNeeded( Count+amnt, amnt ); 
		Cn.mulConstruct( &Data[Count], amnt );
		Count += amnt;
	}
	void enSpace( const u32 &amnt ) { extendIfNeeded( Count+amnt, amnt ); }

	void decCount() { Assume( Count != 0 ); Cn.destruct( Data[--Count]); }
	void decCountTo( const u32 &c ) {
		Assume( c < Count );
		Cn.mulDestruct( &Data[c], Count-c);
		Count = c;
	}	

	void sfDecCountTo( const u32 &c ) { if( c < Count ) decCountTo(c); }  //safe version
	void clear() { sfDecCountTo(0); }


	void erase( const u32 &i ) {
		Assume( i < Count );
		Count--;
		if( i != Count ) {
			memcpy( &Data[i], &Data[Count], sizeof( Obj ) );
		}
	}
	class iter {
	public:
		iter( ary &a, const u32 &i ) : Ind(i), Array(a) {}
		Obj& operator++(int)				{ return Array[Ind++]; }
		operator bool()					{ return Ind < Array.count(); }
		operator Obj&()					{ return Array[Ind]; }
		Obj* operator->()				{ return &Array[Ind]; }
		Obj& operator*()					{ return Array[Ind]; }
		const u32& ind()			const	{ return Ind; }
	protected:
		u32 Ind;
		ary &Array;
	};

	class rvsIter : public iter {
	public:
		rvsIter( ary &a, const u32 &i ) : iter(a, i ) {}
		bool operator--(int)				{ return Ind-- != 0; }
	};

	iter start()		{ return iter(*this, 0 ); }
	rvsIter end()		{ return rvsIter(*this, Count ); }

	//void Iterate( void (Obj::*fnc)(void) )						{ for( iter it(first()); it; it++ ) ((*it).*fnc)(); }
//	Template void Iterate( void (Obj::*fnc)(T&), T &p1 )			{ for( iter it(first()); it; it++ ) ((*it).*fnc)(p1); }

protected:
	static	Obj*	alloc( const u32 &a ) { return static_cast<Obj*>(malloc(a*sizeof(Obj))); }
	void	extendIfNeeded( const u32 &nCount, const u32 &extendBy = DEFAULT_EXTENDBY ) { 		
		if( nCount > Allocated ) {
			Data = static_cast<Obj*>(realloc(Data,(Allocated+=(Allocated+extendBy))*sizeof(Obj)));
		}
	}
	u32 Allocated;
	u32 Count;
	Obj* Data;
	//setUncopyable(ary);
};


Template1 struct StrideIter {
	StrideIter() : Cur(0), Remaining(0), Stride(0) {}
	StrideIter( 	T* start, const u32 & amnt, const u32 &stride ) : Cur(start), Remaining(amnt), Stride(stride) {}
	operator bool() { return Remaining != 0; }
	operator T&() { return *Cur; }
	T& operator++(int) { T& ret = *Cur; Remaining--; Cur = (T*)(((u8*)Cur)+Stride); return ret; }
	const u32& remain() const { return Remaining; }
private:	
	T* Cur;
	u32 Remaining;
	u32 Stride;
};
#define GetStrideIter( Array, Member ) (StrideIter< decltype( Array[0].Member ) > ( &Array[0].Member, Array.count(), sizeof( decltype( Array[0] ) ) ))

}

#endif //GEM_ORG_ARY_H