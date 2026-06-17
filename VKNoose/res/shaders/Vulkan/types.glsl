
struct Vertex {
    vec3 position;
    float pad;
    vec3 normal;
    float pad2;
    vec2 texCoord;
    vec2 pad3;
    vec3 tangent;
    float pad4;
};

struct Light {
    vec4 position;
    vec4 color;
};

struct Instance {
    mat4 worldMatrix;
    
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
    
    uint vertexOffset;
    uint indexOffset;
    uint basecolorIndex;
    uint normalIndex;
    
    uint rmaIndex;
    uint materialType; // 0 standard, 1 mirror, 2 glass 
    uint dummy1;
    uint dummy2;
};

struct CameraData {
    mat4 proj;
    mat4 view;
    mat4 projInverse;
    mat4 viewInverse;
    vec4 viewPos;	
    int sizeOfVertex;
    int frameIndex;
    int inventoryOpen;
    int wallpaperALBIndex;
};

struct ObjectData2D {
    mat4 model;
    int index_basecolor;
    int index_color;
    int xClipMin;
    int xClipMax;
    int yClipMin;
    int yClipMax;
    int dummy0;
    int dummy2;
};