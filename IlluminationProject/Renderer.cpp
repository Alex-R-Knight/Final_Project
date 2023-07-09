#include "Renderer.h"
#include "../nclgl/CubeRobot.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include <algorithm > //For std::sort ...
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
const int LIGHT_NUM = 25;
const int POST_PASSES = 10;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	//cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	quad = Mesh::GenerateQuad();
	quad_L = Mesh::GenerateQuadHalf();
	quad_R = Mesh::GenerateQuadHalf_2();
	tree = Mesh::LoadFromMeshFile("Tree.msh");

	animMesh = Mesh::LoadFromMeshFile("skeleton.msh");

	heightMap = new HeightMap(TEXTUREDIR"heightmap_1_3.jpg");

	earthTex = SOIL_load_OGL_texture(
		TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	earthBump = SOIL_load_OGL_texture(
		TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	grassTex = SOIL_load_OGL_texture(
		TEXTUREDIR"mud_diffuse.png", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	grassBump = SOIL_load_OGL_texture(
		TEXTUREDIR"mud_bump.png", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	nullBump = SOIL_load_OGL_texture(
		TEXTUREDIR"stubdot3.tga", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	reflectiveTex = SOIL_load_OGL_texture(
		TEXTUREDIR"fullreflect.tga", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	unreflectiveTex = SOIL_load_OGL_texture(
		TEXTUREDIR"noreflect.tga", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"nightsky2_left.png", TEXTUREDIR"nightsky2_right.png",
		TEXTUREDIR"nightsky2_up.png", TEXTUREDIR"nightsky2_down.png",
		TEXTUREDIR"nightsky2_front.png", TEXTUREDIR"nightsky2_back.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	glassTex = SOIL_load_OGL_texture(
		TEXTUREDIR"stainedglasstreetex.png", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	//glassTex = SOIL_load_OGL_texture(
	//	TEXTUREDIR"stainedglass.tga", SOIL_LOAD_AUTO,
	//	SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	if (!earthTex || !earthBump || !grassTex || !grassBump || !nullBump || !cubeMap || !glassTex) {
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(grassTex, true);
	SetTextureRepeating(grassBump, true);
	SetTextureRepeating(nullBump, true);

	SetTextureRepeating(reflectiveTex, true);
	SetTextureRepeating(unreflectiveTex, true);

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	//std::cout << "X = " << heightmapSize.x << "\n";
	//std::cout << "Y = " << heightmapSize.y << "\n";
	//std::cout << "Z = " << heightmapSize.z << "\n";

	//camera = new Camera(10.0f, 0.0f, heightmapSize * Vector3(0.5f, 1.0f, 0.9f));
	camera = new Camera(0.0f, 0.0f, Vector3(0.0f, 0.0f, 0.0f));

	startPos = heightmapSize * Vector3(0.5f, 1.0f, 0.9f);

	onePos = heightmapSize * Vector3(0.5f, 2.0f, 0.72f);

	twoPos = heightmapSize * Vector3(0.4f, 1.7f, 0.55f);

	threePos = heightmapSize * Vector3(0.5f, 2.0f, 0.25f);

	camera_2 = new Camera(-20.0f, 135.0f, heightmapSize * Vector3(0.75f, 3.0f, 0.25f));

	activeCamera = camera;

	pointLights = new Light[LIGHT_NUM];

	heightShader = new Shader("BumpVertex.glsl", // reused!
		"bufferIGNFragment_HeightMap.glsl");

	sceneShader = new Shader("BumpVertex.glsl", // reused!
		"bufferIGNFragment.glsl");

	pointlightShader = new Shader("pointlightvert.glsl",
		"pointlightfrag.glsl");

	combineShader = new Shader("combinevert.glsl",
		"combinefrag.glsl");

	skyboxShader = new Shader("skyboxVertex.glsl",
		"skyboxFragment.glsl");

	alphaShader = new Shader("SceneVertex.glsl",
		"SceneFragment.glsl");

	animatedshader = new Shader("SkinningVertex_Deferred.glsl",
		"bufferIGNFragment.glsl");

	meshshader = new Shader("BumpVertex.glsl",
		"bufferIGNFragment.glsl");

	endshader = new Shader("TexturedVertex.glsl",
		"texturedfragment.glsl");

	blurShader = new Shader("TexturedVertex.glsl",
		"processfrag.glsl");

	sobelShader = new Shader("TexturedVertex.glsl",
		"processfrag_sobel_edge.glsl");

	marchShader = new Shader("TexturedVertex.glsl",
		"raymarchfrag.glsl");

	if (!sceneShader->LoadSuccess()			|| !pointlightShader->LoadSuccess()
		|| !combineShader->LoadSuccess()	|| !skyboxShader->LoadSuccess()
		|| !alphaShader->LoadSuccess()		|| !heightShader->LoadSuccess()
		|| !meshshader->LoadSuccess()		|| !endshader->LoadSuccess()
		|| !blurShader->LoadSuccess()		|| !sobelShader->LoadSuccess()
		|| !animatedshader->LoadSuccess()	|| !marchShader->LoadSuccess()	) {
		return;
	}

	root = new SceneNode();

	treeMaterial = new MeshMaterial("Tree.mat");

	for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = treeMaterial->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	//for (int i = 0; i < 100; i++) {
	//	SceneNode* treenode = new SceneNode();
	//	treenode->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	//	treenode->SetTransform(Matrix4::Translation(Vector3((rand() % (int)(0.7 * heightmapSize.x)) + (0.15 * heightmapSize.x), 50.0f, (rand() % (int)(0.7 * heightmapSize.z)) + (0.15 * heightmapSize.z))));
	//	treenode->SetModelScale(Vector3(25.0f, 35.0f, 25.0f));
	//	treenode->SetBoundingRadius(1000.0f);
	//	treenode->SetMesh(tree);
	//
	//	for (int i = 0; i < tree->GetSubMeshCount(); ++i) {
	//		treenode->SetTexture(matTextures[i]);
	//	}
	//
	//	root->AddChild(treenode);
	//}

	for (int i = 0; i < LIGHT_NUM; i++) {
		Vector3 newlocation = Vector3((rand() % (int)(0.7 * heightmapSize.x)) + (0.15 * heightmapSize.x), 0.0f, (rand() % (int)(0.7 * heightmapSize.z)) + (0.15 * heightmapSize.z));

		Light& l = pointLights[i];
		l.SetPosition(Vector3(newlocation.x, newlocation.y + 350, newlocation.z));

		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 200000));
		l.SetRadius(1500.0f + (rand() % 300));


		//SceneNode* treepos = new SceneNode();
		//treepos->SetTransform(Matrix4::Translation(newlocation));
		//
		//SceneNode* treenode = new SceneNode();
		//treenode->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.9f));
		//treenode->SetModelScale(Vector3(40.0f, 55.0f, 40.0f));
		//treenode->SetBoundingRadius(1000.0f);
		//treenode->SetMesh(tree);
		//treenode->SetTexture(glassTex);
		//
		//treepos->AddChild(treenode);
		//
		//root->AddChild(treepos);
	}

	//SceneNode* statue = new SceneNode();
	//statue->SetTransform(Matrix4::Translation(Vector3(0.5 * heightmapSize.x, 50.0f, 0.5 * heightmapSize.z)));
	//statue->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	//statue->SetModelScale(Vector3(1000.0f, 1000.0f, 1000.0f));
	//statue->SetBoundingRadius(2000.0f);
	//statue->SetMesh(Mesh::LoadFromMeshFile("Role_T.msh"));
	//statue->SetTexture(earthTex);
	//
	//root->AddChild(statue);

	
	// Testing cubes

	SceneNode* testFloor = new SceneNode();
	testFloor->SetTransform(Matrix4::Translation(Vector3(0.0f, -2.5f, 0.0f)));
	testFloor->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testFloor->SetModelScale(Vector3(100.0f, 1.0f, 100.0f));
	testFloor->SetBoundingRadius(2000.0f);
	testFloor->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testFloor->SetTexture(earthTex);
	testFloor->SetReflect(reflectiveTex);

	root->AddChild(testFloor);

	SceneNode* testCube = new SceneNode();
	testCube->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -25.0f)));
	testCube->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube->SetBoundingRadius(2000.0f);
	testCube->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube->SetTexture(earthTex);
	testCube->SetReflect(unreflectiveTex);
	
	root->AddChild(testCube);
	
	SceneNode* testCube2 = new SceneNode();
	testCube2->SetTransform(Matrix4::Translation(Vector3(0.0f, 5.0f, -30.0f)));
	testCube2->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube2->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube2->SetBoundingRadius(2000.0f);
	testCube2->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube2->SetTexture(earthTex);
	testCube2->SetReflect(unreflectiveTex);
	
	root->AddChild(testCube2);
	
	SceneNode* testCube3 = new SceneNode();
	testCube3->SetTransform(Matrix4::Translation(Vector3(5.0f, 5.0f, -30.0f)));
	testCube3->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube3->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube3->SetBoundingRadius(2000.0f);
	testCube3->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube3->SetTexture(grassTex);
	testCube3->SetReflect(unreflectiveTex);
	
	root->AddChild(testCube3);

	SceneNode* testCube4 = new SceneNode();
	testCube4->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -30.0f)));
	testCube4->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube4->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube4->SetBoundingRadius(2000.0f);
	testCube4->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube4->SetTexture(earthTex);
	testCube4->SetReflect(unreflectiveTex);

	root->AddChild(testCube4);

	SceneNode* testCube5 = new SceneNode();
	testCube5->SetTransform(Matrix4::Translation(Vector3(5.0f, 0.0f, -30.0f)));
	testCube5->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube5->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube5->SetBoundingRadius(2000.0f);
	testCube5->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube5->SetTexture(grassTex);
	testCube5->SetReflect(unreflectiveTex);

	root->AddChild(testCube5);

	// animation test

	//for (int i = 0; i < 10; i++) {
	//	SceneNode* animationfather = new SceneNode();
	//
	//	AnimatedNode* skeleton = new AnimatedNode();
	//
	//	skeleton->SetTransform(Matrix4::Translation(Vector3((rand() % (int)(0.7 * heightmapSize.x)) + (0.15 * heightmapSize.x), 50.0f, (rand() % (int)(0.7 * heightmapSize.z)) + (0.15 * heightmapSize.z))));
	//	skeleton->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.8f));
	//	skeleton->SetModelScale(Vector3(600.0f, 600.0f, 600.0f));
	//	skeleton->SetBoundingRadius(600.0f);
	//	skeleton->SetMesh(animMesh);
	//	skeleton->setAnim("skeleton.anm");
	//	skeleton->setMat("skeleton.mat");
	//
	//	animationfather->AddChild(skeleton);
	//
	//	root->AddChild(animationfather);
	//}

	glGenFramebuffers(1, &alphaFBO);
	glGenFramebuffers(1, &alphaFBO_2);
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);
	glGenFramebuffers(1, &processFBO);

	glGenFramebuffers(1, &UVFBO);

	GLenum buffers[5] = {
		GL_COLOR_ATTACHMENT0 ,
		GL_COLOR_ATTACHMENT1 ,
		GL_COLOR_ATTACHMENT2 ,
		GL_COLOR_ATTACHMENT3 ,
		GL_COLOR_ATTACHMENT4
	};

	// Generate our scene depth texture ...
	GenerateScreenTexture(processColourTex);
	GenerateScreenTexture(alphaColourTex);
	GenerateScreenTexture(alphaDepthTex, true);
	GenerateScreenTexture(alphaColourTex_2);
	GenerateScreenTexture(alphaDepthTex_2, true);
	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(bufferStochasticNormalTex);
	GenerateScreenTexture(reflectionBufferTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	GenerateScreenTexture(bufferUVTex);

	GeneratePositionTexture(bufferViewSpacePosTex);
	GeneratePositionTexture(debugStorageTex1);
	GeneratePositionTexture(debugStorageTex2);
	GeneratePositionTexture(debugStorageTex3);

	//First camera alpha FBO
	glBindFramebuffer(GL_FRAMEBUFFER, alphaFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, alphaColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, alphaDepthTex, 0);
	glDrawBuffers(1, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Second camera alpha FBO
	glBindFramebuffer(GL_FRAMEBUFFER, alphaFBO_2);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, alphaColourTex_2, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, alphaDepthTex_2, 0);
	glDrawBuffers(1, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//And now attach them to our FBOs
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, bufferStochasticNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, reflectionBufferTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(4, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Preparing the other rendering pass
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Preparing UV FBO
	glBindFramebuffer(GL_FRAMEBUFFER, UVFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferUVTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferViewSpacePosTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, debugStorageTex1, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, debugStorageTex2, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, debugStorageTex3, 0);
	glDrawBuffers(5, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	onRails = false;
	railStage = 0;
	timePassed = 0;

	init = true;
}

Renderer::~Renderer(void) {
	delete sceneShader;
	delete combineShader;
	delete pointlightShader;
	delete skyboxShader;
	delete alphaShader;
	delete heightShader;
	delete meshshader;
	delete endshader;
	delete blurShader;
	delete sobelShader;
	delete animatedshader;
	delete marchShader;

	delete heightMap;
	delete camera;
	delete sphere;
	delete quad;
	delete[] pointLights;

	glDeleteTextures(1, &processColourTex);
	glDeleteTextures(1, &currentAlphaColourTex);
	glDeleteTextures(1, &currentAlphaDepthTex);
	glDeleteTextures(1, &alphaColourTex);
	glDeleteTextures(1, &alphaColourTex_2);
	glDeleteTextures(1, &alphaDepthTex);
	glDeleteTextures(1, &alphaDepthTex_2);
	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferStochasticNormalTex);
	glDeleteTextures(1, &reflectionBufferTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteTextures(1, &bufferViewSpacePosTex);
	glDeleteTextures(1, &debugStorageTex1);
	glDeleteTextures(1, &debugStorageTex2);
	glDeleteTextures(1, &debugStorageTex3);

	glDeleteFramebuffers(1, &processFBO);
	glDeleteFramebuffers(1, &currentAlphaFBO);
	glDeleteFramebuffers(1, &alphaFBO);
	glDeleteFramebuffers(1, &alphaFBO_2);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::GeneratePositionTexture(GLuint& into)
{
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::UpdateScene(float dt) {
	if (onRails) { railMovement(dt); }
	else { activeCamera->UpdateCamera(dt); }
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);
}

void Renderer::railMovement(float dt) {
	timePassed += dt;
	if (railStage == 0) {
		float limit = 10;
		float ratio = timePassed / limit;
		Vector3 currentPos = startPos + Vector3(ratio * (onePos.x - startPos.x), ratio * (onePos.y - startPos.y), ratio * (onePos.z - startPos.z));
		camera->SetPosition(currentPos);
		camera->SetPitch(10 + ratio * (-15 - 10));
		camera->SetYaw(0 + ratio * 30);
		if (timePassed >= limit) { railStage = 1; timePassed = 0; }
	}

	if (railStage == 1) {
		float limit = 5;
		float ratio = timePassed / limit;
		Vector3 currentPos = onePos + Vector3(ratio * (twoPos.x - onePos.x), ratio * (twoPos.y - onePos.y), ratio * (twoPos.z - onePos.z));
		camera->SetPosition(currentPos);
		camera->SetPitch(-15 + ratio * (-10 - -15));
		camera->SetYaw(30 + ratio * (0 - 30));
		if (timePassed >= limit) { railStage = 2; timePassed = 0; }
	}

	if (railStage == 2) {
		float limit = 10;
		float ratio = timePassed / limit;
		Vector3 currentPos = twoPos + Vector3(ratio * (threePos.x - twoPos.x), ratio * (threePos.y - twoPos.y), ratio * (threePos.z - twoPos.z));
		camera->SetPosition(currentPos);
		camera->SetPitch(-10 + ratio * (-25 - -10));
		camera->SetYaw(0 + ratio * -30);
		if (timePassed >= limit) { breakRail(); }
	}
}

void Renderer::UpdateScene_2() {
	viewMatrix = activeCamera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - activeCamera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w == 0.9f) {
			transparentNodeList.push_back(from);
		}
		else if (from->GetColour().w < 0.9f) {
			animNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}

	for (vector <SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), //note the r!
		transparentNodeList.rend(), //note the r!
		SceneNode::CompareByCameraDistance);

	std::sort(nodeList.begin(),
		nodeList.end(),
		SceneNode::CompareByCameraDistance);

	std::sort(animNodeList.begin(),
		animNodeList.end(),
		SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNodesSolid() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNodesAnim() {
	for (const auto& i : animNodeList) {
		DrawNodeAnim((AnimatedNode*)i);
	}
}

void Renderer::DrawNodesAlpha() {
	for (const auto& i : transparentNodeList) {
		DrawAlphaNode(i);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {


		glUniform4fv(glGetUniformLocation(currentShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		texture = n->GetTexture();
		SetTextureRepeating(texture, true);

		reflectStorage = n->GetReflect();
		SetTextureRepeating(reflectStorage, true);

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "reflectTex"), 2);
		
		UpdateShaderMatrices();
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, model.values);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, nullBump);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, reflectStorage);

		n->Draw(*this);
	}
}

void Renderer::DrawNodeAnim(AnimatedNode* n) {
	vector <Matrix4> frameMatrices;
	MeshAnimation* anim = n->getAnim();
	vector<GLuint> mat = n->getMat();
	int currentFrame = n->getCurrentFrame();

	const Matrix4* invBindPose = n->GetMesh()->GetInverseBindPose();
	const Matrix4* frameData = anim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < n->GetMesh()->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(currentShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	UpdateShaderMatrices();

	Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, model.values);

	for (int i = 0; i < n->GetMesh()->GetSubMeshCount(); ++i) {
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mat[i]);

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, nullBump);

		n->GetMesh()->DrawSubMesh(i);
	}
}

