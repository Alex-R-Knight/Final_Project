#include "Renderer.h"
#include "../nclgl/CubeRobot.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include <algorithm > //For std::sort ...
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"
const int LIGHT_NUM = 2;
const int POST_PASSES = 10;

const int REFLECT_BLUR_PASSES = 2;

const int ILLUMINATION_BLUR_PASSES = 10;

const int SSAO_BLUR_PASSES = 2;

const int POST_SOBEL_BLUR_PASSES = 1;

// MUST BE CHANGED IN "SSAOFrag.glsl" AS WELL
const int SSAO_KERNEL_COUNT = 16;

// Deferred shadowmapping
const unsigned int SHADOWSIZE = 2048;
//const unsigned int SHADOWSIZE = 1024;
//const unsigned int SHADOWSIZE = 512;

// Virtual Point Lighting Parameters
const Vector3 virtualLightStartPos = Vector3(0.0f, 0.0f, 0.0f);

const int virtualLightRowsHorizontal = 5;

const int virtualLightSpreadHorizontal = 50;

const int virtualLightRowsVertical = 1;

const int virtualLightSpreadVertical = 0;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	// For timer usage
	glGenQueries(1, &queryObject);

	spinnyTime = 0.0f;
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


	// Virtual Point Light count

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
		"FinalMixingFrag.glsl");

	blurShader = new Shader("TexturedVertex.glsl",
		"processfrag.glsl");

	sobelShader = new Shader("TexturedVertex.glsl",
		"processfrag_sobel_edge.glsl");

	reflectionShader = new Shader("TexturedVertex.glsl",
		"reflectionRayFrag.glsl");

	illuminationShader = new Shader("TexturedVertex.glsl",
		"lightingRayFrag.glsl");

	shadowShader = new Shader("deferredShadowVert.glsl",
		"deferredShadowFrag.glsl");

	SSAOShader = new Shader("TexturedVertex.glsl",
		"SSAOFrag.glsl");

	SobelDepthShader = new Shader("TexturedVertex.glsl",
		"processfrag_sobel_depth.glsl");

	virtualPointlightShader = new Shader("pointlightvert.glsl",
		"virtualPointlightfrag.glsl");

	pointlightRaymarchShader = new Shader("pointlightvert.glsl",
		"pointlightRaymarchFrag.glsl");

	if (!sceneShader->LoadSuccess()			|| !pointlightShader->LoadSuccess()
		|| !combineShader->LoadSuccess()	|| !skyboxShader->LoadSuccess()
		|| !alphaShader->LoadSuccess()		|| !heightShader->LoadSuccess()
		|| !meshshader->LoadSuccess()		|| !endshader->LoadSuccess()
		|| !blurShader->LoadSuccess()		|| !sobelShader->LoadSuccess()
		|| !animatedshader->LoadSuccess()	|| !reflectionShader->LoadSuccess()
		|| !shadowShader->LoadSuccess()		|| !illuminationShader->LoadSuccess()
		|| !SSAOShader->LoadSuccess()		|| !SobelDepthShader->LoadSuccess()	
		|| !virtualPointlightShader->LoadSuccess()
		|| !pointlightRaymarchShader->LoadSuccess()			) {
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

	// Real Point Lights

	for (int i = 0; i < LIGHT_NUM; i++) {

		Vector3 newlocation = Vector3(30.0f - i * 50.0f, 5.0f, 0.0f);

		Light& l = pointLights[i];
		l.SetPosition(Vector3(newlocation.x, newlocation.y, newlocation.z));

		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 2000));
		l.SetRadius(75.0f);
		//l.SetRadius(50.0f + (rand() % 30));

	}


	// Virtual Point Light Distribution

	float VPLStartOffsetVal = virtualLightSpreadHorizontal - (virtualLightSpreadHorizontal / virtualLightRowsHorizontal);
	Vector3 VPLStartOffset = Vector3(VPLStartOffsetVal, 0.0f, VPLStartOffsetVal);
	float VPLIncrementHorizontal = 2 * (virtualLightSpreadHorizontal / virtualLightRowsHorizontal);

	int VPLIterationCounter = 0;

	for (int i = 0; i < virtualLightRowsHorizontal; i++) // X
	{
		for (int j = 0; j < virtualLightRowsHorizontal; j++) // Z
		{
			Vector3 newVPLLocation = (virtualLightStartPos - VPLStartOffset) + Vector3(i * VPLIncrementHorizontal, 0.0f, j * VPLIncrementHorizontal);

			virtualPointLights.push_back(newVPLLocation);
			virtualPointLightsColour.push_back(Vector3(0.0f, 0.0f, 0.0f));
			virtualPointLightsRadius.push_back(0.0f);

		}
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
	testCube->SetBoundingRadius(6.0f);
	testCube->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube->SetTexture(earthTex);
	testCube->SetReflect(unreflectiveTex);
	
	root->AddChild(testCube);
	
	SceneNode* testCube2 = new SceneNode();
	testCube2->SetTransform(Matrix4::Translation(Vector3(0.0f, 5.0f, -30.0f)));
	testCube2->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube2->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube2->SetBoundingRadius(6.0f);
	testCube2->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube2->SetTexture(earthTex);
	testCube2->SetReflect(unreflectiveTex);
	
	root->AddChild(testCube2);
	
	SceneNode* testCube3 = new SceneNode();
	testCube3->SetTransform(Matrix4::Translation(Vector3(5.0f, 5.0f, -30.0f)));
	testCube3->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube3->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube3->SetBoundingRadius(6.0f);
	testCube3->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube3->SetTexture(grassTex);
	testCube3->SetReflect(unreflectiveTex);
	
	root->AddChild(testCube3);

	SceneNode* testCube4 = new SceneNode();
	testCube4->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -30.0f)));
	testCube4->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube4->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube4->SetBoundingRadius(6.0f);
	testCube4->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube4->SetTexture(earthTex);
	testCube4->SetReflect(unreflectiveTex);

	root->AddChild(testCube4);

	SceneNode* testCube5 = new SceneNode();
	testCube5->SetTransform(Matrix4::Translation(Vector3(5.0f, 0.0f, -30.0f)));
	testCube5->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube5->SetModelScale(Vector3(5.0f, 5.0f, 5.0f));
	testCube5->SetBoundingRadius(6.0f);
	testCube5->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube5->SetTexture(grassTex);
	testCube5->SetReflect(unreflectiveTex);

	root->AddChild(testCube5);

	SceneNode* testCube6 = new SceneNode();
	testCube6->SetTransform(Matrix4::Translation(Vector3(-5.0f, -0.5f, -30.0f)));
	testCube6->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube6->SetModelScale(Vector3(3.0f, 3.0f, 3.0f));
	testCube6->SetBoundingRadius(6.0f);
	testCube6->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube6->SetTexture(grassTex);
	testCube6->SetReflect(unreflectiveTex);

	root->AddChild(testCube6);

	SceneNode* testCube7 = new SceneNode();
	testCube7->SetTransform(Matrix4::Translation(Vector3(-10.0f, 0.0f, 0.0f)));
	testCube7->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube7->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	testCube7->SetBoundingRadius(6.0f);
	testCube7->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube7->SetTexture(grassTex);
	testCube7->SetReflect(unreflectiveTex);

	root->AddChild(testCube7);

	SceneNode* testCube8 = new SceneNode();
	testCube8->SetTransform(Matrix4::Translation(Vector3(-30.0f, 0.0f, 0.0f)));
	testCube8->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube8->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	testCube8->SetBoundingRadius(6.0f);
	testCube8->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube8->SetTexture(grassTex);
	testCube8->SetReflect(unreflectiveTex);

	root->AddChild(testCube8);

	SceneNode* testCube9 = new SceneNode();
	testCube9->SetTransform(Matrix4::Translation(Vector3(-20.0f, 0.0f, -10.0f)));
	testCube9->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube9->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	testCube9->SetBoundingRadius(6.0f);
	testCube9->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube9->SetTexture(grassTex);
	testCube9->SetReflect(unreflectiveTex);

	root->AddChild(testCube9);

	SceneNode* testCube10 = new SceneNode();
	testCube10->SetTransform(Matrix4::Translation(Vector3(-20.0f, 0.0f, -10.0f)));
	testCube10->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube10->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	testCube10->SetBoundingRadius(6.0f);
	testCube10->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube10->SetTexture(grassTex);
	testCube10->SetReflect(unreflectiveTex);

	root->AddChild(testCube10);

	SceneNode* testCube11 = new SceneNode();
	testCube11->SetTransform(Matrix4::Translation(Vector3(-20.0f, 0.0f, 10.0f)));
	testCube11->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube11->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	testCube11->SetBoundingRadius(6.0f);
	testCube11->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube11->SetTexture(grassTex);
	testCube11->SetReflect(unreflectiveTex);

	root->AddChild(testCube11);

	SceneNode* testCube12 = new SceneNode();
	testCube12->SetTransform(Matrix4::Translation(Vector3(40.0f, 0.0f, 10.0f)));
	testCube12->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	testCube12->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	testCube12->SetBoundingRadius(6.0f);
	testCube12->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	testCube12->SetTexture(grassTex);
	testCube12->SetReflect(unreflectiveTex);

	root->AddChild(testCube12);

	rotatingCube = new SceneNode();
	rotatingCube->SetTransform(Matrix4::Translation(Vector3(15.0f, 0.0f, -15.0f)));
	rotatingCube->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rotatingCube->SetModelScale(Vector3(4.0f, 4.0f, 4.0f));
	rotatingCube->SetBoundingRadius(100.0f);
	rotatingCube->SetMesh(Mesh::LoadFromMeshFile("Cube.msh"));
	rotatingCube->SetTexture(earthTex);
	rotatingCube->SetReflect(unreflectiveTex);

	root->AddChild(rotatingCube);

	glGenFramebuffers(1, &alphaFBO);
	glGenFramebuffers(1, &alphaFBO_2);
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);
	glGenFramebuffers(1, &processFBO);

	glGenFramebuffers(1, &reflectionFBO);

	glGenFramebuffers(1, &SSAOFBO);

	glGenFramebuffers(1, &illuminationFBO);

	glGenFramebuffers(1, &edgeFBO);

	GLenum buffers[7] = {
		GL_COLOR_ATTACHMENT0 ,
		GL_COLOR_ATTACHMENT1 ,
		GL_COLOR_ATTACHMENT2 ,
		GL_COLOR_ATTACHMENT3 ,
		GL_COLOR_ATTACHMENT4 ,
		GL_COLOR_ATTACHMENT5 ,
		GL_COLOR_ATTACHMENT6
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
	GenerateScreenTexture(reflectionBufferTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	GenerateScreenTexture(reflectionStorageTex);

	GenerateScreenTexture(illuminationStorageTex);

	GenerateScreenTexture(SSAOTex);

	GenerateScreenTexture(edgeStorageTex);

	GeneratePositionTexture(bufferViewSpacePosTex);
	GeneratePositionTexture(debugStorageTex1);
	GeneratePositionTexture(debugStorageTex2);
	GeneratePositionTexture(debugStorageTex3);

////// Deferred shadowmapping

	shadowProj = Matrix4::Perspective(1, 100, 1, 90);

	for (int i = 0; i < LIGHT_NUM; i++) {
	
		vector<GLuint> newShadowFBOs;

		vector<GLuint> shadowTextures;

		for (int i = 0; i < 6; i++)
		{
			GLuint newFBO;
			newShadowFBOs.push_back(newFBO);


			GLuint shadowTex;
			shadowTextures.push_back(shadowTex);

			glGenTextures(1, &shadowTextures[i]);
			glBindTexture(GL_TEXTURE_2D, shadowTextures[i]);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

			glBindTexture(GL_TEXTURE_2D, 0);

			////

			glGenFramebuffers(1, &newShadowFBOs[i]);

			glBindFramebuffer(GL_FRAMEBUFFER, newShadowFBOs[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTextures[i], 0);
			glDrawBuffer(GL_NONE);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

		}

		shadowMaps.push_back(shadowTextures);

		shadowFBO.push_back(newShadowFBOs);

		// Light direction transforms
		
		vector<Matrix4> newLightTransforms;

		// In order: X_pox, X_neg, Y_pos, Y_neg, Z_pos, Z_neg
		newLightTransforms.push_back( Matrix4::BuildViewMatrix( pointLights[i].GetPosition(), pointLights[i].GetPosition() + Vector3(1, 0, 0)));
		newLightTransforms.push_back( Matrix4::BuildViewMatrix( pointLights[i].GetPosition(), pointLights[i].GetPosition() + Vector3(-1, 0, 0)));
		newLightTransforms.push_back( Matrix4::BuildViewMatrix( pointLights[i].GetPosition(), pointLights[i].GetPosition() + Vector3(0, 1, 0)));
		newLightTransforms.push_back( Matrix4::BuildViewMatrix( pointLights[i].GetPosition(), pointLights[i].GetPosition() + Vector3(0, -1, 0)));
		newLightTransforms.push_back( Matrix4::BuildViewMatrix( pointLights[i].GetPosition(), pointLights[i].GetPosition() + Vector3(0, 0, 1)));
		newLightTransforms.push_back( Matrix4::BuildViewMatrix( pointLights[i].GetPosition(), pointLights[i].GetPosition() + Vector3(0, 0, -1)));
		
		shadowTransforms.push_back(newLightTransforms);
		
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
//////


////// SSAO Prep

	// Kernel generation
	for (int i = 0; i < SSAO_KERNEL_COUNT; i++)
	{
		Vector3 kernel(
			(float)(rand() / (float)RAND_MAX) * 2.0 - 1.0,
			(float)(rand() / (float)RAND_MAX) * 2.0 - 1.0,
			(float)(rand() / (float)RAND_MAX)
		);
		kernel = kernel.Normalised();

		float scale = (float)i / SSAO_KERNEL_COUNT;

		scale = 0.1f + (scale * scale) * 0.9f;

		kernel = kernel * scale;

		SSAOKernels.push_back(kernel);
	}

	// Noise generation
	// SSAO
	for (int i = 0; i < 16; i++)
	{
		Vector3 noise(
			(float)(rand() / (float)RAND_MAX) * 2.0 - 1.0,
			(float)(rand() / (float)RAND_MAX) * 2.0 - 1.0,
			0.0f
		);
		SSAONoise.push_back(noise);
	}

	// Noise texture creation
	glGenTextures(1, &SSAONoiseTex);
	glBindTexture(GL_TEXTURE_2D, SSAONoiseTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &SSAONoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// IGN
	vector<Vector3> IGNVector;

	for (int y = 0; y < 16; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			float noiseVal = IGN(x, y);
			float noiseVal2 = IGN(y, x);
			Vector3 newIGNVector = Vector3(noiseVal, noiseVal2, 0.0f);
			IGNVector.push_back(newIGNVector);
		}
	}
	glGenTextures(1, &illuminationNoiseTex);
	glBindTexture(GL_TEXTURE_2D, illuminationNoiseTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 16, 16, 0, GL_RGB, GL_FLOAT, &IGNVector[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

/////


	//First camera alpha FBO
	glBindFramebuffer(GL_FRAMEBUFFER, alphaFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, alphaColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferViewSpacePosTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, alphaDepthTex, 0);
	glDrawBuffers(2, buffers);

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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, reflectionBufferTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(3, buffers);

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

	//Preparing reflection FBO
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionStorageTex, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferViewSpacePosTex, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, debugStorageTex1, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, debugStorageTex2, 0);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, debugStorageTex3, 0);
	glDrawBuffers(1, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Preparing ilumination FBO
	glBindFramebuffer(GL_FRAMEBUFFER, illuminationFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, illuminationStorageTex, 0);
	glDrawBuffers(1, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Preparing SSAO FBO
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOTex, 0);
	glDrawBuffers(1, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	//Preparing Edge FBO
	glBindFramebuffer(GL_FRAMEBUFFER, edgeFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, edgeStorageTex, 0);
	glDrawBuffers(1, buffers);

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
	delete reflectionShader;
	delete illuminationShader;
	delete shadowShader;
	delete SSAOShader;

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
	glDeleteTextures(1, &reflectionBufferTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteTextures(1, &edgeStorageTex);


	// Noise textures here
	glDeleteTextures(1, &SSAONoiseTex);
	glDeleteTextures(1, &illuminationNoiseTex);

	// Debug textures here
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

	glDeleteFramebuffers(1, &reflectionFBO);
	glDeleteFramebuffers(1, &illuminationFBO);

	glDeleteFramebuffers(1, &SSAOFBO);

	glDeleteFramebuffers(1, &edgeFBO);


	for (const auto& i : shadowFBO)
	{
		for (int j = 0; j < 6; j++)
		{
			glDeleteFramebuffers(1, &i[j]);
		}
	}

	for (const auto& i : shadowMaps)
	{
		for (int j = 0; j < 6; j++)
		{
			glDeleteTextures(1, &i[j]);
		}
	}
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

	spinnyTime += dt;

	rotatingCube->SetTransform(Matrix4::Translation(Vector3(15.0f, 0.0f, -15.0f)) * Matrix4::Rotation(spinnyTime * 10, Vector3(0, 1, 0)));

	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 1000.0f, (float)width / (float)height, 45.0f);
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

void Renderer::FillShadowMaps()
{

	for (int i = 0; i < LIGHT_NUM; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[i][j]);

			glClear(GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			BindShader(shadowShader);

			viewMatrix = shadowTransforms[i][j];
			projMatrix = shadowProj;
			UpdateShaderMatrices();


			DrawNodesSolid();
		}
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BuildNodeLists(root);
	SortNodeLists();

	std::cout << "Shadowmapping" << std::endl;
	startTiming();
	FillShadowMaps();
	endTiming();

	std::cout << "Fill buffers" << std::endl;
	startTiming();
	FillBuffers();
	endTiming();

	std::cout << "Point Lights" << std::endl;
	startTiming();
	// Toggle between these as needed
	//DrawPointLights();
	DrawPointLightsRaymarched();
	endTiming();


	// Dont forget to toggle me as needed
	//DrawVirtualPointLights();

	std::cout << "SSAO Process" << std::endl;
	startTiming();
	// SSAO
	SSAOProcess();
	endTiming();

	std::cout << "SSAO Blur" << std::endl;
	startTiming();
	SSAOBlurring();
	endTiming();

	// Raymarch lighting
	std::cout << "Global Illumination" << std::endl;
	startTiming();
	RaymarchLighting();
	endTiming();

	std::cout << "Global Illumination Blurring" << std::endl;
	startTiming();
	LightingBlurring();
	endTiming();

	CombineBuffers();

	// Raymarch reflections
	std::cout << "Reflection Process" << std::endl;
	startTiming();
	RaymarchReflection();
	endTiming();

	std::cout << "Reflection Blurring" << std::endl;
	startTiming();
	ReflectionBlurring();
	endTiming();

	DrawAlphaMeshes();

	std::cout << "Edge Detection" << std::endl;
	startTiming();
	SobelProcess();
	endTiming();

	std::cout << "Edge Detection Blur" << std::endl;
	startTiming();
	SobelBlurring();
	endTiming();

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

	glClearColor(0, 0, 0, 1);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	quad->Draw();

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glDepthMask(GL_TRUE);
}

void Renderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//

	//DrawSkybox();

	
	modelMatrix.ToIdentity();
	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 1000.0f, (float)width / (float)height, 45.0f);
	
	UpdateShaderMatrices();

	glClearColor(0, 0, 0, 1);
	DrawSkybox();
	glClearColor(0.2f, 0.2f, 0.2f, 1);

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

		// The many shadow maps of RSI

		glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "shadowTex1"), 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][0]);

		glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "shadowTex2"), 3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][1]);

		glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "shadowTex3"), 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][2]);

		glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "shadowTex4"), 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][3]);

		glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "shadowTex5"), 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][4]);

		glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "shadowTex6"), 7);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][5]);

		// IMPORTANT: Dont forget to enter the view and projection matrices
		// SEALED AWAY FOR ESOTERIC GPU ISSUES
		//vector<Matrix4> ShadowVPMatrices;

		//for (int j = 0; j < 6; j++)
		//{
		//	ShadowVPMatrices.push_back( shadowProj * shadowTransforms[i][j]);
		//}

		//glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix"), 6, false, (float*)ShadowVPMatrices.data() );


		// Fixed matrix solution
		// anomalous GPU behaviour prevents streamlined method

		Matrix4 shadowMatrix1 = shadowProj * shadowTransforms[i][0];
		glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix1"), 1, false, shadowMatrix1.values);

		Matrix4 shadowMatrix2 = shadowProj * shadowTransforms[i][1];
		glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix2"), 1, false, shadowMatrix2.values);

		Matrix4 shadowMatrix3 = shadowProj * shadowTransforms[i][2];
		glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix3"), 1, false, shadowMatrix3.values);

		Matrix4 shadowMatrix4 = shadowProj * shadowTransforms[i][3];
		glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix4"), 1, false, shadowMatrix4.values);

		Matrix4 shadowMatrix5 = shadowProj * shadowTransforms[i][4];
		glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix5"), 1, false, shadowMatrix5.values);

		Matrix4 shadowMatrix6 = shadowProj * shadowTransforms[i][5];
		glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "shadowMatrix6"), 1, false, shadowMatrix6.values);



		UpdateShaderMatrices();

		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPointLightsRaymarched()
{
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightRaymarchShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);


	glUniform3fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	glUniform2f(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	Matrix4 tempProjMatrix = projMatrix;
	Matrix4 invProj = tempProjMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "inverseProjection"), 1, false, invProj.values);

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);

		// The many shadow maps of RSI

		glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowTex1"), 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][0]);

		glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowTex2"), 3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][1]);

		glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowTex3"), 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][2]);

		glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowTex4"), 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][3]);

		glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowTex5"), 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][4]);

		glUniform1i(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowTex6"), 7);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i][5]);

		// IMPORTANT: Dont forget to enter the view and projection matrices

		//vector<Matrix4> ShadowVPMatrices;
		//
		//for (int j = 0; j < 6; j++)
		//{
		//	ShadowVPMatrices.push_back(shadowProj * shadowTransforms[i][j]);
		//}
		//
		//glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix"), 6, false, (float*)ShadowVPMatrices.data());

		// Fixed matrix solution
		// anomalous GPU behaviour prevents streamlined method

		Matrix4 shadowMatrix1 = shadowProj * shadowTransforms[i][0];
		glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix1"), 1, false, shadowMatrix1.values);

		Matrix4 shadowMatrix2 = shadowProj * shadowTransforms[i][1];
		glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix2"), 1, false, shadowMatrix2.values);

		Matrix4 shadowMatrix3 = shadowProj * shadowTransforms[i][2];
		glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix3"), 1, false, shadowMatrix3.values);

		Matrix4 shadowMatrix4 = shadowProj * shadowTransforms[i][3];
		glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix4"), 1, false, shadowMatrix4.values);

		Matrix4 shadowMatrix5 = shadowProj * shadowTransforms[i][4];
		glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix5"), 1, false, shadowMatrix5.values);

		Matrix4 shadowMatrix6 = shadowProj * shadowTransforms[i][5];
		glUniformMatrix4fv(glGetUniformLocation(pointlightRaymarchShader->GetProgram(), "shadowMatrix6"), 1, false, shadowMatrix6.values);

		UpdateShaderMatrices();

		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawVirtualPointLights()
{
	// VPL Occlusion and Light Data Management

	for (int i = 0; i < virtualPointLights.size(); i++)
	{
		Vector3 VPLWorldPosition = virtualPointLights[i];
		Vector4 VPLWorldPositionVec4 = Vector4(VPLWorldPosition.x, VPLWorldPosition.y, VPLWorldPosition.z, 1.0f);

		Vector3 colourValues = Vector3(0.0f, 0.0f, 0.0f);

		float maxRadius = 0.0f;

		for (int j = 0; j < LIGHT_NUM; ++j)
		{
			Light& l = pointLights[j];

			// Is hit bool
			bool withinRealLight = false;

			// Distance checks
			float lightDistance = (VPLWorldPosition - l.GetPosition()).Length();
			float realLightRadius = l.GetRadius();

			// If beyond light radius, skip
			if (lightDistance > realLightRadius)
			{
				continue;
			}

			// Per-shadowmap test
			for (int sixth = 0; sixth < 6; sixth++)
			{
				Matrix4 LightShadowTransform = shadowTransforms[j][sixth];

				Vector4 virtshadowProj = shadowProj * LightShadowTransform * VPLWorldPositionVec4;

				Vector3 shadowNDC = Vector3(virtshadowProj.x, virtshadowProj.y, virtshadowProj.z) / virtshadowProj.w;
				if (abs(shadowNDC.x) < 1.0f &&
					abs(shadowNDC.y) < 1.0f &&
					abs(shadowNDC.z) < 1.0f)
				{
					Vector3 biasCoord = (shadowNDC * 0.5f) + Vector3(0.5f, 0.5f, 0.5f);

					float depthVal;

					glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO[j][sixth]);
					glReadBuffer(GL_DEPTH_ATTACHMENT);
					glReadPixels(biasCoord.x, biasCoord.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthVal);

					std::cout << "depthval = " << depthVal << "\n";
					std::cout << "biasCoord = " << biasCoord.z << "\n";

					if (depthVal > biasCoord.z) {
						std::cout << "within light\n";
						withinRealLight = true;
						break;
					}
				}
			}

			if (withinRealLight)
			{
				// Update virtual light radius if this light has greater impact
				float radiusDifference = realLightRadius - lightDistance;
				maxRadius = (radiusDifference > maxRadius) ? radiusDifference : maxRadius;

				Vector4 sourceLightColourVec4 = l.GetColour();

				// Attenuation methods
				//float attenuation = 1.0 - (lightDistance / realLightRadius);

				float RadiusAtten = 1.0 - (lightDistance / realLightRadius);
				float invSqrAtten = sourceLightColourVec4.w / (lightDistance * lightDistance);

				float attenuation = RadiusAtten * invSqrAtten;

				attenuation = (attenuation < 1.0f) ? attenuation : 1.0f;

				colourValues += Vector3(sourceLightColourVec4.x, sourceLightColourVec4.y, sourceLightColourVec4.z) * attenuation;
			}
		}
		// Apply calculated values to vectors
		virtualPointLightsColour[i] = colourValues;
		virtualPointLightsRadius[i] = maxRadius;
	}


	// Drawing virtual point lights

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(virtualPointlightShader);

	glClearColor(0, 0, 0, 1);

	// Dont forget to disable me for normal usage
	glClear(GL_COLOR_BUFFER_BIT);

	glBlendFunc(GL_ONE, GL_ONE);

	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(virtualPointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(virtualPointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);


	glUniform3fv(glGetUniformLocation(virtualPointlightShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());

	glUniform2f(glGetUniformLocation(virtualPointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(virtualPointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();

	for (int i = 0; i < virtualPointLights.size(); ++i) {

		Vector3 virtualPointLightPosition = virtualPointLights[i];
		glUniform3fv(glGetUniformLocation(virtualPointlightShader->GetProgram(), "lightPos"), 1, (float*)&virtualPointLightPosition);

		Vector4 virtualPointLightColour = Vector4(virtualPointLightsColour[i].x, virtualPointLightsColour[i].y, virtualPointLightsColour[i].z, 20.0f);
		glUniform4fv(glGetUniformLocation(virtualPointlightShader->GetProgram(), "lightColour"), 1, (float*)&virtualPointLightColour);

		float virtualPointLightRadius = virtualPointLightsRadius[i];
		//glUniform1f(glGetUniformLocation(virtualPointlightShader->GetProgram(), "lightRadius"), virtualPointLightRadius);
		glUniform1f(glGetUniformLocation(virtualPointlightShader->GetProgram(), "lightRadius"), 25.0f);

		UpdateShaderMatrices();

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

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "illuminationTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, illuminationStorageTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "SSAOTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, SSAOTex);
	

	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawAlphaMeshes() {
	glBindFramebuffer(GL_FRAMEBUFFER, currentAlphaFBO);
	glBlitNamedFramebuffer(bufferFBO, currentAlphaFBO, 0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	BindShader(alphaShader);

	viewMatrix = activeCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 1000.0f, (float)width / (float)height, 45.0f);

	UpdateShaderMatrices();

	glDisable(GL_CULL_FACE);

	DrawNodesAlpha();

	glEnable(GL_CULL_FACE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RaymarchLighting()
{
	glBindFramebuffer(GL_FRAMEBUFFER, illuminationFBO);
	BindShader(illuminationShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(illuminationShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	//Reflected geometry colour
	glUniform1i(glGetUniformLocation(illuminationShader->GetProgram(), "baseTexture"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	
	//Reflected geometry Light
	glUniform1i(glGetUniformLocation(illuminationShader->GetProgram(), "lightTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	// Normal texture for ray valuing
	glUniform1i(glGetUniformLocation(illuminationShader->GetProgram(), "normalTexture"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	// IGN Noise Texture
	glUniform1i(glGetUniformLocation(illuminationShader->GetProgram(), "noiseTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, illuminationNoiseTex);


	Matrix4 tempProjMatrix = Matrix4::Perspective(1.0f, 1000.0f, (float)width / (float)height, 45.0f);
	glUniformMatrix4fv(glGetUniformLocation(illuminationShader->GetProgram(), "lensProjection"), 1, false, tempProjMatrix.values);

	Matrix4 invProj = tempProjMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(illuminationShader->GetProgram(), "inverseProjection"), 1, false, invProj.values);

	Matrix4 tempViewMatrix = activeCamera->BuildViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(illuminationShader->GetProgram(), "NormalViewMatrix"), 1, false, tempViewMatrix.values);

	Matrix4 inverseTempViewMatrix = tempViewMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(illuminationShader->GetProgram(), "InverseViewMatrix"), 1, false, inverseTempViewMatrix.values);


	// Camera position
	glUniform3fv(glGetUniformLocation(illuminationShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());


	quad->Draw();

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::LightingBlurring()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(blurShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < ILLUMINATION_BLUR_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, illuminationStorageTex);
		quad->Draw();
		//Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, illuminationStorageTex, 0);
		glBindTexture(GL_TEXTURE_2D, processColourTex);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::RaymarchReflection()
{
	glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
	BindShader(reflectionShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	//glUniform1i(glGetUniformLocation(reflectionShader->GetProgram(), "positionTexture"), 0);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, bufferViewSpacePosTex);

	glUniform1i(glGetUniformLocation(reflectionShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(reflectionShader->GetProgram(), "normalTexture"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform1i(glGetUniformLocation(reflectionShader->GetProgram(), "reflectionTexture"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, reflectionBufferTex);

	// Skybox
	glUniform1i(glGetUniformLocation(reflectionShader->GetProgram(), "cubeTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	//Reflected geometry colour
	glUniform1i(glGetUniformLocation(reflectionShader->GetProgram(), "baseTexture"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, alphaColourTex);

	Matrix4 tempProjMatrix = Matrix4::Perspective(1.0f, 1000.0f, (float)width / (float)height, 45.0f);
	glUniformMatrix4fv(glGetUniformLocation(reflectionShader->GetProgram(), "lensProjection"), 1, false, tempProjMatrix.values);

	Matrix4 invProj = tempProjMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(reflectionShader->GetProgram(), "inverseProjection"), 1, false, invProj.values);

	Matrix4 tempViewMatrix = activeCamera->BuildViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(reflectionShader->GetProgram(), "NormalViewMatrix"), 1, false, tempViewMatrix.values);

	Matrix4 inverseTempViewMatrix = tempViewMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(reflectionShader->GetProgram(), "InverseViewMatrix"), 1, false, inverseTempViewMatrix.values);
	

	// Camera position
	glUniform3fv(glGetUniformLocation(reflectionShader->GetProgram(), "cameraPos"), 1, (float*)&activeCamera->GetPosition());


	quad->Draw();

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::ReflectionBlurring()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(blurShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < REFLECT_BLUR_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, reflectionStorageTex);
		quad->Draw();
		//Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionStorageTex, 0);
		glBindTexture(GL_TEXTURE_2D, processColourTex);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::SSAOProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, SSAOFBO);
	BindShader(SSAOShader);

	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(SSAOShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(SSAOShader->GetProgram(), "normalTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform1i(glGetUniformLocation(SSAOShader->GetProgram(), "noiseTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, SSAONoiseTex);


	Matrix4 tempProjMatrix = Matrix4::Perspective(1.0f, 1000.0f, (float)width / (float)height, 45.0f);
	glUniformMatrix4fv(glGetUniformLocation(SSAOShader->GetProgram(), "lensProjection"), 1, false, tempProjMatrix.values);

	Matrix4 invProj = tempProjMatrix.Inverse();
	glUniformMatrix4fv(glGetUniformLocation(SSAOShader->GetProgram(), "inverseProjection"), 1, false, invProj.values);

	Matrix4 tempViewMatrix = activeCamera->BuildViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(SSAOShader->GetProgram(), "NormalViewMatrix"), 1, false, tempViewMatrix.values);

	// Passing vec3 array values
	vector<float> flatArray;
	for (const auto& vec3 : SSAOKernels)
	{
		flatArray.push_back(vec3.x);
		flatArray.push_back(vec3.y);
		flatArray.push_back(vec3.z);
	}

	glUniform3fv(glGetUniformLocation(SSAOShader->GetProgram(), "samples"), SSAO_KERNEL_COUNT, flatArray.data());

	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::SSAOBlurring()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(blurShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < SSAO_BLUR_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, SSAOTex);
		quad->Draw();
		//Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SSAOTex, 0);
		glBindTexture(GL_TEXTURE_2D, processColourTex);
		quad->Draw();
	}
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

	glUniform1i(glGetUniformLocation(endshader->GetProgram(), "rayMarchUV"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, reflectionStorageTex);

	glUniform1i(glGetUniformLocation(endshader->GetProgram(), "reflectivity"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, reflectionBufferTex);

	glUniform1i(glGetUniformLocation(endshader->GetProgram(), "blurTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(endshader->GetProgram(), "sobelTex"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, edgeStorageTex);	
	

	quad->Draw();
	

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

float Renderer::IGN(int x, int y)
{
	return std::fmodf(52.9829189f * std::fmodf(0.06711056f * float(x) + 0.00583715f * float(y), 1.0f), 1.0f);
}

void Renderer::SobelProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, edgeFBO);
	BindShader(SobelDepthShader);

	glClearColor(0, 0, 0, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(SobelDepthShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(SobelDepthShader->GetProgram(), "normalTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform1i(glGetUniformLocation(SobelDepthShader->GetProgram(), "lightColourTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	quad->Draw();

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::SobelBlurring()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(blurShader);

	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "sceneTex"), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processColourTex, 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, alphaColourTex);
		quad->Draw();
		//Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
		glBindTexture(GL_TEXTURE_2D, processColourTex);
		quad->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::startTiming()
{
	if (!timingInProgress) {
		glBeginQuery(GL_TIME_ELAPSED, queryObject);
		timingInProgress = true;
	}
}

void Renderer::endTiming()
{
	if (timingInProgress) {
		glEndQuery(GL_TIME_ELAPSED);
		timingInProgress = false;

		// Get timer value
		glGetQueryObjectui64v(queryObject, GL_QUERY_RESULT, &endTime);

		// output time
		std::cout << "Elapsed Time: " << endTime << " ns" << std::endl;
	}
}