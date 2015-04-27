#ifndef GEM_CONCUR_CRITSEC_H
#define GEM_CONCUR_CRITSEC_H

#include "../Basic.h"

namespace Gem { 
class CritSec : public Uncopyable {
public:
	struct Handle  : public Uncopyable {
		Handle( CritSec &cs ) : CS(cs) { CS.lock(); }
		~Handle() { CS.unlock(); }
	private:
		CritSec &CS;
	};

	CritSec();
	~CritSec();
private: friend Handle;

	OS_DatA(CRITICAL_SECTION, 8 * 3, Hndl);

	void lock();
	void unlock();
};

template< class IF, class LockT > class lockedIF_T : public LockT::Handle {
public:
	lockedIF_T( LockT &lock, IF &i ) : Handle(lock), If(i) {}
	operator IF* () { return &If; }
	//operator IF& () { return If; }
	IF* operator -> () { return &If; }
private:
	IF &If;
};
template<typename IF> struct lockedIF : public lockedIF_T<IF,CritSec> { ConstructorWrap( lockedIF, lockedIF_T ); };


template<class Base > struct lockableIF : protected Base {
	lockableIF() {}
	struct Hndl : public lockedIF<Base> {
		Hndl( lockableIF<Base> &a ) : lockedIF(a.Lock, a.base() ) {}
	};
private: 
	Base& base() { return *static_cast<Base*>(this); }
	CritSec Lock;
};

}	

#endif //GEM_CONCUR_CRITSEC_H