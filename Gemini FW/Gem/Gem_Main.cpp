#include "stdafx.h"

#include "Gem/Main.h"
#include "Main_Internal.h"

#include "../Dis/Dis_Main.h"
#include "../JUI/JUI_Main.h"

#include <stdio.h>

/*
int main() {
	 
	struct Acsr : Main_Hlpr {
		Acsr() {
			Main_Hlpr::construct();
			Main_Hlpr::loop();
			Main_Hlpr::destruct();
		}
	} a;
}			*/		

Gem_Main::Gem_Main() {
	Dis::Dis_Main::construct();
	JUI::JUI_Main::construct();
	Main_Hlpr::construct();
}
Gem_Main::~Gem_Main() {
	Main_Hlpr::destruct();
	JUI::JUI_Main::Ptr.destruct();
	Dis::Dis_Main::Ptr.destruct();
}

void Gem_Main::_shutdown() {
	Main_Hlpr::shutdown();
	Dis::Dis_Main::shutdown();
	JUI::JUI_Main::shutdown();
}

Alloc_Singleton(Gem_Main);


#ifdef CONFIG_OS_WIN
#pragma comment( lib, LibName( "WinG" ) ) 
#else 
#error "CONFIG_OS_XXX not defined.... (You ass monkey)"
#endif 

#ifdef CONFIG_DIS_DX10
#pragma comment( lib, LibName( "DisG_Dx10" ) ) 
#elif defined(CONFIG_DIS_DX11)
#pragma comment( lib, LibName( "DisG_Dx11" ) ) 
#elif defined(CONFIG_DIS_OGL)
#pragma comment( lib, LibName( "DisG_OGL" ) ) 
#else 
#error "CONFIG_DIS_XXX not defined.... (You ass monkey)"
#endif 

namespace Gem { 
namespace _Internal { 

void _INT_forceLink_OS();
void _INT_forceLink_Dis();
void _INT_forceLink_Gemini() { 
	_INT_forceLink_OS();
	_INT_forceLink_Dis(); 
}  

}
}
