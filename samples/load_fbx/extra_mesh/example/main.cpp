#define GLFW_INCLUDE_NONE

#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast_beauty.hpp>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include "debug.h"

namespace Engine{
    //顶点
    struct Vertex{
        glm::vec3 position_;
        glm::vec4 color_;
        glm::vec2 uv_;
    };

    //Mesh文件头
    struct MeshFileHead{
        char type_[4];//文件类型文件头
        char name_[32];//名字
        unsigned short vertex_num_;//顶点个数
        unsigned short vertex_index_num_;//索引个数
    };

    //Mesh文件
    struct MeshFile{
        MeshFileHead head_;
        Vertex *vertex_;
        unsigned short *index_;

        MeshFile(){
            vertex_ = nullptr;
            index_ = nullptr;
        }

        ~MeshFile(){
            if(vertex_ != nullptr){
                delete [] vertex_;
                vertex_ = nullptr;
            }
            if(index_ != nullptr){
                delete [] index_;
                index_ = nullptr;
            }
        }

        // 写入文件
        void Write(const char* filePath){
            std::ofstream file(filePath, std::ios::binary);
            if(file.is_open()){
                file.write(reinterpret_cast<char*>(&head_), sizeof(head_));
                file.write(reinterpret_cast<char*>(vertex_), sizeof(Vertex) * head_.vertex_num_);
                file.write(reinterpret_cast<char*>(index_), sizeof(unsigned short) * head_.vertex_index_num_);
                file.close();
            }
        }
    };
}



