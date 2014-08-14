// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <array>
#include <memory>
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


struct PicaTrace {
    struct Write : public std::pair<u32,u32> {
        using std::pair<u32,u32>::pair;

        u32& Id() { return first; }
        const u32& Id() const { return first; }

        u32& Value() { return second; }
        const u32& Value() const { return second; }
    };
    std::vector<Write> writes;
};

void StartPicaTracing();
bool IsPicaTracing();
void OnPicaRegWrite(u32 id, u32 value);
std::unique_ptr<PicaTrace> FinishPicaTracing();

} // namespace

} // namespace
