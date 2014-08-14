// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <array>
#include <vector>

#include "video_core/pica.h"

namespace Pica {

namespace DebugUtils {

using TriangleTopology = Regs::TriangleTopology;

class GeometryDumper {
public:
    void AddVertex(std::array<float,3> pos, TriangleTopology topology);

    void Dump();

private:
    struct Vertex {
        std::array<float,3> pos;
    };

    struct Face {
        int index[3];
    };

    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

void DumpShader(const u32* binary_data, u32 binary_size, const u32* swizzle_data, u32 swizzle_size, u32 main_offset);

} // namespace

} // namespace