int main(void){
    Debug::Init();

    //实例化Assimp Importer
    Assimp::Importer importer;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile( "../data/model/assimp_extra.fbx",aiProcess_CalcTangentSpace|aiProcess_Triangulate|aiProcess_JoinIdenticalVertices|aiProcess_SortByPType);

    // 读取文件失败
    if (nullptr == scene) {
        DEBUG_LOG_ERROR(importer.GetErrorString());
        return false;
    }


//    for (int i = 0; i < scene->mMetaData->mNumProperties; ++i) {
//        DEBUG_LOG_INFO("MetaData: {}", scene->mMetaData->mKeys[i].C_Str());
//
//        if(scene->mMetaData->mValues[i].mType == AI_AISTRING){
//            aiString value;
//            scene->mMetaData->Get<aiString>(scene->mMetaData->mKeys[i], value);
//            DEBUG_LOG_INFO("MetaData: {}", value.C_Str());
//        } else if(scene->mMetaData->mValues[i].mType == AI_INT32){
//            int value;
//            scene->mMetaData->Get<int>(scene->mMetaData->mKeys[i], value);
//            DEBUG_LOG_INFO("MetaData: {}", value);
//        } else if(scene->mMetaData->mValues[i].mType == AI_UINT64){
//            unsigned long long value;
//            scene->mMetaData->Get<unsigned long long>(scene->mMetaData->mKeys[i], value);
//            DEBUG_LOG_INFO("MetaData: {}", value);
//        } else if(scene->mMetaData->mValues[i].mType == AI_FLOAT){
//            float value;
//            scene->mMetaData->Get<float>(scene->mMetaData->mKeys[i], value);
//            DEBUG_LOG_INFO("MetaData: {}", value);
//        } else if(scene->mMetaData->mValues[i].mType == AI_BOOL){
//            bool value;
//            scene->mMetaData->Get<bool>(scene->mMetaData->mKeys[i], value);
//            DEBUG_LOG_INFO("MetaData: {}", value);
//        }
//    }

    // 获取向上的轴属性，0表示右 1表示上 2表示向前
    int upAxis = 0;
    scene->mMetaData->Get<int>("UpAxis", upAxis);
    int upAxisSign = 0;
    scene->mMetaData->Get<int>("upAxisSign", upAxisSign);
    int frontAxis = 0;
    scene->mMetaData->Get<int>("frontAxis", frontAxis);
    int frontAxisSign = 0;
    scene->mMetaData->Get<int>("frontAxisSign", frontAxisSign);
    int coordAxis = 0;
    scene->mMetaData->Get<int>("coordAxis", coordAxis);
    int coordAxisSign = 0;
    scene->mMetaData->Get<int>("coordAxisSign", coordAxisSign);

    aiVector3D upVec = upAxis == 0 ? aiVector3D(upAxisSign,0,0) : upAxis == 1 ? aiVector3D(0, upAxisSign,0) : aiVector3D(0, 0, upAxisSign);
    aiVector3D forwardVec = frontAxis == 0 ? aiVector3D(frontAxisSign, 0, 0) : frontAxis == 1 ? aiVector3D(0, frontAxisSign, 0) : aiVector3D(0, 0, frontAxisSign);
    aiVector3D rightVec = coordAxis == 0 ? aiVector3D(coordAxisSign, 0, 0) : coordAxis == 1 ? aiVector3D(0, coordAxisSign, 0) : aiVector3D(0, 0, coordAxisSign);
    aiMatrix4x4 mat(rightVec.x, rightVec.y, rightVec.z, 0.0f,
                    upVec.x, upVec.y, upVec.z, 0.0f,
                    forwardVec.x, forwardVec.y, forwardVec.z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);

    scene->mRootNode->mTransformation=mat * scene->mRootNode->mTransformation;

    // 遍历Material
    for(unsigned int i = 0; i < scene->mNumMaterials; i++){
        aiMaterial* material = scene->mMaterials[i];
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        DEBUG_LOG_INFO("Material name: {}", name.C_Str());

        // 遍历 Texture
        for(unsigned int j = 0; j < material->GetTextureCount(aiTextureType_DIFFUSE); j++){
            aiString path;
            material->GetTexture(aiTextureType_DIFFUSE, j, &path);
            DEBUG_LOG_INFO("Texture path: {}", path.C_Str());

            // 加载EmbeddedTexture
            auto texture = scene->GetEmbeddedTexture(path.C_Str());
            if(texture != nullptr){
                DEBUG_LOG_INFO("EmbeddedTexture width:{} height:{}" , texture->mWidth, texture->mHeight);
                // 将EmbeddedTexture写入文件
                std::ofstream texture_file(texture->mFilename.C_Str(), std::ios::binary);
                if(texture_file.is_open()){
                    texture_file.write(reinterpret_cast<char*>(texture->pcData), texture->mWidth * texture->mHeight * 4);
                    texture_file.close();
                }
            }else{
                DEBUG_LOG_ERROR("Texture {} is not EmbeddedTexture",path.C_Str());
            }
        }
    }

    // 遍历Mesh，一个FBX里面可能有多个Mesh。
    for (int i = 0; i < scene->mNumMeshes; ++i) {
        Engine::MeshFile mesh_file;

        aiMesh* mesh = scene->mMeshes[i]; // 获取Mesh

        DEBUG_LOG_INFO("Mesh {} has {} vertices",mesh->mName.C_Str() ,mesh->mNumVertices);

        // 构造引擎Mesh结构
        strcpy(mesh_file.head_.type_,"Mesh");
        strcpy(mesh_file.head_.name_,mesh->mName.C_Str());
        mesh_file.head_.vertex_num_ = mesh->mNumVertices;
        mesh_file.head_.vertex_index_num_ = mesh->mNumFaces * 3;
        mesh_file.vertex_ = new Engine::Vertex[mesh_file.head_.vertex_num_];
        mesh_file.index_ = new unsigned short[mesh_file.head_.vertex_index_num_];

        // 遍历顶点
        for (int j = 0; j < mesh->mNumVertices; ++j) {
            // 顶点坐标
            aiVector3D& v = mesh->mVertices[j];
//            DEBUG_LOG_INFO("Vertex {} has position {}",j ,glm::to_string(glm::vec3(v.x, v.y, v.z)));

            // 顶点颜色
            glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
            if(mesh->mColors!=nullptr && mesh->mColors[0]!=nullptr){
                aiColor4D& c = mesh->mColors[0][j];//一个顶点有多套顶点颜色，暂时只使用第一套。
                color = glm::vec4(c.r, c.g, c.b, c.a);
//                DEBUG_LOG_INFO("Vertex {} has color {}",j ,glm::to_string(color));
            }

            // 顶点法线
            aiVector3D& n = mesh->mNormals[j];//暂时不使用法线

            // 顶点纹理坐标
            aiVector3D& t = mesh->mTextureCoords[0][j];
//            DEBUG_LOG_INFO("Vertex {} has texture coordinate {}",j ,glm::to_string(glm::vec2(t.x, t.y)));

            mesh_file.vertex_[j].position_ = glm::vec3(v.x, v.y, v.z);
            mesh_file.vertex_[j].color_ = color;
            mesh_file.vertex_[j].uv_ = glm::vec2(t.x, t.y);
        }

        // 获取模型的索引数据
        DEBUG_LOG_INFO("Mesh {} has {} indices",i ,mesh->mNumFaces);
        for (int j = 0; j < mesh->mNumFaces; ++j) {
            aiFace& face = mesh->mFaces[j];
//            DEBUG_LOG_INFO("Face {} has {} indices",j ,face.mNumIndices);
            if(face.mNumIndices!=3){
//                DEBUG_LOG_ERROR("Face {} has {} indices",j ,face.mNumIndices);
                assert(false);
            }
            for (int k = 0; k < face.mNumIndices; ++k) {
//                DEBUG_LOG_INFO("  Index {} is {}",k ,face.mIndices[k]);
                mesh_file.index_[j * 3 + k] = face.mIndices[k];
            }
        }

        // 写入文件
        mesh_file.Write(fmt::format("../data/model/assimp_extra_{}.mesh", mesh_file.head_.name_).c_str());
    }

    DEBUG_LOG_INFO("extra mesh success");

    return 0;
}


