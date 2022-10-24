/**
 *  This file is part of xEngine, a C++ game engine library.
 *  Copyright (C) 2021  Julian Zampiccoli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef XENGINE_RESOURCEPARSER_HPP
#define XENGINE_RESOURCEPARSER_HPP

#include <set>
#include <string>

#include "io/archive.hpp"
#include "resource/resourcebundle.hpp"
#include "driver/driver.hpp"

namespace xng {
    class ResourceImporter;

    /**
     * A parser creates resource objects from the data in buffers.
     */
    class XENGINE_EXPORT ResourceParser : public Driver {
    public:
        virtual ~ResourceParser() noexcept override = default;

        /**
         * Read the bundle data from the buffer.
         *
         * @param buffer
         * @param hint
         * @param importer The importer instance to use for importing referenced asset data
         * @param archive The archive instance to use for resolving referenced asset paths.
         * @return
         */
        virtual ResourceBundle read(const std::string &buffer,
                                    const std::string &hint,
                                    const ResourceImporter &importer,
                                    Archive *archive) const = 0;

        /**
         * @return The set of supported file extensions with each containing the preceding dot
         */
        virtual const std::set<std::string> &getSupportedFormats() const = 0;

    private:
        std::type_index getBaseType() override {
            return typeid(ResourceParser);
        }
    };
}

#endif //XENGINE_RESOURCEPARSER_HPP
