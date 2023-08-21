#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/AnimatedNode.h"
#include "../nclgl/Frustum.h"

class Camera;
class Mesh;
class HeightMap;
class MeshMaterial;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;
	void firstCamera() { activeCamera = camera; };

	void firstCameraBuffer();

	void useBlur();
	void useSobel();

	void drawPostProcess();

	void DrawToScreen(); //Draw end result to screen

	void startRail() { onRails = true; timePassed = 0; railStage = 0; };
	void breakRail() { onRails = false; };

protected:
	bool onRails;
	float timePassed;
	void railMovement(float dt);
	int railStage;

	Vector3 startPos;
	Vector3 onePos;
	Vector3 twoPos;
	Vector3 threePos;

	void FillBuffers(); //G-Buffer Fill Render Pass
	void DrawPointLights(); // Lighting Render Pass
	void CombineBuffers(); // Combination Render Pass

	void DrawVirtualPointLights(); // Extra Lighting Render Pass

	void DrawPointLightsRaymarched(); // Lighting Render Pass with Screen Space Shadows


	void DrawAlphaMeshes(); //Draw transparent meshes after

	// Screenspace reflections
	void RaymarchReflection();
	void ReflectionBlurring();

	//Deferred shadows
	void FillShadowMaps();

	// Screenspace illumination
	void RaymarchLighting();
	void LightingBlurring();
	float IGN(int x, int y);

	// Screen Space Ambient Occlusion
	void SSAOProcess();
	void SSAOBlurring();

	// Edge blurring
	void SobelProcess();
	void SobelBlurring();


	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNodesSolid();
	void DrawNodesAnim();
	void DrawNodesAlpha();
	void DrawNode(SceneNode* n);
	void DrawNodeAnim(AnimatedNode* n);
	void DrawAlphaNode(SceneNode* n);

	SceneNode* root;

	SceneNode* rotatingCube;

	void DrawSkybox();
	Shader* skyboxShader;

	void GenerateScreenTexture(GLuint& into, bool depth = false); //Make a new texture ...

	void GeneratePositionTexture(GLuint& into); //Make a new texture ... to hold viewspace position

	Shader* heightShader; //To process heightmap with multiple textures
	Shader* sceneShader; // Shader to fill our GBuffers
	Shader* pointlightShader; // Shader to calculate lighting
	Shader* combineShader; // Shader to stick it all together
	Shader* meshshader; // Shader for drawing meshes
	Shader* animatedshader; // Shader for animated meshes
	Shader* endshader; // To display final result to quad
	Shader* alphaShader; // for handling transparency
	Shader* blurShader; // for post-processing blur
	Shader* sobelShader; // for post-processing sobel

	//// Raymarch Shaders ////
	Shader* reflectionShader;

	Shader* illuminationShader;

	// Deferred Shadows
	Shader* shadowShader;

	// SSAO Shader
	Shader* SSAOShader;

	// Edge Shader
	Shader* SobelDepthShader;

	// Virtual Point Light
	Shader* virtualPointlightShader;

	// Point Lights with Screen Space Shadows
	Shader* pointlightRaymarchShader; // Shader to calculate lighting

	GLuint processFBO; //The FBO for post processing
	GLuint processColourTex; //The texture for post processing

	GLuint currentAlphaFBO; //The current in-use FBO
	GLuint currentAlphaColourTex; // Current albedo goes here
	GLuint currentAlphaDepthTex; // Current depth goes here

	GLuint alphaFBO; //FBO for our transparency pass
	GLuint alphaColourTex; // Albedo goes here
	GLuint alphaDepthTex; // Depth goes here

	GLuint bufferFBO; //FBO for our G-Buffer pass
	GLuint bufferColourTex; // Albedo goes here
	GLuint bufferNormalTex; // Normals go here
	GLuint bufferDepthTex; // Depth goes here
	//...
	// Global Illumination Part
	GLuint illuminationNoiseTex;

	// Reflection part
	GLuint reflectionBufferTex;

	// SSAO Part
	GLuint SSAOFBO;
	GLuint SSAOTex;
	// noise storage tex
	GLuint SSAONoiseTex;


	// Reflection buffer
	GLuint reflectionFBO;			// FBO for reflection storage
	GLuint reflectionStorageTex;	// Reflection tex goes here

	// Illumination buffer
	GLuint illuminationFBO;			// FBO for illumination storage
	GLuint illuminationStorageTex;	// Illumination tex goes here

	GLuint pointLightFBO; //FBO for our lighting pass
	GLuint lightDiffuseTex; // Store diffuse lighting
	GLuint lightSpecularTex; // Store specular lighting

	HeightMap* heightMap; // Terrain!
	Light* pointLights; // Array of lighting data
	Mesh* sphere; // Light volume
	Mesh* quad; //To draw a full -screen quad
	Mesh* quad_L; //To draw half a screen
	Mesh* quad_R; //To draw half a screen
	Mesh* tree;
	Mesh* cube;
	Camera* camera; //Our usual camera
	Camera* camera_2; //Our usual camera
	Camera* activeCamera; //The currently used camera
	GLuint earthTex;
	GLuint earthBump;

	GLuint grassTex;
	GLuint grassBump;
	GLuint nullBump;
	GLuint texture;
	GLuint glassTex;
	GLuint cubeMap;

	// Reflect
	GLuint reflectStorage;

	GLuint reflectiveTex;
	GLuint unreflectiveTex;

	Mesh* animMesh;

	MeshMaterial* treeMaterial;

	MeshAnimation* anim;

	vector<GLuint> matTextures;
	MeshMaterial* animMaterial;
	Frustum frameFrustum;

	vector <SceneNode*> transparentNodeList;
	vector <SceneNode*> nodeList;
	vector <SceneNode*> animNodeList;


	// Deferred shadowmapping
	vector<vector<GLuint>> shadowFBO;
	vector<vector<GLuint>> shadowMaps;
	Matrix4 shadowProj;
	vector<vector<Matrix4>> shadowTransforms;

	// SSAO Storage
	vector<Vector3> SSAOKernels;
	vector<Vector3> SSAONoise;

	// Edge blurring Storage
	GLuint edgeFBO;
	GLuint edgeStorageTex;

	// Virtual Point Lighting
	vector<Vector3> virtualPointLights; // Array of VPL positions
	vector<Vector3> virtualPointLightsColour; // Array of VPL colour values
	vector<float>	virtualPointLightsRadius; // Radius of lights

	float spinnyTime;

	// Timing

	GLuint queryObject;
	GLuint64 endTime;
	bool timingInProgress = false;

	void startTiming();
	void endTiming();

};