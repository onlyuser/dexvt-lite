#ifndef VT_KEYFRAMER_MGR_H_
#define VT_KEYFRAMER_MGR_H_

#include <TransformObject.h>
#include <glm/glm.hpp>
#include <map>

namespace vt {

class Mesh;

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
    bool add_keyframe(int frame_number, Keyframe* keyframe);
    MotionTrack::keyframes_t get_keyframes() const { return m_keyframes; }
    bool lerp_frame_data(int frame_number, glm::vec3* value) const;

private:
    motion_type_t m_motion_type;
    keyframes_t m_keyframes;
};

class ObjectMotion
{
public:
    ObjectMotion();
    ~ObjectMotion();
    bool add_keyframe(MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    bool lerp_frame_data(int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const;

private:
    MotionTrack* m_motion_track[MotionTrack::MOTION_TYPE_COUNT];
};

class KeyframeMgr
{
public:
    typedef std::map<vt::Mesh*, ObjectMotion*> script_t;

    static KeyframeMgr* instance()
    {
        static KeyframeMgr keyframe_mgr;
        return &keyframe_mgr;
    }
    bool add_keyframe(vt::Mesh* mesh, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    bool lerp_frame_data(vt::Mesh* mesh, int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const;

private:
    KeyframeMgr();
    ~KeyframeMgr();

    script_t m_script;
};

}

#endif
