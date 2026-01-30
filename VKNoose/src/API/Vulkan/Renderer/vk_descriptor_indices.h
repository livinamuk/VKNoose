#pragma once

// Static binding indices
#define DESC_IDX_SAMPLERS 0
#define DESC_IDX_TEXTURES 1
#define DESC_IDX_UBOS 2
#define DESC_IDX_SSBOS 3
#define DESC_IDX_STORAGE_IMAGES_RGBA32F 4
#define DESC_IDX_STORAGE_IMAGES_RGBA16F 5
#define DESC_IDX_STORAGE_IMAGES_RGBA8   6

// REMOVE ME WHEN U CAN
#define DESC_IDX_VERTICES 7
#define DESC_IDX_INDICES 8

// Render Tagets
#define RT_IDX_FIRST_HIT_COLOR    1000
#define RT_IDX_FIRST_HIT_NORMALS  1001
#define RT_IDX_FIRST_HIT_BASE     1002
#define RT_IDX_SECOND_HIT_COLOR   1003
#define RT_IDX_GBUFFER_BASE       1004
#define RT_IDX_GBUFFER_NORMAL     1005
#define RT_IDX_GBUFFER_RMA        1006
#define RT_IDX_LAPTOP             1007
#define RT_IDX_COMPOSITE          1008
#define RT_IDX_DEPTH_GBUFFER      1009 // Read-only as texture

// Image Stores RGBA32F
#define IMG_IDX_RT_FIRST_COLOR    0
#define IMG_IDX_RT_FIRST_NORMALS  1
#define IMG_IDX_RT_FIRST_BASE     2
#define IMG_IDX_RT_SECOND_COLOR   3

// Image Stores RGBA16F
#define NONE_AS_OF_YET            666

// Image Stores RGBA8
#define IMG_IDX_GBUFFER_BASE      0
#define IMG_IDX_GBUFFER_NORMAL    1
#define IMG_IDX_GBUFFER_RMA       2
#define IMG_IDX_LAPTOP            3
#define IMG_IDX_COMPOSITE         4