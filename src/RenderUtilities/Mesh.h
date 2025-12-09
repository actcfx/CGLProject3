#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "RenderUtilities/Shader.h"

#define MAX_BONE_INFLUENCE 4

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};

class Mesh {
    public:
        // mesh data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture>      textures;

        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
            this->vertices = vertices;
            this->indices = indices;
            this->textures = textures;

            setupMesh();
        }   

        void Draw(Shader &shader) {
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            for(unsigned int i = 0; i < textures.size(); i++) {
                glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
                // retrieve texture number (the N in diffuse_textureN)
                std::string number;
                std::string name = textures[i].type;
                if(name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if(name == "texture_specular")
                    number = std::to_string(specularNr++);

                shader.setInt(("material." + name + number).c_str(), i);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }
            glActiveTexture(GL_TEXTURE0);

            // draw mesh
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }  
    private:
        //  render data
        unsigned int VAO, VBO, EBO;

        void setupMesh() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
        
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                        &indices[0], GL_STATIC_DRAW);

            // vertex positions
            glEnableVertexAttribArray(0);	
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            // vertex normals
            glEnableVertexAttribArray(1);	
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
            // vertex texture coords
            glEnableVertexAttribArray(2);	
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

            glBindVertexArray(0);
        }  
};  
#endif