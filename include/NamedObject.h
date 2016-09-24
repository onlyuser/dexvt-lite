#ifndef VT_NAMED_OBJECT_H_
#define VT_NAMED_OBJECT_H_

#include <string>

namespace vt {

class NamedObject
{
public:
    const std::string name() const
    {
        return m_name;
    }

protected:
    std::string m_name;

    NamedObject(std::string name);
};

}

#endif
