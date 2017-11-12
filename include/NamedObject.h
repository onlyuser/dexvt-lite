// This file is part of dexvt-lite.
// -- 3D Inverse Kinematics (Cyclic Coordinate Descent) with Constraints
// Copyright (C) 2018 onlyuser <mailto:onlyuser@gmail.com>
//
// dexvt-lite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dexvt-lite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dexvt-lite.  If not, see <http://www.gnu.org/licenses/>.

#ifndef VT_NAMED_OBJECT_H_
#define VT_NAMED_OBJECT_H_

#include <string>

namespace vt {

class NamedObject
{
public:
    const std::string get_name() const
    {
        return m_name;
    }
    void set_name(std::string name)
    {
        m_name = name;
    }

protected:
    std::string m_name;

    NamedObject(std::string name);
};

struct FindByName : std::unary_function<NamedObject*, std::string>
{
    std::string m_key;

    FindByName(std::string key)
    {
        m_key = key;
    }

    bool operator()(const NamedObject* x) const
    {
        return x->get_name() == m_key;
    }
};

}

#endif
