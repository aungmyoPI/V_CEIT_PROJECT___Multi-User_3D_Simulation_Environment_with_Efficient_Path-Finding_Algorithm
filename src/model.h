#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <mesh.h>
#include <shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>

#include <assimp_glm_helpers.h>
#include <animdata.h>

using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, const aiScene* scene = nullptr, bool gamma = false);
unsigned int CreateDefaultTexture(unsigned char r = 255, unsigned char g = 255, unsigned char b = 255);
unsigned int FindFallbackDiffuseTexture(const string &directory, const aiScene* scene = nullptr);

enum ModelType {
    ENV,
    CHARR
};

class Model
{
public:
    // model data
    vector<Texture> textures_loaded;
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
    unsigned int directoryFallbackDiffuse = 0;

    Model(string const &path, ModelType type = ENV, bool gamma = false) : gammaCorrection(gamma)
        {
            if (type == CHARR) {
                loadMixamoModel(path);
            } else {
                loadEnvironmentModel(path);
            }
        }

    // draws the model
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }


private:
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

        void loadEnvironmentModel(string const &path)
        {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

            if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
                return;
            }
            directory = path.substr(0, path.find_last_of("/\\"));
            processNode(scene->mRootNode, scene);
        }

        void loadMixamoModel(string const &path)
        {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

            if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
                return;
            }
            directory = path.substr(0, path.find_last_of("/\\"));
            processNode(scene->mRootNode, scene);
        }

    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    void setVertexBoneDataToDefault(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            setVertexBoneDataToDefault(vertex);
            glm::vec3 vector;
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;

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
        ExtractBoneWeightForVertices(vertices, mesh, scene);

        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. diffuse
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        if (diffuseMaps.empty())
            diffuseMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
        if (diffuseMaps.empty())
            diffuseMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_diffuse", scene);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
        if (normalMaps.empty())
            normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height", scene);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        if (textures.empty())
        {
            if (directoryFallbackDiffuse == 0)
                directoryFallbackDiffuse = FindFallbackDiffuseTexture(this->directory, scene);

            Texture fallback;
            fallback.id = directoryFallbackDiffuse;
            fallback.type = "texture_diffuse";
            fallback.path = "__directory_fallback__";
            textures.push_back(fallback);
        }
        return Mesh(vertices, indices, textures);
    }

    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_BoneIDs[i] < 0)
			{
				vertex.m_Weights[i] = weight;
				vertex.m_BoneIDs[i] = boneID;
				break;
			}
		}
	}


	void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
		auto& boneInfoMap = m_BoneInfoMap;
		int& boneCount = m_BoneCounter;

		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = boneCount;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				boneInfoMap[boneName] = newBoneInfo;
				boneID = boneCount;
				boneCount++;
			}
			else
			{
				boneID = boneInfoMap[boneName].id;
			}
			assert(boneID != -1);
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId <= vertices.size());
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}

    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName, const aiScene *scene)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            if(!skip)
            {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory, scene);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};


static unsigned int loadTextureFromData(unsigned char* data, int width, int height, int nrComponents)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    GLenum format;
    if (nrComponents == 1)
        format = GL_RED;
    else if (nrComponents == 3)
        format = GL_RGB;
    else if (nrComponents == 4)
        format = GL_RGBA;
    else
        return 0;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

unsigned int CreateDefaultTexture(unsigned char r, unsigned char g, unsigned char b)
{
    unsigned char data[] = { r, g, b, 255 };
    return loadTextureFromData(data, 1, 1, 4);
}

static string toLowerCopy(string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

static bool isImageFilename(const string& filename)
{
    const string lower = toLowerCopy(filename);
    return lower.find(".jpg") != string::npos
        || lower.find(".jpeg") != string::npos
        || lower.find(".png") != string::npos;
}

static bool isLikelyAlbedoMap(const string& filename)
{
    const string lower = toLowerCopy(filename);
    return lower.find("albedo") != string::npos
        || lower.find("diffuse") != string::npos
        || lower.find("basecolor") != string::npos
        || lower.find("base_color") != string::npos;
}

static bool isLikelyNonColorMap(const string& filename)
{
    const string lower = toLowerCopy(filename);
    return lower.find("normal") != string::npos
        || lower.find("metallic") != string::npos
        || lower.find("roughness") != string::npos
        || lower.find("_ao") != string::npos
        || lower.find("ambientocclusion") != string::npos;
}

unsigned int FindFallbackDiffuseTexture(const string &directory, const aiScene* scene)
{
    if (scene && scene->mNumTextures > 0)
    {
        const unsigned int embeddedId = TextureFromFile("*0", directory, scene);
        if (embeddedId != 0)
            return embeddedId;
    }

    std::error_code ec;
    if (std::filesystem::exists(directory, ec))
    {
        string firstColorMap;
        for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
        {
            if (!entry.is_regular_file())
                continue;

            const string filename = entry.path().filename().string();
            if (!isImageFilename(filename) || isLikelyNonColorMap(filename))
                continue;

            if (isLikelyAlbedoMap(filename))
                return TextureFromFile(filename.c_str(), directory, scene);

            if (firstColorMap.empty())
                firstColorMap = filename;
        }

        if (!firstColorMap.empty())
            return TextureFromFile(firstColorMap.c_str(), directory, scene);
    }

    std::cout << "No diffuse texture found for model directory: " << directory << std::endl;
    return CreateDefaultTexture();
}

unsigned int TextureFromFile(const char *path, const string &directory, const aiScene* scene, bool gamma)
{
    string filename = string(path);

    // 1. Check for Assimp Embedded Texture (e.g., "*0", "*1")
    if (!filename.empty() && filename[0] == '*')
    {
        if (scene)
        {
            const aiTexture* embedded = scene->GetEmbeddedTexture(path);
            if (embedded)
            {
                unsigned char* image_data = nullptr;
                int width = 0, height = 0, nrComponents = 0;

                // Check if compressed (mHeight == 0 means PNG/JPG buffer in pcData)
                if (embedded->mHeight == 0)
                {
                    image_data = stbi_load_from_memory(
                        reinterpret_cast<const unsigned char*>(embedded->pcData),
                        embedded->mWidth,
                        &width, &height, &nrComponents, 0);
                }
                else
                {
                    // Uncompressed raw RGBA8888 data
                    image_data = reinterpret_cast<unsigned char*>(embedded->pcData);
                    width = embedded->mWidth;
                    height = embedded->mHeight;
                    nrComponents = 4;
                }

                if (image_data)
                {
                    unsigned int textureID = loadTextureFromData(image_data, width, height, nrComponents);

                    // Only free if stb_image allocated compressed memory
                    if (embedded->mHeight == 0)
                    {
                        stbi_image_free(image_data);
                    }
                    return textureID;
                }
            }
        }
        std::cout << "Failed to parse embedded texture: " << path << std::endl;
        return CreateDefaultTexture();
    }

    // 2. Fallback: Try extracting base filename for loose disk files
    std::string baseFilename = std::filesystem::path(filename).filename().string();
    vector<string> candidates = {
        directory + "/" + baseFilename,
        directory + "\\" + baseFilename,
        directory + "/" + filename,
        filename
    };

    for (const string& candidate : candidates)
    {
        int width, height, nrComponents;
        unsigned char *data = stbi_load(candidate.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            unsigned int textureID = loadTextureFromData(data, width, height, nrComponents);
            stbi_image_free(data);
            return textureID;
        }
    }

    std::cout << "Texture failed to load at path: " << path << std::endl;
    return CreateDefaultTexture();
}


#endif
