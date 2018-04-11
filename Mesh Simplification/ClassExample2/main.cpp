#include <iostream>
#include "display.h"
#include "inputManager.h"

Display display(DISPLAY_WIDTH, DISPLAY_HEIGHT, "OpenGL");	
Scene scn(glm::vec3(0.0f, 0.0f, -15.0f), CAM_ANGLE, relation, NEAR, FAR);

int main(int argc, char** argv)
{
	initCallbacks(display);
	
	// Box OBJ
	// Start faces = 12
	
	// Remove/add comments to add/remove Box object to the scene
	//scn.addShape("./res/objs/testboxNoUV.obj",0);
	//scn.addShape("./res/objs/testboxNoUV.obj", 1);


	//Monkey OBJ
	// Start faces = 4k

	// Remove/add comments to add/remove Monkey object to the scene
	scn.addShape("./res/objs/monkey3.obj",0);		// adding the original Mesh to the scene
	scn.addShape("./res/objs/monkey3.obj",1);		// adding the simplified Mesh to the scene


	scn.addShader("./res/shaders/basicShader");
	scn.addShader("./res/shaders/pickingShader");
		
	while(!display.toClose())
	{
		display.Clear(0.7f, 0.7f, 0.7f, 1.0f);
		scn.draw(0,0,true);
		display.SwapBuffers();
		display.pullEvent();
	}

	return 0;
}
