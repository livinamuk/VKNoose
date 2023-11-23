#include "Lighting.h"
#include "../Game/Scene.h"


std::vector<CloudPoint> _pointCloud;
ProbePoint _probeGrid[PROPOGATION_WIDTH][PROPOGATION_HEIGHT][PROPOGATION_DEPTH];


void Lighting::InitPointCloud() {

    _pointCloud.clear();

	for (Wall& wall : Scene::GetWalls()) {
        glm::vec3 dir = glm::normalize(wall._end - wall._begin);
        float wallLength = distance(wall._begin, wall._end);;
        for (float x = POINT_CLOUD_SPACING * 0.5f; x < wallLength; x += POINT_CLOUD_SPACING) {
            glm::vec3 pos = wall._begin + (dir * x);
            for (float y = POINT_CLOUD_SPACING * 0.5f; y < wall._height; y += POINT_CLOUD_SPACING) {
                CloudPoint cloudPoint;
                cloudPoint.position = pos;
                cloudPoint.position.y = wall._begin.y + y;
                glm::vec3 wallVector = glm::normalize(wall._end - wall._begin);
                cloudPoint.normal = glm::vec3(-wallVector.z, 0, wallVector.x);                
                _pointCloud.push_back(cloudPoint);
            }
        }
	}

    // Floor and ceiling
    for (float x = -3.0f; x < 3; x += POINT_CLOUD_SPACING) {
        for (float z = -3.0f; z < 3; z += POINT_CLOUD_SPACING) {
            CloudPoint cloudPoint;
            cloudPoint.position = glm::vec3(x, 0, z);
            cloudPoint.position.z += 0.05f;
            cloudPoint.normal = glm::vec3(0, 1, 0);
            _pointCloud.push_back(cloudPoint);

            cloudPoint.position = glm::vec3(x, CEILING_HEIGHT, z);
            cloudPoint.position.z += 0.1f;
            cloudPoint.normal = glm::vec3(0, -1, 0);
            _pointCloud.push_back(cloudPoint);
        }
    }



    int x = 0;
    int y = 10;
    int z = 0;
    glm::vec3 probePos = glm::vec3(x, y, z) * PROPOGATION_GRID_SPACING;



    //


    std::cout << "Point cloud created. " << _pointCloud.size() << " points\n";
}

std::vector<CloudPoint>& Lighting::GetPointCloud() {
    return _pointCloud;
}

/*std::vector<ProbePoint>& Lighting::GetProbeGrid() {
    return _probeGrid;
}*/

ProbePoint& Lighting::GetProbe(int x, int y, int z) {
    return _probeGrid[x][y][z];
}