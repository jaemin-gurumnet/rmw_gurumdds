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

#include <array>
#include <utility>
#include <set>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/impl/cpp/key_value.hpp"
#include "rmw/sanity_checks.h"
#include "rmw/convert_rcutils_ret_to_rmw_ret.h"

#include "rmw_coredds_shared_cpp/dds_include.hpp"
#include "rmw_coredds_shared_cpp/types.hpp"
#include "rmw_coredds_shared_cpp/rmw_common.hpp"

rmw_node_t *
shared__rmw_create_node(
  const char * identifier,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_node_security_options_t * security_options)
{
  if (security_options == nullptr) {
    RMW_SET_ERROR_MSG("security_options is null");
    return nullptr;
  }

  dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();
  if (factory == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    return nullptr;
  }

  dds_DomainParticipantQos participant_qos;
  dds_ReturnCode_t ret =
    dds_DomainParticipantFactory_get_default_participant_qos(factory, &participant_qos);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default domain participant qos");
    return nullptr;
  }

  // This is used to get node name from discovered participants
  size_t size = strlen(name) + strlen("name=;") + strlen(namespace_) + strlen("namespace=;") + 1;
  participant_qos.user_data.size = size;
  int written = snprintf(
    reinterpret_cast<char *>(participant_qos.user_data.value), size,
    "name=%s;namespace=%s;", name, namespace_);
  if (written < 0 || written > static_cast<int>(size) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return nullptr;
  }

  rmw_node_t * node_handle = nullptr;
  CoreddsNodeInfo * node_info = nullptr;
  rmw_guard_condition_t * graph_guard_condition = nullptr;
  CoreddsPublisherListener * publisher_listener = nullptr;
  CoreddsSubscriberListener * subscriber_listener = nullptr;
  void * buf = nullptr;
  dds_Subscriber * builtin_subscriber = nullptr;
  dds_DataReader * builtin_publication_datareader = nullptr;
  dds_DataReader * builtin_subscription_datareader = nullptr;

  dds_DomainParticipant * participant = nullptr;

  // TODO(junho): Implement security features

  participant = dds_DomainParticipantFactory_create_participant(
    factory, domain_id, &participant_qos, nullptr, 0);
  graph_guard_condition = shared__rmw_create_guard_condition(identifier);
  if (graph_guard_condition == nullptr) {
    RMW_SET_ERROR_MSG("failed to create graph guard condition");
    goto fail;
  }

  buf = rmw_allocate(sizeof(CoreddsPublisherListener));
  if (buf == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    goto fail;
  }

  RMW_TRY_PLACEMENT_NEW(
    publisher_listener, buf, goto fail,
    CoreddsPublisherListener, identifier, graph_guard_condition);
  buf = nullptr;

  buf = rmw_allocate(sizeof(CoreddsSubscriberListener));
  if (buf == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    goto fail;
  }
  RMW_TRY_PLACEMENT_NEW(
    subscriber_listener, buf, goto fail,
    CoreddsSubscriberListener, identifier, graph_guard_condition);
  buf = nullptr;

  node_handle = rmw_node_allocate();
  if (node_handle == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node handle");
    goto fail;
  }

  node_handle->implementation_identifier = identifier;
  node_handle->data = participant;
  node_handle->name = reinterpret_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
  if (node_handle->name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node name");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->name), name, strlen(name) + 1);

  node_handle->namespace_ =
    reinterpret_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
  if (node_handle->name == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node namespace");
    goto fail;
  }
  memcpy(const_cast<char *>(node_handle->namespace_), namespace_, strlen(namespace_) + 1);

  buf = rmw_allocate(sizeof(CoreddsNodeInfo));
  if (buf == nullptr) {
    RMW_SET_ERROR_MSG("failed to allocate memory");
    goto fail;
  }

  RMW_TRY_PLACEMENT_NEW(node_info, buf, goto fail, CoreddsNodeInfo, )
  buf = nullptr;

  node_info->participant = participant;
  node_info->graph_guard_condition = graph_guard_condition;
  node_info->pub_listener = publisher_listener;
  node_info->sub_listener = subscriber_listener;

  node_handle->implementation_identifier = identifier;
  node_handle->data = node_info;

  // set listeners
  builtin_subscriber = dds_DomainParticipant_get_builtin_subscriber(participant);
  builtin_publication_datareader =
    dds_Subscriber_lookup_datareader(builtin_subscriber, "BuiltinPublications");
  if (builtin_publication_datareader == nullptr) {
    RMW_SET_ERROR_MSG("builtin publication datareader handle is null");
    goto fail;
  }
  builtin_subscription_datareader =
    dds_Subscriber_lookup_datareader(builtin_subscriber, "BuiltinSubscriptions");
  if (builtin_subscription_datareader == nullptr) {
    RMW_SET_ERROR_MSG("builtin subscription datareader handle is null");
    goto fail;
  }

  node_info->pub_listener->dds_reader = builtin_publication_datareader;
  dds_DataReader_set_listener(
    builtin_publication_datareader,
    &node_info->pub_listener->dds_listener, dds_DATA_AVAILABLE_STATUS);
  dds_DataReader_set_listener_context(
    builtin_publication_datareader, &node_info->pub_listener->context);
  node_info->sub_listener->dds_reader = builtin_subscription_datareader;
  dds_DataReader_set_listener(
    builtin_subscription_datareader,
    &node_info->sub_listener->dds_listener, dds_DATA_AVAILABLE_STATUS);
  dds_DataReader_set_listener_context(
    builtin_subscription_datareader, &node_info->sub_listener->context);

  return node_handle;

fail:
  dds_ReturnCode_t fret = dds_DomainParticipantFactory_delete_participant(factory, participant);
  if (fret != dds_RETCODE_OK) {
    std::stringstream ss;
    ss << "leaking participant while handling failure at " <<
      __FILE__ << ":" << __LINE__;
    (std::cerr << ss.str()).flush();
  }

  if (graph_guard_condition != nullptr) {
    rmw_ret_t rmw_ret = shared__rmw_destroy_guard_condition(identifier, graph_guard_condition);
    if (rmw_ret != RMW_RET_OK) {
      std::stringstream ss;
      ss << "failed to destroy guard condition while handling failure at " <<
        __FILE__ << ":" << __LINE__;
      (std::cerr << ss.str()).flush();
    }
  }

  if (node_handle != nullptr) {
    if (node_handle->name != nullptr) {
      rmw_free(const_cast<char *>(node_handle->name));
    }

    if (node_handle->namespace_ != nullptr) {
      rmw_free(const_cast<char *>(node_handle->namespace_));
    }

    rmw_free(node_handle);
  }

  if (node_info != nullptr) {
    rmw_free(node_info);
  }

  if (buf != nullptr) {
    rmw_free(buf);
  }

  return nullptr;
}

