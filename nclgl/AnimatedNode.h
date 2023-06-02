#pragma once
#include "..\nclgl\scenenode.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"

class MeshAnimation;
class MeshMaterial;

class AnimatedNode : public SceneNode {
public:
	void initialise();
	void Update(float dt) override;
	void setAnim(const std::string& filename) { anim = new MeshAnimation(filename); };
	void setMat(const std::string& filename);
	int getCurrentFrame() { return currentFrame; };
	MeshAnimation* getAnim() { return anim; };
	vector<GLuint> getMat() { return matTextures; };
protected:
	MeshAnimation* anim;
	MeshMaterial* material;
	vector<GLuint> matTextures;

	int currentFrame;
	float frameTime;
};