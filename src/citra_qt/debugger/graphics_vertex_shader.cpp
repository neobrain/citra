// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <iomanip>
#include <sstream>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <QBoxLayout>
#include <QTreeView>

#include "video_core/vertex_shader.h"

#include "graphics_vertex_shader.hxx"

using nihstro::Instruction;
using nihstro::SourceRegister;
using nihstro::SwizzlePattern;

GraphicsVertexShaderModel::GraphicsVertexShaderModel(QObject* parent): QAbstractItemModel(parent) {

}

QModelIndex GraphicsVertexShaderModel::index(int row, int column, const QModelIndex& parent) const {
    return createIndex(row, column);
}

QModelIndex GraphicsVertexShaderModel::parent(const QModelIndex& child) const {
    return QModelIndex();
}

int GraphicsVertexShaderModel::columnCount(const QModelIndex& parent) const {
    return 3;
}

int GraphicsVertexShaderModel::rowCount(const QModelIndex& parent) const {
    return info.code.size();
}

QVariant GraphicsVertexShaderModel::headerData(int section, Qt::Orientation orientation, int role) const {
    switch(role) {
    case Qt::DisplayRole:
    {
        if (section == 0) {
            return tr("Offset");
        } else if (section == 1) {
            return tr("Raw");
        } else if (section == 2) {
            return tr("Disassembly");
        }

        break;
    }
    }

    return QVariant();
}

QVariant GraphicsVertexShaderModel::data(const QModelIndex& index, int role) const {
    switch (role) {
    case Qt::DisplayRole:
    {
        switch (index.column()) {
        case 0:
            return QString("%1").arg(4*index.row(), 8, 16, QLatin1Char('0'));

        case 1:
            return QString("%1").arg(info.code[index.row()].hex, 8, 16, QLatin1Char('0'));

        case 2:
        {
            std::stringstream output;

            Instruction instr = info.code[index.row()];
            const SwizzlePattern& swizzle = info.swizzle_info[instr.common.operand_desc_id].pattern;

            output << instr.opcode.GetInfo().name;

            if (instr.opcode.GetInfo().type == Instruction::OpCodeType::Arithmetic) {
                bool src_is_inverted = 0 != (instr.opcode.GetInfo().subtype & Instruction::OpCodeInfo::SrcInversed);

                std::string src1_relative_address;
                if (!instr.common.AddressRegisterName().empty())
                    src1_relative_address = "[" + instr.common.AddressRegisterName() + "]";

                SourceRegister src1 = instr.common.GetSrc1(src_is_inverted);
                output << std::setw(4) << std::right << instr.common.dest.GetName() << "." << swizzle.DestMaskToString() << "  "
                       << std::setw(8) << std::right << ((swizzle.negate_src1 ? "-" : "") + src1.GetName()) + src1_relative_address << "." << swizzle.SelectorToString(false) << "  ";

                if (instr.opcode.GetInfo().subtype & Instruction::OpCodeInfo::Src2) {
                    SourceRegister src2 = instr.common.GetSrc2(src_is_inverted);
                    output << std::setw(4) << std::right << (swizzle.negate_src2 ? "-" : "") + src2.GetName() << "." << swizzle.SelectorToString(true) << "   ";
                }
            } else if (instr.opcode.GetInfo().type == Instruction::OpCodeType::Conditional) {
                // TODO
            }

            return QString::fromLatin1(output.str().c_str());
        }

        default:
            break;
        }
    }

    case Qt::FontRole:
        return QFont("monospace");

    default:
        break;
    }

    return QVariant();
}

void GraphicsVertexShaderModel::OnUpdate()
{
    beginResetModel();

    info.Clear();

    for (auto instr : Pica::VertexShader::GetShaderBinary())
        info.code.push_back({instr});

    for (auto pattern : Pica::VertexShader::GetSwizzlePatterns())
        info.swizzle_info.push_back({pattern});

    nihstro::LabelInfo main_label;
    main_label.id = 0;
    main_label.program_offset = Pica::registers.vs_main_offset;
    info.labels.insert({0, "main"});
    info.label_table.push_back(main_label);

    endResetModel();
}


GraphicsVertexShaderWidget::GraphicsVertexShaderWidget(std::shared_ptr< Pica::DebugContext > debug_context,
                                                       QWidget* parent)
        : BreakPointObserverDock(debug_context, "Pica Vertex Shader", parent) {
    setObjectName("PicaVertexShader");

    auto binary_model = new GraphicsVertexShaderModel(this);
    auto binary_list = new QTreeView;
    binary_list->setModel(binary_model);
    binary_list->setRootIsDecorated(false);

    connect(this, SIGNAL(Update()), binary_model, SLOT(OnUpdate()));

    auto main_widget = new QWidget;
    auto main_layout = new QVBoxLayout;
    {
        auto sub_layout = new QHBoxLayout;
        sub_layout->addWidget(binary_list);
        main_layout->addLayout(sub_layout);
    }
    main_widget->setLayout(main_layout);
    setWidget(main_widget);
}

void GraphicsVertexShaderWidget::OnBreakPointHit(Pica::DebugContext::Event event, void* data) {
    emit Update();
    widget()->setEnabled(true);
}

void GraphicsVertexShaderWidget::OnResumed() {
    widget()->setEnabled(false);
}