void Renderer::DrawAlphaNode(SceneNode* n) {
	if (n->GetMesh()) {
		UpdateShaderMatrices();
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, model.values);

		glUniform4fv(glGetUniformLocation(currentShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		texture = n->GetTexture();
		SetTextureRepeating(texture, true);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "useTexture"), texture);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

		n->Draw(*this);
	}
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BuildNodeLists(root);
	SortNodeLists();

	FillBuffers();
	DrawPointLights();

	// Raymarch
	RaymarchLighting();

	CombineBuffers();

	DrawAlphaMeshes();

	ClearNodeLists();
}

//void Renderer::RenderScene() {
//	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
//	BuildNodeLists(root);
//	SortNodeLists();
//
//	FillBuffers();
//	DrawPointLights();
//
//	// Raymarch
//	RaymarchLighting();
//
//	CombineBuffers();
//
//	DrawAlphaMeshes();
//
//	ClearNodeLists();
//}

void Renderer::DrawSkybox() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//
	DrawSkybox();

	//BindShader(heightShader);
	//glUniform1i(glGetUniformLocation(heightShader->GetProgram(), "diffuseTex"), 0);
	//glUniform1i(glGetUniformLocation(heightShader->GetProgram(), "bumpTex"), 1);
	//glUniform1i(glGetUniformLocation(heightShader->GetProgram(), "diffuseTex_2"), 2);
	//glUniform1i(glGetUniformLocation(heightShader->GetProgram(), "bumpTex_2"), 3);
	//
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, grassTex);
	//
	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, grassBump);
	//
	//glActiveTexture(GL_TEXTURE2);
	//glBindTexture(GL_TEXTURE_2D, earthTex);
	//
	//glActiveTexture(GL_TEXTURE3);
	//glBindTexture(GL_TEXTURE_2D, earthBump);
	//
	modelMatrix.ToIdentity();
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	
	UpdateShaderMatrices();
	//
	//heightMap->Draw();
	//
	//render meshes

	BindShader(meshshader);

	DrawNodesSolid();

	BindShader(animatedshader);

	DrawNodesAnim();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, currentAlphaFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(combineShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "rayMarchUV"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, bufferUVTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "reflectivity"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, reflectionBufferTex);

	

	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawAlphaMeshes() {
	glBindFramebuffer(GL_FRAMEBUFFER, currentAlphaFBO);
	glBlitNamedFramebuffer(bufferFBO, currentAlphaFBO, 0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	BindShader(alphaShader);

	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	UpdateShaderMatrices();

	glDisable(GL_CULL_FACE);

	DrawNodesAlpha();

	glEnable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RaymarchLighting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, UVFBO);
	BindShader(marchShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	//glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "positionTexture"), 0);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, bufferViewSpacePosTex);

	glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "hemisphereTexture"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferStochasticNormalTex);

	glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "normalTexture"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "reflectionTexture"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, reflectionBufferTex);

	// Skybox
	glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "cubeTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	//Reflected geometry colour
	glUniform1i(glGetUniformLocation(marchShader->GetProgram(), "baseTexture"), 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	Matrix4 tempProjMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	glUniformMatrix4fv(glGetUniformLocation(marchShader->GetProgram(), "lensProjection"), 1, false, tempProjMatrix.values);

	Matrix4 invProj = tempProjMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(marchShader->GetProgram(), "inverseProjection"), 1, false, invProj.values);

	Matrix4 tempViewMatrix = activeCamera->BuildViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(marchShader->GetProgram(), "NormalViewMatrix"), 1, false, tempViewMatrix.values);

	Matrix4 inverseTempViewMatrix = tempViewMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(marchShader->GetProgram(), "InverseViewMatrix"), 1, false, inverseTempViewMatrix.values);
	

	// Camera position
	glUniform3fv(glGetUniformLocation(marchShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());
	

	glUniform2f(glGetUniformLocation(marchShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	quad->Draw();

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawToScreen() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(endshader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);



	


	glUniform1i(glGetUniformLocation(endshader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, alphaColourTex);

	//stochastic test
	//glBindTexture(GL_TEXTURE_2D, bufferStochasticNormalTex);

	//UV test
	//glBindTexture(GL_TEXTURE_2D, bufferUVTex);

	//Normal test
	//glBindTexture(GL_TEXTURE_2D, bufferNormalTex);
	
	
	if (twoCameras == false) {
		quad->Draw();
	}
	else {
		quad_L->Draw();

		glUniform1i(glGetUniformLocation(endshader->GetProgram(), "diffuseTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, alphaColourTex_2);

		quad_R->Draw();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::useBlur() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(blurShader);
}

void Renderer::useSobel() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sobelShader);
}

void Renderer::drawPostProcess() {
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, alphaColourTex);
		quad->Draw();
		//Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, alphaColourTex, 0);
		glBindTexture(GL_TEXTURE_2D, processColourTex);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
	animNodeList.clear();
}

void Renderer::firstCameraBuffer() {
	currentAlphaFBO = alphaFBO;
	currentAlphaColourTex = alphaColourTex;
	currentAlphaDepthTex = alphaDepthTex;
}

void Renderer::secondCameraBuffer() {
	currentAlphaFBO = alphaFBO_2;
	currentAlphaColourTex = alphaColourTex_2;
	currentAlphaDepthTex = alphaDepthTex_2;
}