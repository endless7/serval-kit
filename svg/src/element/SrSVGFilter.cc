// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "element/SrSVGFilter.h"
#include <cstring>
#include "element/SrSVGTypes.h"

namespace serval {
namespace svg {
namespace element {

bool SrSVGFilter::ParseAndSetAttribute(const char* name, const char* value) {
  if (strcmp(name, "x") == 0) {
    x_ = make_serval_length(value);
  } else if (strcmp(name, "y") == 0) {
    y_ = make_serval_length(value);
  } else if (strcmp(name, "width") == 0) {
    width_ = make_serval_length(value);
  } else if (strcmp(name, "height") == 0) {
    height_ = make_serval_length(value);
  } else if (strcmp(name, "filterUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      filter_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else if (strcmp(value, "objectBoundingBox") == 0) {
      filter_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
  } else if (strcmp(name, "primitiveUnits") == 0) {
    if (strcmp(value, "userSpaceOnUse") == 0) {
      primitive_units_ = SR_SVG_OBB_UNIT_TYPE_USER_SPACE_ON_USE;
    } else if (strcmp(value, "objectBoundingBox") == 0) {
      primitive_units_ = SR_SVG_OBB_UNIT_TYPE_OBJECT_BOUNDING_BOX;
    }
  } else {
    return SrSVGContainer::ParseAndSetAttribute(name, value);
  }
  return true;
}

}  // namespace element
}  // namespace svg
}  // namespace serval
