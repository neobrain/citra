// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <fstream>
#include <mutex>
#include <string>

#include "video_core/pica.h"

#include "debug_utils.h"

namespace Pica {

namespace DebugUtils {

void GeometryDumper::AddVertex(std::array<float,3> pos, TriangleTopology topology) {
    vertices.push_back({pos[0], pos[1], pos[2]});

    int num_vertices = vertices.size();

    switch (topology) {
    case TriangleTopology::List:
    case TriangleTopology::ListIndexed:
        if (0 == (num_vertices % 3))
            faces.push_back({ num_vertices-3, num_vertices-2, num_vertices-1 });
        break;

    default:
        ERROR_LOG(GPU, "Unknown triangle topology %x", (int)topology);
        exit(0);
        break;
    }
}

void GeometryDumper::Dump() {
    // NOTE: Permanently enabling this just trashes hard disks for no reason.
    //       Hence, this is currently disabled.
    return;

    static int index = 0;
    std::string filename = std::string("geometry_dump") + std::to_string(++index) + ".obj";

    std::ofstream file(filename);

    for (const auto& vertex : vertices) {
        file << "v " << vertex.pos[0]
             << " "  << vertex.pos[1]
             << " "  << vertex.pos[2] << std::endl;
    }

    for (const Face& face : faces) {
        file << "f " << 1+face.index[0]
             << " "  << 1+face.index[1]
             << " "  << 1+face.index[2] << std::endl;
    }
}

#pragma pack(1)
struct DVLBHeader {
    enum : u32 {
        MAGIC_WORD = 0x424C5644, // "DVLB"
    };

    u32 magic_word;
    u32 num_programs;
//    u32 dvle_offset_table[];
};
static_assert(sizeof(DVLBHeader) == 0x8, "Incorrect structure size");

struct DVLPHeader {
    enum : u32 {
        MAGIC_WORD = 0x504C5644, // "DVLP"
    };

    u32 magic_word;
    u32 version;
    u32 binary_offset;  // relative to DVLP start
    u32 binary_size_words;
    u32 unk1_offset;
    u32 unk1_num_entries;
    u32 unk2;
};
static_assert(sizeof(DVLPHeader) == 0x1C, "Incorrect structure size");

struct DVLEHeader {
    enum : u32 {
        MAGIC_WORD = 0x454c5644, // "DVLE"
    };

    enum class ShaderType : u8 {
        VERTEX = 0,
        GEOMETRY = 1,
    };

    u32 magic_word;
    u16 pad1;
    ShaderType type;
    u8 pad2;
    u32 main_offset_words; // offset within binary blob
    u32 endmain_offset_words;
    u32 pad3;
    u32 pad4;
    u32 constant_table_offset;
    u32 constant_table_size; // number of entries
    u32 label_table_offset;
    u32 label_table_size;
    u32 output_register_table_offset;
    u32 output_register_table_size;
    u32 uniform_table_offset;
    u32 uniform_table_size;
    u32 symbol_table_offset;
    u32 symbol_table_size;

};
static_assert(sizeof(DVLEHeader) == 0x40, "Incorrect structure size");
#pragma pack()

void DumpShader(const u32* binary_data, u32 binary_size, const u32* swizzle_data, u32 swizzle_size, u32 main_offset)
{
    // NOTE: Permanently enabling this just trashes hard disks for no reason.
    //       Hence, this is currently disabled.
    return;

    struct {
        DVLBHeader header;
        u32 dvle_offset;
    } dvlb{ {DVLBHeader::MAGIC_WORD, 1 } }; // 1 DVLE
    DVLPHeader dvlp{ DVLPHeader::MAGIC_WORD };
    DVLEHeader dvle{ DVLEHeader::MAGIC_WORD };

    // TODO: dvle_offset_table

    dvlb.dvle_offset = sizeof(dvlb) +  // DVLB header + DVLE offset table
                       sizeof(dvlp); // DVLP header

    dvlp.binary_offset = sizeof(dvlp) + sizeof(dvle);
    dvlp.binary_size_words = binary_size; // TODO
    dvlp.unk1_offset = sizeof(dvlp) + sizeof(dvle) + 4 * binary_size;
    dvlp.unk1_num_entries = swizzle_size; // TODO

    dvle.main_offset_words = main_offset;

    static int dump_index = 0;
    std::string filename = std::string("shader_dump") + std::to_string(++dump_index) + std::string(".shbin");
    std::ofstream file(filename, std::ios_base::out | std::ios_base::binary);

    file.write((char*)&dvlb, sizeof(dvlb));
    file.write((char*)&dvlp, sizeof(dvlp));
    file.write((char*)&dvle, sizeof(dvle));
    file.write((char*)binary_data, binary_size * sizeof(u32));
    for (int i = 0; i < swizzle_size; ++i) {
        u32 dummy = 0;
        file.write((char*)&swizzle_data[i], sizeof(swizzle_data[i]));
        file.write((char*)&dummy, sizeof(dummy));
    }
}

static std::unique_ptr<PicaTrace> pica_trace;
static std::mutex pica_trace_mutex;
static int is_pica_tracing = false;

void StartPicaTracing()
{
    if (is_pica_tracing) {
        ERROR_LOG(GPU, "StartPicaTracing called even though tracing already running!");
        return;
    }

    pica_trace_mutex.lock();
    pica_trace = std::unique_ptr<PicaTrace>(new PicaTrace);

    is_pica_tracing = true;
    pica_trace_mutex.unlock();
}

bool IsPicaTracing()
{
    return is_pica_tracing;
}

void OnPicaRegWrite(u32 id, u32 value)
{
    // Double check for is_pica_tracing to avoid pointless locking overhead
    if (!is_pica_tracing)
        return;

    std::unique_lock<std::mutex> lock(pica_trace_mutex);

    if (!is_pica_tracing)
        return;

    pica_trace->writes.push_back({id, value});
}

std::unique_ptr<PicaTrace> FinishPicaTracing()
{
    if (!is_pica_tracing) {
        ERROR_LOG(GPU, "FinishPicaTracing called even though tracing already running!");
        return {};
    }

    // signalize that no further tracing should be performed
    is_pica_tracing = false;

    // Wait until running tracing is finished
    pica_trace_mutex.lock();
    std::unique_ptr<PicaTrace> ret(std::move(pica_trace));
    pica_trace_mutex.unlock();
    return std::move(ret);
}

} // namespace

} // namespace
