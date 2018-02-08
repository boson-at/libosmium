#ifndef OSMIUM_INDEX_MAP_SPARSE_MEM_TABLE_HPP
#define OSMIUM_INDEX_MAP_SPARSE_MEM_TABLE_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2018 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#ifdef OSMIUM_WITH_SPARSEHASH

#include <osmium/index/index.hpp>
#include <osmium/index/map.hpp>
#include <osmium/io/detail/read_write.hpp>

#include <google/sparsetable>

#include <cstddef>
#include <utility>
#include <vector>

#define OSMIUM_HAS_INDEX_MAP_SPARSE_MEM_TABLE

namespace osmium {

    namespace index {

        namespace map {

            /**
             * The SparseMemTable index stores elements in a Google sparsetable,
             * a data structure that can hold sparsly filled tables in a
             * space efficient way. It will resize automatically.
             *
             * Use this index if the ID space is only sparsly
             * populated, such as when working with smaller OSM files (like
             * country extracts).
             *
             * This will only work on 64 bit machines.
             */
            template <typename TId, typename TValue>
            class SparseMemTable : public osmium::index::map::Map<TId, TValue> {

                TId m_grow_size;

                google::sparsetable<TValue> m_elements;

                static_assert(sizeof(typename google::sparsetable<TValue>::size_type) >= 8, "google::sparsetable needs 64bit machine");

            public:

                /**
                * Constructor.
                *
                * @param grow_size The initial size of the index (ie number of
                *                  elements that fit into the index).
                *                  The storage will grow by at least this size
                *                  every time it runs out of space.
                */
                explicit SparseMemTable(const TId grow_size = 10000) :
                    m_grow_size(grow_size),
                    m_elements(grow_size) {
                }

                void set(const TId id, const TValue value) final {
                    if (id >= m_elements.size()) {
                        m_elements.resize(id + m_grow_size);
                    }
                    m_elements[id] = value;
                }

                TValue get(const TId id) const final {
                    if (id >= m_elements.size()) {
                        throw osmium::not_found{id};
                    }
                    const TValue value = m_elements[id];
                    if (value == osmium::index::empty_value<TValue>()) {
                        throw osmium::not_found{id};
                    }
                    return value;
                }

                TValue get_noexcept(const TId id) const noexcept final {
                    if (id >= m_elements.size()) {
                        return osmium::index::empty_value<TValue>();
                    }
                    return m_elements[id];
                }

                size_t size() const final {
                    return m_elements.size();
                }

                size_t used_memory() const final {
                    // unused elements use 1 bit, used elements sizeof(TValue) bytes
                    // http://google-sparsehash.googlecode.com/svn/trunk/doc/sparsetable.html
                    return (m_elements.size() / 8) + (m_elements.num_nonempty() * sizeof(TValue));
                }

                void clear() final {
                    m_elements.clear();
                }

                void dump_as_list(const int fd) final {
                    std::vector<std::pair<TId, TValue>> v;
                    v.reserve(m_elements.size());
                    int n = 0;
                    for (const TValue value : m_elements) {
                        if (value != osmium::index::empty_value<TValue>()) {
                            v.emplace_back(n, value);
                        }
                        ++n;
                    }
                    osmium::io::detail::reliable_write(fd, reinterpret_cast<const char*>(v.data()), sizeof(std::pair<TId, TValue>) * v.size());
                }

            }; // class SparseMemTable

        } // namespace map

    } // namespace index

} // namespace osmium

#ifdef OSMIUM_WANT_NODE_LOCATION_MAPS
    REGISTER_MAP(osmium::unsigned_object_id_type, osmium::Location, osmium::index::map::SparseMemTable, sparse_mem_table)
#endif

#endif // OSMIUM_WITH_SPARSEHASH

#endif // OSMIUM_INDEX_MAP_SPARSE_MEM_TABLE_HPP
