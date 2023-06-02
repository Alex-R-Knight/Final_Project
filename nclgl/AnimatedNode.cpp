#include "../nclgl/AnimatedNode.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"

void AnimatedNode::initialise() {
	currentFrame = 0;
	frameTime = 0.0f;
}

void AnimatedNode::setMat(const std::string& filename) {
	material = new MeshMaterial(filename);

	for (int i = 0; i < mesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}
}

void AnimatedNode::Update(float dt) {
	if (parent) { //This node has a parent ...
		worldTransform = parent->GetWorldTransform() * transform;
	}
	else { //Root node , world transform is local transform!
		worldTransform = transform;
	}
	for (vector <SceneNode*>::iterator i = children.begin();
		i != children.end(); ++i) {
		(*i)->Update(dt);
	}

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
}