rmw_ret_t
shared__rmw_destroy_node(const char * identifier, rmw_node_t * node)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node_handle,
    node->implementation_identifier, identifier,
    return RMW_RET_ERROR);

  dds_DomainParticipantFactory * factory = dds_DomainParticipantFactory_get_instance();
  if (factory == nullptr) {
    RMW_SET_ERROR_MSG("failed to get domain participant factory");
    return RMW_RET_ERROR;
  }

  auto node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return RMW_RET_ERROR;
  }

  dds_ReturnCode_t ret = dds_DomainParticipantFactory_delete_participant(factory, participant);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to delete participant");
    return RMW_RET_ERROR;
  }

  if (node_info->pub_listener != nullptr) {
    RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
      node_info->pub_listener->~CoreddsPublisherListener(), CoreddsPublisherListener)
    rmw_free(node_info->pub_listener);
    node_info->pub_listener = nullptr;
  }

  if (node_info->sub_listener != nullptr) {
    RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
      node_info->sub_listener->~CoreddsSubscriberListener(), CoreddsSubscirberListener)
    rmw_free(node_info->sub_listener);
    node_info->sub_listener = nullptr;
  }

  if (node_info->graph_guard_condition != nullptr) {
    rmw_ret_t rmw_ret =
      shared__rmw_destroy_guard_condition(identifier, node_info->graph_guard_condition);
    if (rmw_ret != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("failed to delete graph guard condition");
      return RMW_RET_ERROR;
    }
    node_info->graph_guard_condition = nullptr;
  }

  rmw_free(node_info);
  node->data = nullptr;
  rmw_free(const_cast<char *>(node->name));
  node->name = nullptr;
  rmw_free(const_cast<char *>(node->namespace_));
  node->namespace_ = nullptr;
  rmw_node_free(node);

  return RMW_RET_OK;
}

