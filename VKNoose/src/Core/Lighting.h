#pragma once
#include "../Common.h"

#define MAP_WIDTH 6.0f
#define MAP_HEIGHT 2.5f
#define MAP_DEPTH 6.0f
#define POINT_CLOUD_SPACING 0.3f
#define PROPOGATION_GRID_SPACING 0.1f
#define PROPOGATION_WIDTH 50
#define PROPOGATION_HEIGHT 25
#define PROPOGATION_DEPTH 60

struct CloudPoint {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 directLighting = glm::vec3(0);
};

struct ProbePoint {
    glm::vec3 indirectLighting = glm::vec3(0);
    float sampleCount = 0;
};

namespace Lighting {
    void InitPointCloud();
    std::vector<CloudPoint>& GetPointCloud();
    //td::vector<ProbePoint>& GetProbeGrid();
    ProbePoint& GetProbe(int x, int y, int z);

    inline bool _showProbesInstead = false;

    inline int GetProbeGridSize() {
        return PROPOGATION_WIDTH * PROPOGATION_HEIGHT * PROPOGATION_DEPTH;
    }
}