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

#ifndef RMW_COREDDS_SHARED_CPP__RMW_COMMON_HPP_
#define RMW_COREDDS_SHARED_CPP__RMW_COMMON_HPP_

#include "./visibility_control.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/types.h"
#include "rmw/names_and_types.h"

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_guard_condition_t *
shared__rmw_create_guard_condition(const char * identifier);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_destroy_guard_condition(
  const char * implementation_identifier,
  rmw_guard_condition_t * guard_condition);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_trigger_guard_condition(
  const char * implementation_identifier,
  const rmw_guard_condition_t * guard_condition);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_node_t *
shared__rmw_create_node(
  const char * identifier,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  const rmw_node_security_options_t * security_options);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_destroy_node(
  const char * identifier,
  rmw_node_t * node);

RMW_COREDDS_SHARED_CPP_PUBLIC
const rmw_guard_condition_t *
shared__rmw_node_get_graph_guard_condition(const rmw_node_t * node);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_node_names(
  const char * identifier,
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_publish(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const void * ros_message);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_publish_serialized_message(
  const char * identifier,
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_take(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_take_with_info(
  const char * identifier,
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_wait_set_t *
shared__rmw_create_wait_set(
  const char * implementation_identifier,
  size_t max_conditions);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_destroy_wait_set(
  const char * identifier,
  rmw_wait_set_t * wait_set);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_subscriber_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_publisher_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_service_names_and_types_by_node(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_count_publishers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_count_subscribers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_service_names_and_types(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_get_topic_names_and_types(
  const char * implementation_identifier,
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * service_names_and_types);

RMW_COREDDS_SHARED_CPP_PUBLIC
rmw_ret_t
shared__rmw_set_log_severity(rmw_log_severity_t severity);

#endif  // RMW_COREDDS_SHARED_CPP__RMW_COMMON_HPP_