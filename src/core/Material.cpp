#include "Material.h"

namespace materializr {

MaterialLibrary::MaterialLibrary()
{
    initDefaults();
}

void MaterialLibrary::initDefaults()
{
    // Steel
    {
        Material m;
        m.name = "Steel";
        m.baseColor = glm::vec3(0.7f, 0.7f, 0.72f);
        m.roughness = 0.3f;
        m.metallic = 0.95f;
        m_materials.push_back(m);
    }
    // Aluminum
    {
        Material m;
        m.name = "Aluminum";
        m.baseColor = glm::vec3(0.85f, 0.85f, 0.87f);
        m.roughness = 0.25f;
        m.metallic = 0.9f;
        m_materials.push_back(m);
    }
    // Copper
    {
        Material m;
        m.name = "Copper";
        m.baseColor = glm::vec3(0.85f, 0.5f, 0.3f);
        m.roughness = 0.3f;
        m.metallic = 0.95f;
        m_materials.push_back(m);
    }
    // Gold
    {
        Material m;
        m.name = "Gold";
        m.baseColor = glm::vec3(0.9f, 0.75f, 0.3f);
        m.roughness = 0.2f;
        m.metallic = 0.95f;
        m_materials.push_back(m);
    }
    // Plastic White
    {
        Material m;
        m.name = "Plastic White";
        m.baseColor = glm::vec3(0.9f, 0.9f, 0.9f);
        m.roughness = 0.5f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
    // Plastic Black
    {
        Material m;
        m.name = "Plastic Black";
        m.baseColor = glm::vec3(0.05f, 0.05f, 0.05f);
        m.roughness = 0.4f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
    // Plastic Red
    {
        Material m;
        m.name = "Plastic Red";
        m.baseColor = glm::vec3(0.8f, 0.1f, 0.1f);
        m.roughness = 0.5f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
    // Wood
    {
        Material m;
        m.name = "Wood";
        m.baseColor = glm::vec3(0.55f, 0.35f, 0.15f);
        m.roughness = 0.7f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
    // Glass
    {
        Material m;
        m.name = "Glass";
        m.baseColor = glm::vec3(0.95f, 0.95f, 0.97f);
        m.roughness = 0.05f;
        m.metallic = 0.0f;
        m.transmission = 0.9f;
        m.ior = 1.5f;
        m_materials.push_back(m);
    }
    // Rubber
    {
        Material m;
        m.name = "Rubber";
        m.baseColor = glm::vec3(0.15f, 0.15f, 0.15f);
        m.roughness = 0.9f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
    // Ceramic
    {
        Material m;
        m.name = "Ceramic";
        m.baseColor = glm::vec3(0.9f, 0.88f, 0.82f);
        m.roughness = 0.3f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
    // Concrete
    {
        Material m;
        m.name = "Concrete";
        m.baseColor = glm::vec3(0.6f, 0.58f, 0.55f);
        m.roughness = 0.95f;
        m.metallic = 0.0f;
        m_materials.push_back(m);
    }
}

const Material& MaterialLibrary::get(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_materials.size())) {
        return m_materials[index];
    }
    static Material fallback;
    return fallback;
}

const Material& MaterialLibrary::getByName(const std::string& name) const
{
    for (const auto& mat : m_materials) {
        if (mat.name == name) {
            return mat;
        }
    }
    static Material fallback;
    return fallback;
}

int MaterialLibrary::count() const
{
    return static_cast<int>(m_materials.size());
}

const std::vector<Material>& MaterialLibrary::getAll() const
{
    return m_materials;
}

int MaterialLibrary::addCustom(const Material& mat)
{
    m_materials.push_back(mat);
    return static_cast<int>(m_materials.size()) - 1;
}

} // namespace materializr
