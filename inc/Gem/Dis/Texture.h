#ifndef GEM_DIS_TEXTURE_H
#define GEM_DIS_TEXTURE_H

#include "../Basic.h"
//#include "../Org/dList.h"
#include "../String/CStr.h"

namespace Gem {  namespace Dis {

class DisMain;

class Texture  { //:  public dListNode<Texture> {
public:
	Texture(  );

	static Texture* fromFile(const CStr &s);  //temporary


	void fromFile(DisMain &dm, const CStr &s);
	void gen(DisMain &dm, ptr imageBuffer, u32 width, u32 height, u32 intFormat, u32 format );


	void apply(DisMain &dm);
	//void render(DisMain &dm);
	//void setOutput( DisMain &dm, RenderTarget& rt );

protected:

private:
	Dis_Dat( GLuint, u32, TexID  );

};

}	}

#endif //GEM_DIS_TEXTURE_H