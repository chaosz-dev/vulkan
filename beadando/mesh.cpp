#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "buffer.h"

#define DEBUG 0

class Mesh {
  public:
    Mesh() {}

    Mesh(const std::string &filename, const VkPhysicalDevice &phyDevice, const VkDevice &device, glm::vec3 modelPos) {
        m_vertices = std::vector<float>();
        loadObject(filename.c_str());

        m_bufferInfo =
            BufferInfo::Create(phyDevice, device, m_vertices.size() * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m_bufferInfo.Update(device, m_vertices.data(), m_vertices.size() * sizeof(float));

        m_rotation = {0, 0, 0};
        m_model    = glm::translate(glm::mat4(1.0f), modelPos);
    }

    void destroyResources(const VkDevice &device) { m_bufferInfo.Destroy(device); }

    struct point_t {
        float coordinates[3];
    };

    struct rotation_t {
        uint32_t x;
        uint32_t y;
        uint32_t z;
    };

    void loadObject(const char *filename) {
        std::ifstream file(filename);

        if (!file.is_open() || file.fail()) {
            printf("Error: Could not open obj file\n");
            return;
        }

        std::vector<point_t> points;
        std::string line;

        while (std::getline(file, line)) {
            if (line.find("o ") != std::string::npos) {
                continue;
            }

            if (line.find("v ") != std::string::npos) {
                line.append(" \0");
                line.erase(0, 2);
                point_t point;
                uint32_t idx = 0;
                size_t pos   = 0;
                while ((pos = line.find(" ")) != std::string::npos) {
#if DEBUG == 1
                    printf("%s\n", line.c_str());
#endif
                    point.coordinates[idx++] = std::stof(line.substr(0, pos));
                    line.erase(0, pos + 1);
                }

                points.push_back(point);
                continue;
            }

            if (line.find("f ") != std::string::npos) {
                line.append(" ");
                line.erase(0, 2);
                size_t pos = 0;
                while ((pos = line.find(" ")) != std::string::npos) {
                    uint32_t idx = std::stoi(line.substr(0, pos)) - 1;

                    m_vertices.push_back(points[idx].coordinates[0]);
                    m_vertices.push_back(points[idx].coordinates[1]);
                    m_vertices.push_back(points[idx].coordinates[2]);

                    m_vertices.push_back(0); // u
                    m_vertices.push_back(0); // v
                    m_vertices.push_back(0); // x
                    m_vertices.push_back(0); // y
                    m_vertices.push_back(0); // z

#if DEBUG == 1
                    printf("idx: %d\n", idx);
                    printf("%f ", points[idx].coordinates[0]);
                    printf("%f ", points[idx].coordinates[1]);
                    printf("%f\n", points[idx].coordinates[2]);
#endif

                    line.erase(0, pos + 1);
                }
            }
        }
        file.close();

        printf("Succesfully read %s file\n", filename);
        printf("Vertices: %ld \n", m_vertices.size() / 3);

#if DEBUG == 1
        for (auto &elem : vertices) {
            printf("%f ", elem);
        }
#endif
    }

    glm::mat4 m_model;
    rotation_t m_rotation;
    VkPipeline m_pipeline;
    std::vector<float> m_vertices;
    BufferInfo m_bufferInfo;
};
