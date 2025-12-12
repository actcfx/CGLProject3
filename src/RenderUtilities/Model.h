#ifndef MODEL_H
#define MODEL_H


#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "Mesh.h"
#include "Shader.h"

class Model 
{
    public:
        explicit Model(const std::string& path)
        {
            loadModel(path);
        }
        
        void Draw(Shader &shader)
        {
            for(unsigned int i = 0; i < meshes.size(); i++)
                meshes[i].Draw(shader);
        } 	
    private:
        // model data
        std::vector<Texture> textures_loaded;
        std::vector<Mesh> meshes;
        std::string directory;

        void loadModel(const std::string& path)
        {
            Assimp::Importer import;
            const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);	
            
            if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
            {
                std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
                return;
            }
            directory = path.substr(0, path.find_last_of('/'));

            processNode(scene->mRootNode, scene);
        }  

        static glm::mat4 aiToGlm(const aiMatrix4x4& m)
        {
            glm::mat4 result;
            result[0][0] = m.a1; result[1][0] = m.a2; result[2][0] = m.a3; result[3][0] = m.a4;
            result[0][1] = m.b1; result[1][1] = m.b2; result[2][1] = m.b3; result[3][1] = m.b4;
            result[0][2] = m.c1; result[1][2] = m.c2; result[2][2] = m.c3; result[3][2] = m.c4;
            result[0][3] = m.d1; result[1][3] = m.d2; result[2][3] = m.d3; result[3][3] = m.d4;
            return result;
        }

        void processNode(aiNode *node, const aiScene *scene,
                         const glm::mat4& parentTransform = glm::mat4(1.0f))
        {
            glm::mat4 currentTransform = parentTransform * aiToGlm(node->mTransformation);
            // process all the node's meshes (if any)
            for(unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
                meshes.push_back(processMesh(mesh, scene, currentTransform));			
            }
            // then do the same for each of its children
            for(unsigned int i = 0; i < node->mNumChildren; i++)
            {
                processNode(node->mChildren[i], scene, currentTransform);
            }
        }  
        
        Mesh processMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4& transform)
        {
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Texture> textures;
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            
            for(unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                glm::vec3 vector;
                Vertex vertex;
                // process vertex positions, normals and texture coordinates    
                glm::vec4 position(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                    mesh->mVertices[i].z, 1.0f);
                position = transform * position;
                vertex.Position = glm::vec3(position);
                
                // process indices
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = glm::normalize(normalMatrix * vector);
                
                
                // process material
                if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
                {
                    glm::vec2 vec;
                    vec.x = mesh->mTextureCoords[0][i].x; 
                    vec.y = mesh->mTextureCoords[0][i].y;
                    vertex.TexCoords = vec;
                }
                else
                    vertex.TexCoords = glm::vec2(0.0f, 0.0f); 

                vertices.push_back(vertex);
            }

            for(unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                for(unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }  

            if(mesh->mMaterialIndex >= 0)
            {
                aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
                std::vector<Texture> diffuseMaps = loadMaterialTextures(material, 
                                                    aiTextureType_DIFFUSE, "texture_diffuse", scene);
                textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
                std::vector<Texture> specularMaps = loadMaterialTextures(material, 
                                                    aiTextureType_SPECULAR, "texture_specular", scene);
                textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            }  

            return Mesh(vertices, indices, textures);
        }  

        std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName,
                                                 const aiScene* scene)
        {
            std::vector<Texture> textures;
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
                {   // if texture hasn't been loaded already, load it
                    Texture texture;
                    texture.id = TextureFromFile(str.C_Str(), directory, scene);
                    texture.type = typeName;
                    texture.path = str.C_Str();
                    textures.push_back(texture);
                    textures_loaded.push_back(texture); // add to loaded textures
                }
            }
            return textures;
        }  

        unsigned int TextureFromFile(const char *path, const std::string &directory, const aiScene* scene)
        {
            std::string relativePath = path ? std::string(path) : std::string();
            std::string filename = relativePath;
            std::replace(filename.begin(), filename.end(), '\\', '/');

            unsigned int textureID;
            glGenTextures(1, &textureID);

            int width = 0;
            int height = 0;
            int nrComponents = 0;
            unsigned char *data = nullptr;
            bool shouldFree = false;

            bool attemptedEmbedded = false;
            if (!relativePath.empty() && relativePath[0] == '*' && scene)
            {
                const aiTexture* embedded = scene->GetEmbeddedTexture(relativePath.c_str());
                if (embedded)
                {
                    attemptedEmbedded = true;
                    if (embedded->mHeight == 0)
                    {
                        // Compressed data (e.g., PNG) stored in mWidth bytes
                        unsigned char* compressed = reinterpret_cast<unsigned char*>(embedded->pcData);
                        data = stbi_load_from_memory(compressed, embedded->mWidth, &width, &height,
                                                     &nrComponents, 0);
                        shouldFree = data != nullptr;
                    }
                    else
                    {
                        // Raw image data (width x height) in BGRA 8-bit texels
                        width = embedded->mWidth;
                        height = embedded->mHeight;
                        nrComponents = 4;
                        data = reinterpret_cast<unsigned char*>(embedded->pcData);
                    }
                }
            }

            if (!data)
            {
                std::string resolved = resolveTexturePath(directory, filename);
                data = stbi_load(resolved.c_str(), &width, &height, &nrComponents, 0);
                if (!data)
                {
                    std::cout << "Texture failed to load at path: " << resolved;
                    if (attemptedEmbedded)
                        std::cout << " (embedded fallback also failed)";
                    std::cout << std::endl;
                }
                else
                {
                    shouldFree = true;
                }
            }

            if (data)
            {
                GLenum format = GL_RGB;
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

                if (shouldFree)
                {
                    stbi_image_free(data);
                }
            }

            return textureID;
        }

        static bool fileExists(const std::string& path)
        {
            FILE* f = std::fopen(path.c_str(), "rb");
            if (f)
            {
                std::fclose(f);
                return true;
            }
            return false;
        }

        static std::string resolveTexturePath(const std::string& directory, const std::string& relative)
        {
            if (relative.empty())
                return directory;

            std::string base = directory;
            while (true)
            {
                std::string candidate = base.empty() ? relative : base + '/' + relative;
                if (fileExists(candidate))
                    return candidate;

                if (base.empty())
                    break;

                size_t slashPos = base.find_last_of('/');
                if (slashPos == std::string::npos)
                {
                    base.clear();
                }
                else
                {
                    base = base.substr(0, slashPos);
                }
            }

            if (fileExists(relative))
                return relative;

            return directory + '/' + relative;
        }
};

#endif