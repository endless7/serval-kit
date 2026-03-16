// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGMask.h"

#include <functional>
#include <optional>

#include "element/SrSVGNode.h"
#include "element/SrSVGUse.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGMask::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "mask-type") == 0) {
    mask_is_luminance_ = strcmp(value, "alpha") != 0;
    return true;
  } else if (strcmp(name, "maskUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      mask_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      mask_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "maskContentUnits") == 0) {
    if (strcmp(value, "objectBoundingBox") == 0) {
      mask_content_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    } else {
      mask_content_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    }
    return true;
  } else if (strcmp(name, "x") == 0) {
    x_ = Atof(value);
    return true;
  } else if (strcmp(name, "y") == 0) {
    y_ = Atof(value);
    return true;
  } else if (strcmp(name, "width") == 0) {
    width_ = Atof(value);
    return true;
  } else if (strcmp(name, "height") == 0) {
    height_ = Atof(value);
    return true;
  }
  return SrSVGContainer::ParseAndSetAttribute(name, value);
}

std::unique_ptr<canvas::Path> SrSVGMask::AsPath(
    canvas::PathFactory* path_factory, SrSVGRenderContext* context) const {
  std::unique_ptr<canvas::Path> path = path_factory->CreateMutable();

  auto MultiplyMatrix = [](float* out, const float* a, const float* b) {
    float a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
    float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5];
    out[0] = a0 * b0 + a2 * b1;
    out[1] = a1 * b0 + a3 * b1;
    out[2] = a0 * b2 + a2 * b3;
    out[3] = a1 * b2 + a3 * b3;
    out[4] = a0 * b4 + a2 * b5 + a4;
    out[5] = a1 * b4 + a3 * b5 + a5;
  };

  std::function<void(SrSVGNodeBase*, const float*, SrSVGPaint*, SrSVGPaint*,
                     std::optional<SrSVGLength>)>
      process_node = [&](SrSVGNodeBase* child, const float* parent_xform,
                         SrSVGPaint* inherited_fill,
                         SrSVGPaint* inherited_stroke,
                         std::optional<SrSVGLength> inherited_stroke_width) {
        if (!child || !child->IsSVGNode()) {
          return;
        }
        auto node = static_cast<SrSVGNode*>(child);

        float current_xform[6];
        MultiplyMatrix(current_xform, parent_xform, node->transform_);

        if (node->Tag() == SrSVGTag::kUse) {
          auto use_node = static_cast<SrSVGUse*>(node);
          float dx = convert_serval_length_to_float(
              &use_node->x(), context, SR_SVG_LENGTH_TYPE_HORIZONTAL);
          float dy = convert_serval_length_to_float(
              &use_node->y(), context, SR_SVG_LENGTH_TYPE_VERTICAL);
          if (dx != 0 || dy != 0) {
            float translate[6] = {1, 0, 0, 1, dx, dy};
            float temp[6];
            std::copy(std::begin(current_xform), std::end(current_xform),
                      std::begin(temp));
            MultiplyMatrix(current_xform, temp, translate);
          }
        }

        SrSVGPaint* fill = node->fill_ ? node->fill_ : inherited_fill;
        SrSVGPaint* stroke = node->stroke_ ? node->stroke_ : inherited_stroke;
        auto stroke_width = node->stroke_width_.has_value()
                                ? node->stroke_width_
                                : inherited_stroke_width;

        if (node->Tag() == SrSVGTag::kG || node->Tag() == SrSVGTag::kUse) {
          std::vector<SrSVGNodeBase*> targets;
          if (node->Tag() == SrSVGTag::kG) {
            auto container = static_cast<SrSVGContainer*>(node);
            targets = container->children();
          } else {
            auto use_node = static_cast<SrSVGUse*>(node);
            IDMapper* id_mapper = static_cast<IDMapper*>(context->id_mapper);
            if (id_mapper && !use_node->href().empty()) {
              auto it = id_mapper->find(use_node->href());
              if (it != id_mapper->end()) {
                targets.push_back(it->second);
              }
            }
          }

          for (auto* target : targets) {
            process_node(target, current_xform, fill, stroke, stroke_width);
          }
          return;
        }

        bool has_fill = !fill || fill->type != SERVAL_PAINT_NONE;
        if (has_fill) {
          canvas::OP op = canvas::OP::DIFFERENCE;
          if (fill) {
            if (fill->type == SERVAL_PAINT_COLOR &&
                fill->content.color.color == 0xFFFFFFFF) {
              op = canvas::OP::UNION;
            } else {
              op = canvas::OP::XOR;
            }
          } else {
            op = canvas::OP::XOR;
          }

          std::unique_ptr<canvas::Path> fill_path =
              node->AsPath(path_factory, context);
          if (fill_path) {
            fill_path->Transform(current_xform);
            path_factory->Op(path.get(), fill_path.get(), op);
          }
        }

        float stroke_w = 1.0f;
        if (stroke_width.has_value()) {
          stroke_w = convert_serval_length_to_float(&(*stroke_width), context,
                                                    SR_SVG_LENGTH_TYPE_OTHER);
        }

        bool has_stroke =
            stroke && stroke->type != SERVAL_PAINT_NONE && stroke_w > 0;
        if (has_stroke) {
          canvas::OP op = canvas::OP::UNION;
          if (stroke->type == SERVAL_PAINT_COLOR) {
            if (stroke->content.color.color == 0xFFFFFFFF) {
              op = canvas::OP::UNION;
            } else {
              op = canvas::OP::XOR;
            }
          } else {
            op = canvas::OP::XOR;
          }

          std::unique_ptr<canvas::Path> raw_path =
              node->AsPath(path_factory, context);
          if (raw_path) {
            std::unique_ptr<canvas::Path> stroke_path =
                path_factory->CreateStrokePath(
                    raw_path.get(), stroke_w, node->stroke_cap_,
                    node->stroke_join_, node->stoke_miter_limit_);

            if (stroke_path) {
              stroke_path->Transform(current_xform);
              path_factory->Op(path.get(), stroke_path.get(), op);
            }
          }
        }
      };

  float identity[6] = {1, 0, 0, 1, 0, 0};
  for (SrSVGNodeBase* child : children_) {
    process_node(child, identity, this->fill_, this->stroke_, std::nullopt);
  }

  return path;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
