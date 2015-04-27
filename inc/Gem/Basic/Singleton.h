#ifndef GEM_BASIC_SINGLETON_H
#define GEM_BASIC_SINGLETON_H

#include "..\Basic.h"

namespace Gem {

template<class Obj > struct Singleton {
public:
	struct ScopeHandle {
		ScopeHandle(){ Singleton<Obj>::construct(); }
		~ScopeHandle(){ Singleton<Obj>::destruct(); }
		void shutdown() { Singleton<Obj>::shutdown(); }
	};
	static Obj& Ptr;

	static void construct() { Ptr._construct(); }
	static void destruct()	{ Ptr._destruct(); }
	static void shutdown()	{ Ptr._shutdown(); }

protected:
	void _construct() { new( &Ptr ) Obj(); }
	void _destruct() { Ptr.~Obj(); }
	void _shutdown() { }
};

#define Alloc_Singleton( Obj )	\
u8 _##Obj##_Space[sizeof(Obj)]; \
Obj& Singleton<Obj>::Ptr = *(Obj*)&_##Obj##_Space[0]

}


#endif //GEM_BASIC_SINGLETON_H