#ifndef GEM_DIS_MAIN_H
#define GEM_DIS_MAIN_H

#include "Gem/Basic/Singleton.h"

namespace Gem { namespace Dis {

class Dis_Main : public Singleton<Dis_Main> {
private: friend struct Singleton<Dis_Main>;
	void _construct();
	void _destruct();
	void _shutdown();
};

} }

#endif //GEM_INTERNAL_MAIN_H