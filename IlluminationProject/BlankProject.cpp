#include "../NCLGL/window.h"
#include "Renderer.h"

int main() {
	Window w("Coursework!", 1280, 720, false); //This is all boring win32 window creation stuff!
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w); //This handles all the boring OGL 3.2 initialisation stuff, and sets up our tutorial!
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	bool dualCamera;

	int choosePost;

	dualCamera = false;

	choosePost = 0;

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) {
			dualCamera = false;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_2)) {
			dualCamera = true;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_3)) { // No post-process
			choosePost = 0;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_4)) { // Blur post-process
			choosePost = 1;
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_5)) { // Sobel post-process
			choosePost = 2;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_9)) { // Start camera demo movement
			renderer.startRail();
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_0)) { // End camera demo movement
			renderer.breakRail();
		}

		if (!dualCamera) {
			renderer.setOneCamera();

			renderer.firstCamera();
			renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());

			renderer.firstCameraBuffer();
			renderer.RenderScene();

			if (choosePost == 1) {
				renderer.useBlur();
				renderer.drawPostProcess();
			}
			if (choosePost == 2) {
				renderer.useSobel();
				renderer.drawPostProcess();
			}

			renderer.DrawToScreen();

			renderer.SwapBuffers();
		}
		else {
			renderer.setTwoCameras();

			renderer.firstCamera();
			renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());

			renderer.firstCameraBuffer();
			renderer.RenderScene();

			renderer.secondCamera();
			renderer.UpdateScene_2();

			renderer.secondCameraBuffer();
			renderer.RenderScene();

			if (choosePost == 1) {
				renderer.useBlur();
				renderer.drawPostProcess();
			}
			if (choosePost == 2) {
				renderer.useSobel();
				renderer.drawPostProcess();
			}

			renderer.DrawToScreen();

			renderer.SwapBuffers();
		}


		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}

	return 0;
}










//void Tutorial11(Renderer& renderer) {
//	std::vector <Vector3 > verts;
//
//	for (int i = 0; i < 100; ++i) {
//		float x = (float)(rand() % 100 - 50);
//		float y = (float)(rand() % 100 - 50);
//		float z = (float)(rand() % 100 - 50);
//		
//		verts.push_back(Vector3(x, y, z));
//	}
//
//	Mesh* pointSprites = new Mesh();
//	pointSprites->SetVertexPositions(verts);
//	pointSprites->SetPrimitiveType(GeometryPrimitive::Points);
//	pointSprites->UploadToGPU();
//
//	Matrix4 modelMat = Matrix4::Translation(Vector3(0, 0, -30));
//
//	Shader* newShader = new Shader("RasterisationVert.glsl",
//		"RasterisationFrag.glsl",
//		"pointGeom.glsl");
//
//	RenderObject* object = new RenderObject(pointSprites, modelMat);
//
//	object->SetBaseTexture( OGLTexture::RGBATextureFromFilename("nyan.png"));
//
//	object->SetShader(newShader);
//
//	renderer.AddRenderObject(object);
//}