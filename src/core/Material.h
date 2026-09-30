#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace materializr {

struct Material {
    std::string name = "Default";
    glm::vec3 baseColor = glm::vec3(0.7f);
    float roughness = 0.5f;
    float metallic = 0.0f;
    float transmission = 0.0f;
    float ior = 1.45f;
};

class MaterialLibrary {
public:
    MaterialLibrary();

    const Material& get(int index) const;
    const Material& getByName(const std::string& name) const;
    int count() const;
    const std::vector<Material>& getAll() const;

    int addCustom(const Material& mat);

private:
    std::vector<Material> m_materials;
    void initDefaults();
};

} // namespace materializr
