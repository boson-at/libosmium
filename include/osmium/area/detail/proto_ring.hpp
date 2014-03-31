#ifndef OSMIUM_AREA_DETAIL_PROTO_RING
#define OSMIUM_AREA_DETAIL_PROTO_RING

/*

This file is part of Osmium (http://osmcode.org/osmium).

Copyright 2013,2014 Jochen Topf <jochen@topf.org> and others (see README).

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

#include <algorithm>
#include <cassert>
#include <list>
#include <set>
#include <vector>

#include <osmium/osm/noderef.hpp>
#include <osmium/osm/ostream.hpp>
#include <osmium/area/segment.hpp>

namespace osmium {

    namespace area {

        namespace detail {

            /**
             * A ring in the process of being built by the Assembler object.
             */
            class ProtoRing {

            public:

                typedef std::vector<osmium::area::NodeRefSegment> segments_type;

            private:

                // segments in this ring
                segments_type m_segments;

                bool m_outer {true};

                // if this is an outer ring, these point to it's inner rings (if any)
                std::vector<ProtoRing*> m_inner {};

            public:

                ProtoRing(const NodeRefSegment& segment) :
                    m_segments() {
                    add_segment_end(segment);
                }

                ProtoRing(segments_type::const_iterator sbegin, segments_type::const_iterator send) :
                    m_segments(std::distance(sbegin, send)) {
                    std::copy(sbegin, send, m_segments.begin());
                }

                bool outer() const {
                    return m_outer;
                }

                void set_inner() {
                    m_outer = false;
                }

                segments_type& segments() {
                    return m_segments;
                }

                const segments_type& segments() const {
                    return m_segments;
                }

                void remove_segments(segments_type::iterator sbegin, segments_type::iterator send) {
                    m_segments.erase(sbegin, send);
                }

                void add_segment_end(const NodeRefSegment& segment) {
                    m_segments.push_back(segment);
                }

                void add_segment_start(const NodeRefSegment& segment) {
                    m_segments.insert(m_segments.begin(), segment);
                }

                const NodeRefSegment& first_segment() const {
                    return m_segments.front();
                }

                NodeRefSegment& first_segment() {
                    return m_segments.front();
                }

                const NodeRefSegment& last_segment() const {
                    return m_segments.back();
                }

                NodeRefSegment& last_segment() {
                    return m_segments.back();
                }

                bool closed() const {
                    return m_segments.front().first().location() == m_segments.back().second().location();
                }

                int64_t sum() const {
                    int64_t sum = 0;

                    for (auto segment : m_segments) {
                        sum += static_cast<int64_t>(segment.first().location().x()) * static_cast<int64_t>(segment.second().location().y()) -
                               static_cast<int64_t>(segment.second().location().x()) * static_cast<int64_t>(segment.first().location().y());
                    }

                    return sum;
                }

                bool is_cw() const {
                    return sum() <= 0;
                }

                int64_t area() const {
                    return std::abs(sum()) / 2;
                }

                void swap_segments(ProtoRing& other) {
                    std::swap(m_segments, other.m_segments);
                }

                void add_inner_ring(ProtoRing* ring) {
                    m_inner.push_back(ring);
                }

                const std::vector<ProtoRing*> inner_rings() const {
                    return m_inner;
                }

                void print(std::ostream& out) const {
                    out << "[";
                    bool first = true;
                    for (auto& segment : m_segments) {
                        if (first) {
                            out << segment.first().ref();
                        }
                        out << ',' << segment.second().ref();
                        first = false;
                    }
                    out << "]";
                }

                void reverse() {
                    std::for_each(m_segments.begin(), m_segments.end(), [](NodeRefSegment& segment) {
                        segment.swap_locations();
                    });
                    std::reverse(m_segments.begin(), m_segments.end());
                }

                /**
                 * Merge other ring to end of this ring.
                 */
                void merge_ring(const ProtoRing& other, bool debug) {
                    if (debug) {
                        std::cerr << "        MERGE rings ";
                        print(std::cerr);
                        std::cerr << " to ";
                        other.print(std::cerr);
                        std::cerr << "\n";
                    }
                    size_t n = m_segments.size();
                    m_segments.resize(n + other.m_segments.size());
                    std::copy(other.m_segments.begin(), other.m_segments.end(), m_segments.begin() + n);
                    if (debug) {
                        std::cerr << "          result ring: ";
                        print(std::cerr);
                        std::cerr << "\n";
                    }
                }

                void merge_ring_reverse(const ProtoRing& other, bool debug) {
                    if (debug) {
                        std::cerr << "        MERGE rings (reverse) ";
                        print(std::cerr);
                        std::cerr << " to ";
                        other.print(std::cerr);
                        std::cerr << "\n";
                    }
                    size_t n = m_segments.size();
                    m_segments.resize(n + other.m_segments.size());
                    std::transform(other.m_segments.rbegin(), other.m_segments.rend(), m_segments.begin() + n, [](NodeRefSegment segment) {
                        segment.swap_locations();
                        return segment;
                    });
                    if (debug) {
                        std::cerr << "          result ring: ";
                        print(std::cerr);
                        std::cerr << "\n";
                    }
                }

                const NodeRef& min_node() const {
                    auto it = std::min_element(m_segments.begin(), m_segments.end());
                    if (location_less()(it->first(), it->second())) {
                        return it->first();
                    } else {
                        return it->second();
                    }
                }

                bool is_in(ProtoRing* outer) {
                    osmium::Location testpoint = segments().front().first().location();
                    bool is_in = false;

                    for (size_t i = 0, j = outer->segments().size()-1; i < outer->segments().size(); j = i++) {
                        if (((outer->segments()[i].first().location().y() > testpoint.y()) != (outer->segments()[j].first().location().y() > testpoint.y())) &&
                            (testpoint.x() < (outer->segments()[j].first().location().x() - outer->segments()[i].first().location().x()) * (testpoint.y() - outer->segments()[i].first().location().y()) / (outer->segments()[j].first().location().y() - outer->segments()[i].first().location().y()) + outer->segments()[i].first().location().x()) ) {
                            is_in = !is_in;
                        }
                    }

                    return is_in;
                }

                void get_ways(std::set<const osmium::Way*>& ways) {
                    for (auto& segment : m_segments) {
                        ways.insert(segment.way());
                    }
                }

                bool contains(const osmium::area::NodeRefSegment& segment) const {
                    for (auto& s : m_segments) {
                        if (s == segment || (s.first() == segment.second() && s.second() == segment.first())) {
                            return true;
                        }
                    }
                    return false;
                }

            }; // class ProtoRing

            inline std::ostream& operator<<(std::ostream& out, const ProtoRing& ring) {
                ring.print(out);
                return out;
            }

        } // namespace detail

    } // namespace area

} // namespace osmium

#endif // OSMIUM_AREA_DETAIL_PROTO_RING
