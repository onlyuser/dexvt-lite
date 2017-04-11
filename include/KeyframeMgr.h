#ifndef VT_KEYFRAMER_MGR_H_
#define VT_KEYFRAMER_MGR_H_

#include <TransformObject.h>
#include <glm/glm.hpp>
#include <map>

namespace vt {

class TransformObject;

class Keyframe
{
public:
    Keyframe(glm::vec3 value);
    glm::vec3 get_value() const { return m_value; }

private:
    glm::vec3 m_value;
};

class MotionTrack
{
public:
    enum motion_type_t {
        MOTION_TYPE_ORIGIN,
        MOTION_TYPE_ORIENT,
        MOTION_TYPE_SCALE,
        MOTION_TYPE_COUNT
    };

    typedef std::map<int, Keyframe*> keyframes_t;

    MotionTrack(motion_type_t motion_type);
    ~MotionTrack();
    motion_type_t get_motion_type() const { return m_motion_type; }
    void insert_keyframe(int frame_number, Keyframe* keyframe);
    void erase_keyframe(int frame_number);
    MotionTrack::keyframes_t get_keyframes() const { return m_keyframes; }
    void lerp_frame(int frame_number, glm::vec3* value) const;

private:
    motion_type_t m_motion_type;
    keyframes_t   m_keyframes;
};

class Motion
{
public:
    Motion();
    ~Motion();
    void insert_keyframe(MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    void erase_keyframe(MotionTrack::motion_type_t motion_type, int frame_number);
    void lerp_frame(int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const;

private:
    MotionTrack* m_motion_track[MotionTrack::MOTION_TYPE_COUNT];
};

class KeyframeMgr
{
public:
    typedef std::map<vt::TransformObject*, Motion*> script_t;

    static KeyframeMgr* instance()
    {
        static KeyframeMgr keyframe_mgr;
        return &keyframe_mgr;
    }
    void insert_keyframe(vt::TransformObject* transform_object, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    void erase_keyframe(vt::TransformObject* transform_object, MotionTrack::motion_type_t motion_type, int frame_number = -1);
    bool lerp_frame(vt::TransformObject* transform_object, int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const;
    void apply_lerp_frame(vt::TransformObject* transform_object, int frame_number) const;

private:
    KeyframeMgr();
    ~KeyframeMgr();

    script_t m_script;
};

}

#endif
