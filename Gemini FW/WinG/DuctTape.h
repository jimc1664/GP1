#ifndef GEM_WING_DUCTTAPE_H
#define GEM_WING_DUCTTAPE_H

namespace Gem {		namespace DuctTape {

#ifdef DEBUG

#ifdef _WINDOWS_
	void activateForModule( HMODULE mod = NULL );
	void deactivateForModule( HMODULE mod = NULL );
#endif
	void activateForModule( const char *name );  //todo char?
	void deactivateForModule( const char *name );
	void printAll();
	void shutdown(); //deactivateForModule + printAll
#else
	void activateForModule( ptr mod = NULL ) {}
	void deactivateForModule( ptr mod = NULL ) {}
	void printAll() {}
	void shutdown() {}
#endif //DEBUG
}	}


#endif //GEM_WING_DUCTTAPE_H