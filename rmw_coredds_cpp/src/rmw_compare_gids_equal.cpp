// Copyright 2019 GurumNetworks, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rmw/rmw.h"
#include "rmw/error_handling.h"
#include "rmw/types.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rmw_coredds_cpp/identifier.hpp"
#include "rmw_coredds_cpp/types.hpp"

extern "C"
{
rmw_ret_t
rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result)
{
  if (gid1 == nullptr) {
    RMW_SET_ERROR_MSG("gid1 is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    gid1,
    gid1->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_ERROR)

  if (gid1 == nullptr) {
    RMW_SET_ERROR_MSG("gid1 is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    gid1,
    gid1->implementation_identifier,
    gurum_coredds_identifier,
    return RMW_RET_ERROR)

  if (result == nullptr) {
    RMW_SET_ERROR_MSG("result is null");
    return RMW_RET_ERROR;
  }

  const CoreddsPublisherGID * c_gid1 = reinterpret_cast<const CoreddsPublisherGID *>(gid1->data);
  if (c_gid1 == nullptr) {
    RMW_SET_ERROR_MSG("gid1 is invalid");
    return RMW_RET_ERROR;
  }

  const CoreddsPublisherGID * c_gid2 = reinterpret_cast<const CoreddsPublisherGID *>(gid2->data);
  if (c_gid2 == nullptr) {
    RMW_SET_ERROR_MSG("gid2 is invalid");
    return RMW_RET_ERROR;
  }

  *result = (c_gid1->publication_handle == c_gid2->publication_handle);
  return RMW_RET_OK;
}
}  // extern "C"
