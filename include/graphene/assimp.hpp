#pragma once

#include <graphene/renderable.hpp>

namespace graphene::assimp {

std::vector<std::shared_ptr<renderable>>
load_meshes(const std::filesystem::path& file_path, bool smooth,
            std::optional<vec4f_t> color = std::nullopt);

} // graphene::assimp
