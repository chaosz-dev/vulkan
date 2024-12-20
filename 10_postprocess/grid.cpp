#include "grid.h"

#include <cstdio>

#include <vulkan/vulkan_core.h>


static std::vector<float> buildGrid(float width, float height, uint32_t count) {
    // Output format: { x, y, z, u, v }
    std::vector<float> result;

    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    for (uint32_t y = 0; y <= count; y++) {
        for (uint32_t x = 0; x <= count; x++) {
            float vertex[] = {
                // position
                (float)x / count * width - halfWidth,
                (float)y / count * height - halfHeight,
                0.0f,
                // uv
                (float)x / count,
                (float)y / count,
                // normal
                0.0f,
                0.0f,
                -1.0f,
            };

            result.insert(result.end(), vertex, vertex + (3 + 2 + 3));
        }
    }

    return result;
}


static std::vector<uint32_t> buildIndexList(uint32_t splitCount) {

    std::vector<uint32_t> indexList;

    for (uint32_t y = 0; y < splitCount; y++) {
        for (uint32_t x = 0; x < splitCount; x++) {
            uint32_t row = y * (splitCount + 1);
            uint32_t rowNext = (y + 1) * (splitCount + 1);

            uint32_t triangleIndices[] = {
                // triangle 1
                row + x,  row + x + 1, rowNext + x + 1,

                // triangle 2
                row + x, rowNext + x + 1, rowNext + x,
            };

            indexList.insert(indexList.end(), triangleIndices, triangleIndices + 6);
        }
    }

    return indexList;
}


Grid::Grid(float width, float height, uint32_t count)
    : vertices(buildGrid(width, height, count))
    , vertexCount(vertices.size() / 5)
    , indices(buildIndexList(count)) {
}

void Grid::dump() {
    printf("Vertices: (count: %u)\n" , vertexCount);
    for (size_t idx = 0; idx < vertices.size(); idx++) {
        printf("%-3.2f, ", vertices[idx]);
        if ((idx + 1) % 5 == 0) {
            printf("\n");
        }
    }

    printf("Indices: (count: %lu)\n", indices.size());
    for (size_t idx = 0; idx < indices.size(); idx++) {
        printf("%3u, ", indices[idx]);
        if ((idx + 1) % 3 == 0) {
            printf("\n");
        }
    }
}

void Grid::BuildVertices(
    const VkPhysicalDevice  phyDevice,
    const VkDevice          device) {

    vertexInfo = BufferInfo::Create(phyDevice, device, vertexSize(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vertexInfo.Update(device, vertices.data(), vertexSize());

    indexInfo = BufferInfo::Create(phyDevice, device, indexSize(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    indexInfo.Update(device, indices.data(), indexSize());
}

void Grid::Destroy(const VkDevice device) {
    vertexInfo.Destroy(device);
    indexInfo.Destroy(device);
}
