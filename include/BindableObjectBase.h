#ifndef VT_BINDABLE_OBJECT_BASE_H_
#define VT_BINDABLE_OBJECT_BASE_H_

namespace vt {

class BindableObjectBase
{
public:
    virtual ~BindableObjectBase() {}
    virtual void bind() = 0;
};

}

#endif
