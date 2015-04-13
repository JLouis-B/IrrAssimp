#include <irrlicht.h>
#include "IrrAssimp/IrrAssimp.h"

using namespace irr;

using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;


#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif


/*
This is the main method. We can now use main() on every platform.
*/
int main()
{
	IrrlichtDevice *device =
		createDevice( video::EDT_SOFTWARE, dimension2d<u32>(640, 480), 16,
			false, false, false, 0);

	if (!device)
		return 1;

	device->setWindowCaption(L"Hello World! - IrrAssimp Demo");


	IVideoDriver* driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* guienv = device->getGUIEnvironment();


	guienv->addStaticText(L"Hello World! This is the Irrlicht Software renderer!",
		rect<s32>(10,10,260,22), true);


	IrrAssimp* assimp = new IrrAssimp(smgr);
    IAnimatedMesh* mesh = assimp->getMesh("Media/ninja.b3d");


	//IAnimatedMesh* mesh = smgr->getMesh("../../media/sydney.md2");


	if (!mesh)
	{
		device->drop();
		return 1;
	}

	IMeshSceneNode* node = smgr->addMeshSceneNode( mesh );

	if (node)
	{
		node->setMaterialFlag(EMF_LIGHTING, false);
		//node->setMD2Animation(scene::EMAT_STAND);
		//node->setMaterialTexture( 0, driver->getTexture("../../media/dwarf.jpg") );
	}

	smgr->addCameraSceneNodeFPS(0);

	while(device->run())
	{
		driver->beginScene(true, true, SColor(255,100,101,140));

		smgr->drawAll();
		guienv->drawAll();

		driver->endScene();
	}

	device->drop();

	return 0;
}

/*
That's it. Compile and run.
**/
