#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
    #include <tool/stb_image.h>
#endif
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <tool/mesh.h>
#include <tool/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>


using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

glm::mat4 ToGlmMatrix(const aiMatrix4x4& value) {
    glm::mat4 result{1.0f};
    result[0][0] = value.a1;
    result[1][0] = value.a2;
    result[2][0] = value.a3;
    result[3][0] = value.a4;
    result[0][1] = value.b1;
    result[1][1] = value.b2;
    result[2][1] = value.b3;
    result[3][1] = value.b4;
    result[0][2] = value.c1;
    result[1][2] = value.c2;
    result[2][2] = value.c3;
    result[3][2] = value.c4;
    result[0][3] = value.d1;
    result[1][3] = value.d2;
    result[2][3] = value.d3;
    result[3][3] = value.d4;
    return result;
}

glm::vec3 ToGlmVec3(const aiVector3D& value) {
	return glm::vec3{
		value.x, value.y, value.z
	};
}

glm::quat ToGlmQuaternion(const aiQuaternion& quat) {
	return glm::quat{
		quat.w, quat.x, quat.y, quat.z
	};
}

struct BoneInfo
{
    glm::mat4 offset;
    glm::mat4 finalTransform{1.0f};
};


class Model
{
public:
	vector<Texture> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
	vector<Mesh> meshes;
	string directory;
	bool gammaCorrection;

