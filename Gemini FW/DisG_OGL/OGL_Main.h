#ifndef GEM_DIS_OGL_MAIN_H
#define GEM_DIS_OGL_MAIN_H

#include "../Dis/Dis_Main.h"
//#include "Gem/Dis/Texture.h"
#include "Gem/ConCur/Thread.h"
#include "Gem/ConCur/TaskQ.h"
#include "Gem/Org/sList.h"

namespace Gem {  

namespace Dis { 
	class  MainTarget;
}
	
namespace Dis { 

Template1 struct autoDXPtr {
	autoDXPtr() : Ptr(0) {}
	~autoDXPtr() { if( Ptr ) release();  }
	operator T () { return Ptr; }

	T operator ->() { return Ptr; }
	T* operator &() { return &Ptr; }

	void release() { Ptr->Release(); Ptr = 0; }
private:
	T Ptr;
};

class OGL_Task_Base;

class DisMain : public Dis::Dis_Main, public Singleton<DisMain>, public TaskHost<DisMain> {
public:

private:
	Thread  MainThread;
friend class Dis::Dis_Main;
	static void construct() { new( &Singleton<DisMain>::Ptr ) DisMain(); }
	static void destruct() { Singleton<DisMain>::Ptr.~DisMain(); }


	DisMain();
	~DisMain();
	
	void shutdown();
	void init();
	void deinit();
	void mainLoop();

	volatile bool Shutdown;

	//autoDXPtr<ID3D11Device*>        Device;
//	autoDXPtr<IDXGIFactory*>		DXFactory;


	MainTarget * Primary;
	friend class MainTarget;
	sList<Dis::MainTarget>	Outputs;

	HGLRC Cntx;
	//Texture BlankTex;
};

} }

#endif //GEM_DIS_OGL_MAIN_H