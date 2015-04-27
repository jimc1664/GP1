#ifndef GEM_DIS_CAMERA_H
#define GEM_DIS_CAMERA_H

#include "../Basic.h"
#include "../Org/dList.h"

namespace Gem {  namespace Dis {

class RenderTarget; 

class Camera :  public dListNode<Camera> {
public:
	Camera(  );

	void setOutput( RenderTarget* rt );
#ifdef DIS_INTERNAL
	void render(DisMain &dm);
	void setOutput( DisMain &dm, RenderTarget& rt );
#endif //DIS_INTERNAL
protected:

private:
	//virtual void render() = 0;

};

}	}

#endif //GEM_DIS_CAMERA_H