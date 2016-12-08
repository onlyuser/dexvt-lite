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

}

#endif