	std::map<std::string, unsigned int> boneMapping;
	std::vector<BoneInfo> boneInfos;
	unsigned int boneCount = 0;

	Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
		loadModel(path);
	}

	void Draw(Shader &shader) {
		for (unsigned int i = 0; i < meshes.size(); i++)
			meshes[i].Draw(shader);
	}

	void GetBoneTransforms(float timeInSeconds, std::vector<glm::mat4>& transforms) {
		transforms.resize(boneCount);
		for (auto& boneInfo : boneInfos) {
			boneInfo.finalTransform = glm::mat4(1.0f);
		}
		if (!scene || !scene->HasAnimations() || scene->mAnimations[0]->mDuration <= 0.0) {
			for (std::size_t i = 0; i < boneInfos.size(); ++i) {
				transforms[i] = boneInfos[i].finalTransform;
			}
			return;
		}

		float ticksPerSecond = static_cast<float>(scene->mAnimations[0]->mTicksPerSecond != 0 ? 
			scene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float timeInTicks = timeInSeconds * ticksPerSecond;
		float animationTimeTicks = fmod(timeInTicks, 
			static_cast<float>(scene->mAnimations[0]->mDuration));
		
		
		glm::mat4 identity{1.0f};
		GetBoneTransformsRecursive(animationTimeTicks,
				scene->mRootNode, identity);

		for (int i = 0; i < boneInfos.size(); ++i) {
			transforms[i] = boneInfos[i].finalTransform;
		}
	}

private:
	Assimp::Importer importer;
	const aiScene *scene;
	glm::mat4 rootInverse{1.0f};

	void loadModel(string const &path) {
		directory = path.substr(0, path.find_last_of('/'));
		// read file via ASSIMP
		scene = importer.ReadFile(path, aiProcess_Triangulate 
			| aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
			| aiProcess_JoinIdenticalVertices);
		// check for errors
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		} else {
			cout << "-------\n""scene loaded from ("<< directory<<") :\n";
			// if (scene->HasAnimations()) cout << "  " << scene->mNumAnimations << "\tanimations\n";
			// if (scene->HasCameras()) cout << "  " << scene->mNumCameras << "\tcameras\n";
			// if (scene->HasLights()) cout << "  " << scene->mNumLights << "\tlights\n";
			if (scene->HasMaterials()) cout << "  " << scene->mNumMaterials << "\tmaterials\n";
			if (scene->HasMeshes()) cout << "  " << scene->mNumMeshes << "\tmeshes\n";
			if (scene->HasTextures()) cout << "  " << scene->mNumTextures << "\ttextures\n";
			cout << "-------\n"; 
		}
		// retrieve the directory path of the filepath

		rootInverse = glm::inverse(ToGlmMatrix(scene->mRootNode->mTransformation));

		// process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	// processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode *node, const aiScene *scene) {
		// process each mesh located at the current node
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			// the node object only contains indices to index the actual objects in the scene.
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		// after we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
		// data to fill
		vector<Vertex> vertices;
		vertices.reserve(mesh->mNumVertices);
		vector<unsigned int> indices;
		indices.reserve(mesh->mNumFaces * 3);
		vector<Texture> textures;

		// walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
												// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0]) {
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
				// tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.Tangent = vector;
				// bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.Bitangent = vector;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		// process materials
		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN
		Material mat = loadMaterial(material);

		// 1. diffuse maps
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. specular maps
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		// 3. normal maps
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		// 4. height maps
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		// bones
		if (mesh->HasBones()) {
			
			std::cout << mesh->mName.C_Str() << " : " << mesh->mNumBones << " bones" << std::endl;
			processBones(mesh, vertices);
		}


		// return a mesh object created from the extracted mesh data
		return {vertices, indices, textures, mat};
	} 

	Material loadMaterial(aiMaterial* mat) {
		Material material;
		aiColor3D color;
		float value;
		
		if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
			material.ambient = glm::vec3(color.r, color.g, color.b);
		}
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			material.diffuse = glm::vec3(color.r, color.g, color.b);
		}
		if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
			material.specular = glm::vec3(color.r, color.g, color.b);
		}
		if (mat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
			material.emissive = glm::vec3(color.r, color.g, color.b);
		}
		if (mat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
			material.shininess = value;
		}
		if (mat->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS) {
			material.transparency = 1.0f - value;
		}
		// 注：本 Assimp 版本无 AI_MATKEY_TRANSPARENCYFACTOR，故省略该分支
		if (mat->Get(AI_MATKEY_REFRACTI, value) == AI_SUCCESS) {
			material.ior = value;
		}
		return material;
	}

	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);
			// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++) {
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
					textures.push_back(textures_loaded[j]);
					skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}
			if (!skip) { // if texture hasn't been loaded already, load it
				cout << "  loading texture : " << str.C_Str() << endl;
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture); // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			}
		}
		return textures;
	}

	void processBones(aiMesh* mesh, vector<Vertex>& vertices) {
		for (int i = 0; i < mesh->mNumBones; ++i) {
			processSingleBone(mesh->mBones[i], vertices);
		}
	}

	void processSingleBone(aiBone* bone, vector<Vertex>& vertices) {
		unsigned int boneId = GetBoneId(bone);

		std::cout << "processing bone: " << bone->mName.C_Str() << ' ' << bone->mNumWeights << std::endl;
		for (int i = 0; i < bone->mNumWeights; ++i) {
			auto weight = bone->mWeights[i];
			vertices[weight.mVertexId].AddBoneWeight(boneId, weight.mWeight);
		}
	} 

	unsigned int GetBoneId(aiBone* bone) {
		auto it = boneMapping.find(bone->mName.C_Str());
		if (it != boneMapping.end()) 
			return it->second;
		boneInfos.emplace_back(BoneInfo{ToGlmMatrix(bone->mOffsetMatrix)});
		unsigned int boneId = boneInfos.size() - 1;
		boneMapping.insert_or_assign(bone->mName.C_Str(), boneId);
		++boneCount;
		return boneId;
	}

	void GetBoneTransformsRecursive(float timeInTicks, aiNode* node, const glm::mat4& parentTransfrom) {
		glm::mat4 transform = ToGlmMatrix(node->mTransformation);

		const string name = node->mName.C_Str();
		const aiAnimation* animation = scene->mAnimations[0];
		const aiNodeAnim* nodeAnim = FindNodeAnim(animation, name);

		if (nodeAnim) {
			glm::mat4 result{1.0f};
			glm::vec3 translate = 
				InterpolateTranslation(timeInTicks, animation->mDuration, nodeAnim);
			result = glm::translate(result, translate);
			glm::quat rotate = 
				InterpolateRotation(timeInTicks, animation->mDuration, nodeAnim);
			result = result * glm::mat4_cast(rotate);
			glm::vec3 scale = 
				InterpolateScaling(timeInTicks, animation->mDuration, nodeAnim);
			result = glm::scale(result, scale);
			
			transform = result;
		}

		transform = parentTransfrom * transform;
		
		auto it = boneMapping.find(name);
		if (it != boneMapping.end()) {
			unsigned int index = it->second; 
			boneInfos[index].finalTransform = rootInverse * transform 
				* boneInfos[index].offset;
		}

		for (int i = 0; i < node->mNumChildren; ++i) {
			GetBoneTransformsRecursive(timeInTicks, node->mChildren[i], transform);
		}
	}

	aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const string& nodeName) {
		for (int i = 0; i < animation->mNumChannels; ++i) {
			if (std::string(animation->mChannels[i]->mNodeName.data) == nodeName) {
				return animation->mChannels[i];
			}
		}
		return nullptr;
	}

	glm::vec3 InterpolateScaling(float timeInTicks, float duration, const aiNodeAnim* nodeAnim) {
		const auto* scalingKeys = nodeAnim->mScalingKeys;
		std::size_t keyNum = nodeAnim->mNumScalingKeys;
		if (keyNum == 0) {
			return glm::vec3(1.0f);
		}
		if (keyNum == 1) {
			return ToGlmVec3(scalingKeys[0].mValue);
		}

		if (timeInTicks <= scalingKeys[0].mTime) {
			return ToGlmVec3(scalingKeys[0].mValue);
		}
		std::size_t nextIdx = 1;
		while (nextIdx < keyNum && scalingKeys[nextIdx].mTime < timeInTicks) ++nextIdx;
		if (nextIdx == keyNum) {
			return ToGlmVec3(scalingKeys[keyNum - 1].mValue);
		}
		std::size_t idx = nextIdx - 1;



		glm::vec3 start = ToGlmVec3(scalingKeys[idx].mValue);
		glm::vec3 end = ToGlmVec3(scalingKeys[nextIdx].mValue);
		float startTime = scalingKeys[idx].mTime;
		float endTime = scalingKeys[nextIdx].mTime;
		float deltaTime = timeInTicks - startTime;
		float factor = endTime > startTime ? deltaTime / (endTime - startTime) : 0.0f;
		return glm::mix(start, end, factor);
	}
	
	glm::vec3 InterpolateTranslation(float timeInTicks, float duration, const aiNodeAnim* nodeAnim) {
		const auto* posKeys = nodeAnim->mPositionKeys;
		std::size_t keyNum = nodeAnim->mNumPositionKeys;
		if (keyNum == 0) {
			return glm::vec3(0.0f);
		}
		if (keyNum == 1) {
			return ToGlmVec3(posKeys[0].mValue);
		}

		if (timeInTicks <= posKeys[0].mTime) {
			return ToGlmVec3(posKeys[0].mValue);
		}
		std::size_t nextIdx = 1;
		while (nextIdx < keyNum && posKeys[nextIdx].mTime < timeInTicks) ++nextIdx;
		if (nextIdx == keyNum) {
			return ToGlmVec3(posKeys[keyNum - 1].mValue);
		}
		std::size_t idx = nextIdx - 1;

		glm::vec3 start = ToGlmVec3(posKeys[idx].mValue);
		glm::vec3 end = ToGlmVec3(posKeys[nextIdx].mValue);
		float startTime = posKeys[idx].mTime;
		float endTime = posKeys[nextIdx].mTime;
		float deltaTime = timeInTicks - startTime;
		float factor = endTime > startTime ? deltaTime / (endTime - startTime) : 0.0f;
		return glm::mix(start, end, factor);
	}
	
	glm::quat InterpolateRotation(float timeInTicks, float duration, const aiNodeAnim* nodeAnim) {
		const auto* rotaionKeys = nodeAnim->mRotationKeys;
		std::size_t keyNum = nodeAnim->mNumRotationKeys;
		if (keyNum == 0) {
			return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		}
		if (keyNum == 1) {
			return ToGlmQuaternion(rotaionKeys[0].mValue);
		}

		if (timeInTicks <= rotaionKeys[0].mTime) {
			return ToGlmQuaternion(rotaionKeys[0].mValue);
		}
		std::size_t nextIdx = 1;
		while (nextIdx < keyNum && rotaionKeys[nextIdx].mTime < timeInTicks) ++nextIdx;
		if (nextIdx == keyNum) {
			return ToGlmQuaternion(rotaionKeys[keyNum - 1].mValue);
		}
		std::size_t idx = nextIdx - 1;

		glm::quat start = ToGlmQuaternion(rotaionKeys[idx].mValue);
		glm::quat end = ToGlmQuaternion(rotaionKeys[nextIdx].mValue);
		float startTime = rotaionKeys[idx].mTime;
		float endTime = rotaionKeys[nextIdx].mTime;
		float deltaTime = timeInTicks - startTime;
		float factor = endTime > startTime ? deltaTime / (endTime - startTime) : 0.0f;
		return glm::slerp(start, end, factor);
	}
};

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma) {
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data) {
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else {
		std::cout << "Texture failed to load at path: " << filename << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}




#endif