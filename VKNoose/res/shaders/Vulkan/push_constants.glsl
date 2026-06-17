struct ScenePushConstant {
    uint64_t instancesDeviceAddress;
    uint64_t lightsDeviceAddress;
    uint64_t cameraDeviceAddress;
    uint64_t padding0;

    uint lightCount;
    uint padding1;
    uint padding2;
    uint padding3;
};

struct UIPushConstant {
    uint64_t instancesDeviceAddress;
    uint64_t padding0;
};

struct MousePickPushConstant {
    uint64_t cameraDeviceAddress;
    uint64_t mousePickBufferAddress;
};