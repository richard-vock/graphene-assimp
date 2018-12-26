#include <graphene/assimp.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace graphene::assimp {

namespace detail {

class mesh : public renderable
{
public:
    mesh(aiMesh* mesh, std::optional<vec4f_t> color)
    {
        uint32_t vertex_count = mesh->mNumVertices;
        mat_ = data_matrix_t(vertex_count, 10);
        for (uint32_t i = 0; i < vertex_count; ++i) {
            // pos
            aiVector3D pos = mesh->mVertices[i];
            mat_(i, 0) = pos.x;
            mat_(i, 1) = pos.y;
            mat_(i, 2) = pos.z;
            // nrm
            aiVector3D nrm = mesh->mNormals[i];
            mat_(i, 3) = nrm.x;
            mat_(i, 4) = nrm.y;
            mat_(i, 5) = nrm.z;
            // col
            vec4f_t col = vec4f_t::Ones();
            if (color) {
                col = *color;
            }
            mat_(i, 6) = pack_rgba(col);
            // uv
            mat_.block<1, 2>(i, 7) = vec2f_t::Zero();
            // curv
            mat_(i, 9) = 0.f;
        }

        uint32_t face_count = mesh->mNumFaces;
        indices_.resize(3 * face_count);
        for (uint32_t i = 0; i < face_count; ++i) {
            indices_[i * 3 + 0] = mesh->mFaces[i].mIndices[0];
            indices_[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
            indices_[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
        }
    }

    virtual ~mesh() = default;

    virtual std::optional<std::vector<uint8_t>>
    texture() const
    {
        return std::nullopt;
    }

    virtual vec2i_t
    texture_size() const
    {
        return vec2i_t::Zero();
    }

    virtual render_mode_t
    render_mode() const
    {
        return render_mode_t::triangles;
    }

    virtual bool
    shaded() const
    {
        return true;
    }

    virtual data_matrix_t
    data_matrix() const
    {
        return mat_;
    }

    virtual std::vector<uint32_t>
    vertex_indices() const
    {
        return indices_;
    }

protected:
    data_matrix_t mat_;
    std::vector<uint32_t> indices_;
};

}  // namespace detail

std::vector<std::shared_ptr<renderable>>
load_meshes(const std::filesystem::path& file_path, bool smooth,
            std::optional<vec4f_t> color)
{
    Assimp::Importer importer;
    int process = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices;
    if (smooth) {
        process |= aiProcess_GenSmoothNormals;
    } else {
        process |= aiProcess_GenNormals;
    }
    const aiScene* scene = importer.ReadFile(file_path.string(), process);
    terminate_unless(scene != nullptr, "Unable to read/process input file");

    // assimp uses OpenGL coordinate system convention
    // graphene however does not
    // clang-format off
    mat4f_t opengl_to_math;
    opengl_to_math <<  1.f, 0.f, 0.f, 0.f,
                       0.f, 0.f, -1.f, 0.f,
                       0.f, 1.f, 0.f, 0.f,
                       0.f, 0.f, 0.f, 1.f;
    // clang-format on

    std::vector<std::shared_ptr<renderable>> objects;
    std::function<void(aiNode*, mat4f_t)> traverse;
    traverse = [&](aiNode* node, mat4f_t t) {
        // convert and concatenate transformation
        // clang-format off
        aiMatrix4x4 aim = node->mTransformation;
        mat4f_t new_t;
        new_t <<
            aim.a1, aim.a2, aim.a3, aim.a4,
            aim.b1, aim.b2, aim.b3, aim.b4,
            aim.c1, aim.c2, aim.c3, aim.c4,
            aim.d1, aim.d2, aim.d3, aim.d4;
        t = new_t * t;
        // clang-format on

        aiMatrix4x4 local_t;
        if (node->mNumMeshes > 0) {
            for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
                uint32_t mesh_idx = node->mMeshes[i];
                auto m = std::make_shared<detail::mesh>(
                    scene->mMeshes[mesh_idx], color);
                m->set_transform(t);
                objects.push_back(m);
            }
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i) {
            traverse(node->mChildren[i], t);
        }
    };

    traverse(scene->mRootNode, mat4f_t::Identity());
    return objects;
}

}  // namespace graphene::assimp
