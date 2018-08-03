/*******************************************************************************
* Copyright 2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#pragma once
#include <memory>
#include <sstream>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "ngraph/runtime/gpu/gpu_host_parameters.hpp"

namespace ngraph
{
    namespace runtime
    {
        namespace gpu
        {
            // Helper structs to deduce whether a type is iterable
            template <typename T>
            struct has_const_iterator;
            template <typename T>
            struct is_container;

            class GPUKernelArgs
            {
            public:
                GPUKernelArgs(const std::shared_ptr<GPUHostParameters>& params);
                GPUKernelArgs(const GPUKernelArgs& args);

                GPUKernelArgs& add_placeholder(std::string name, std::string type);

                template <typename T>
                typename std::enable_if<!is_container<T>::value, GPUKernelArgs&>::type
                    add(std::string name, const T& arg)
                {
                    add_argument(name, arg);
                }

                template <typename T>
                typename std::enable_if<is_container<T>::value, GPUKernelArgs&>::type
                    add(std::string name, const T& arg)
                {
                    add_arguments(name, arg);
                }

                std::string get_input_signature();
                void** get_argument_list() { return m_argument_list.data(); }
            private:
                template <typename T>
                GPUKernelArgs& add_argument(std::string name, const T& arg)
                {
                    validate();
                    void* host_arg = m_host_parameters->cache(arg);
                    m_argument_list.push_back(host_arg);
                    add_to_signature(name, type_names.at(std::type_index(typeid(T))));
                    return *this;
                }

                template <typename T>
                GPUKernelArgs& add_arguments(std::string name, const T& args)
                {
                    validate();
                    for (auto const& arg : args)
                    {
                        void* host_arg = m_host_parameters->cache(arg);
                        m_argument_list.push_back(host_arg);
                        add_to_signature(name, type_names.at(std::type_index(typeid(T))));
                    }
                    return *this;
                }

                void validate();
                std::string add_to_signature(std::string name, std::string type);

            private:
                bool m_signature_generated;
                std::vector<void*> m_argument_list;
                std::stringstream m_input_signature;
                std::shared_ptr<GPUHostParameters> m_host_parameters;
                static const std::unordered_map<std::type_index, std::string> type_names;
            };

            template <typename T>
            struct has_const_iterator
            {
            private:
                typedef struct
                {
                    char x;
                } true_type;
                typedef struct
                {
                    char x, y;
                } false_type;

                template <typename U>
                static true_type check(typename U::const_iterator*);
                template <typename U>
                static false_type check(...);

            public:
                static const bool value = sizeof(check<T>(0)) == sizeof(true_type);
                typedef T type;
            };

            template <typename T>
            struct is_container : std::integral_constant<bool, has_const_iterator<T>::value>
            {
            };
        }
    }
}