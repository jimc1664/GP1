#ifndef GEM_DIS_DRAWLIST_H
#define GEM_DIS_DRAWLIST_H

#include "../Basic.h"
#include "../Org/msgQ.h"
#include "../Math/Matrix2x3.h"

namespace Gem {  

	namespace Scene2 { class Sprite; class Texture; }
	
namespace Dis {

class DisMain;
class DrawMsg : public msgQNode<DrawMsg> {
friend class msgQ<DrawMsg>;
	virtual sizet proc(ptr cntx) = 0;
};
Template1 class DrawMsg_T : public DrawMsg {
	sizet proc( ptr cntx ) override 
//#ifdef DIS_INTERNAL	
	{
		static_cast<T*>(this)->proc( *(DisMain*) cntx );
		return sizeof(T);
	}
//#else
	//;
//#endif

};


class DrawSprite : public DrawMsg_T<DrawSprite>  {
friend class msgQ<DrawMsg>;
	DrawSprite( Scene2::Sprite *spr );
public:
	void proc(DisMain &dm);
	mat2x3f Trans;
	Scene2::Texture &Tex;
	// Scene2::Sprite &Spr;
};

//class RenderTarget;
class DrawList : public msgQ<DrawMsg> {
public:
	//DrawList(  );

	//void setOutput( RenderTarget& rt );   //todo thread checks..
#ifdef DIS_INTERNAL
	void render(DisMain &dm);
	//void setOutput( DisMain &dm, RenderTarget& rt );
#endif //DIS_INTERNAL
protected:

private:
	
};

class BufferedDrawList {
public:
	BufferedDrawList();
	class DrawListRef {
	public:
		~DrawListRef() {
			Base.doneUpdate(Dl);
		}
		operator DrawList&() {return Dl; }
	private: friend class BufferedDrawList;
		DrawListRef(DrawList& dl, BufferedDrawList &b ) : Dl(dl), Base(b) {}
		DrawList& Dl; BufferedDrawList &Base;
	};


	DrawListRef forUpdate();

#ifdef DIS_INTERNAL
	void render(DisMain &dm);
#endif //DIS_INTERNAL

private: friend class DrawListRef;
	void doneUpdate( DrawList& dl );
	void swap(u8 a, u8 b);
	DrawList  * volatile ToRender, *volatile Rendering;



	DrawList Dl[3];
	int Ui;

	/*
	__declspec(align(4)) struct Offset {
		union {
			u8 B[3]; 
			volatile u32 I; 
		};
	};
	Offset Inds;
	enum {
		Update = 0,
		Render,
		Mid,
	};*/

};

}	}

#endif //GEM_DIS_DRAWLIST_H