rmw_ret_t
shared__rmw_node_assert_liveliness(
  const char * implementation_identifier,
  const rmw_node_t * node)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier,
    implementation_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)

  auto node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  if (node_info->participant == nullptr) {
    RMW_SET_ERROR_MSG("node internal participant is invalid");
    return RMW_RET_ERROR;
  }

  if (dds_DomainParticipant_assert_liveliness(node_info->participant) != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to assert liveliness of participant");
    return RMW_RET_ERROR;
  }

  return RMW_RET_OK;
}

const rmw_guard_condition_t *
shared__rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  auto node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return nullptr;
  }

  return node_info->graph_guard_condition;
}

rmw_ret_t
shared__rmw_get_node_names(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  if (node == nullptr) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }

  if (rmw_check_zero_rmw_string_array(node_names) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (rmw_check_zero_rmw_string_array(node_namespaces) != RMW_RET_OK) {
    return RMW_RET_ERROR;
  }

  if (node->implementation_identifier != identifier) {
    RMW_SET_ERROR_MSG("node handle not from this implementation");
    return RMW_RET_ERROR;
  }

  CoreddsNodeInfo * node_info = static_cast<CoreddsNodeInfo *>(node->data);
  if (node_info == nullptr) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }

  dds_DomainParticipant * participant = node_info->participant;
  if (participant == nullptr) {
    RMW_SET_ERROR_MSG("domain participant handle is null");
    return RMW_RET_ERROR;
  }

  dds_InstanceHandleSeq * handle_seq = dds_InstanceHandleSeq_create(4);
  if (handle_seq == nullptr) {
    RMW_SET_ERROR_MSG("failed to create instance handle sequence");
    return RMW_RET_BAD_ALLOC;
  }

  // Get discovered participants
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  dds_ReturnCode_t ret = dds_DomainParticipant_get_discovered_participants(participant, handle_seq);
  if (ret != dds_RETCODE_OK) {
    RMW_SET_ERROR_MSG("unable to fetch discovered participants.");
    return RMW_RET_ERROR;
  }

  uint32_t length = dds_InstanceHandleSeq_length(handle_seq);
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  rcutils_ret_t rcutils_ret = rcutils_string_array_init(node_names, length, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
  }

  rcutils_ret = rcutils_string_array_init(node_namespaces, length, &allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
    return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
  }

  for (uint32_t i = 0; i < length; ++i) {
    dds_ParticipantBuiltinTopicData pbtd;
    dds_InstanceHandle_t handle = dds_InstanceHandleSeq_get(handle_seq, i);
    ret = dds_DomainParticipant_get_discovered_participant_data(participant, &pbtd, handle);
    std::string name;
    std::string namespace_;
    if (ret == dds_RETCODE_OK) {
      // Get node name and namespace from user_data
      uint8_t * data = pbtd.user_data.value;
      std::vector<uint8_t> kv(data, data + pbtd.user_data.size);
      auto map = rmw::impl::cpp::parse_key_value(kv);
      auto name_found = map.find("name");
      auto ns_found = map.find("namespace");

      if (name_found != map.end()) {
        name = std::string(name_found->second.begin(), name_found->second.end());
      }

      if (ns_found != map.end()) {
        namespace_ = std::string(ns_found->second.begin(), ns_found->second.end());
      }
    }

    if (name.empty()) {  // Ignore discovered participants without a name
      node_names->data[i] = nullptr;
      node_namespaces->data[i] = nullptr;
      continue;
    }

    node_names->data[i] = rcutils_strdup(name.c_str(), allocator);
    if (node_names->data[i] == nullptr) {
      RMW_SET_ERROR_MSG("could not allocate memory for node name");
      goto fail;
    }

    node_namespaces->data[i] = rcutils_strdup(namespace_.c_str(), allocator);
    if (node_namespaces->data[i] == nullptr) {
      RMW_SET_ERROR_MSG("could not allocate memory for node namspace");
      goto fail;
    }

    RCUTILS_LOG_DEBUG_NAMED(
      "rmw_coredds_cpp", "node found: %s %s", namespace_.c_str(), name.c_str());
  }

  return RMW_RET_OK;

fail:
  if (node_names != nullptr) {
    rcutils_ret = rcutils_string_array_fini(node_names);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_coredds_cpp",
        "failed to cleanup during error handling; %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }

  if (node_namespaces != nullptr) {
    rcutils_ret = rcutils_string_array_fini(node_namespaces);
    if (rcutils_ret != RCUTILS_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
        "rmw_coredds_cpp",
        "failed to cleanup during error handling; %s", rcutils_get_error_string().str);
      rcutils_reset_error();
    }
  }

  return RMW_RET_BAD_ALLOC;
}