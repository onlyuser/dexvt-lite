#include <KeyframeMgr.h>

namespace vt {

Keyframe::Keyframe(glm::vec3 value)
    : m_value(value)
{
}

MotionTrack::MotionTrack(motion_type_t motion_type)
    : m_motion_type(motion_type)
{
}

MotionTrack::~MotionTrack()
{
    for(keyframes_t::iterator p = m_keyframes.begin(); p != m_keyframes.end(); p++) {
        delete (*p).second;
    }
}

bool MotionTrack::add_keyframe(int frame_number, Keyframe* keyframe)
{
    if(m_keyframes.find(frame_number) != m_keyframes.end()) {
        return false;
    }
    m_keyframes.insert(keyframes_t::value_type(frame_number, keyframe));
    return true;
}

bool MotionTrack::lerp_frame_data(int frame_number, glm::vec3* value) const
{
    //TODO: fix-me!
    return true;
}

ObjectMotion::ObjectMotion()
{
    m_motion_track[MotionTrack::MOTION_TYPE_ORIGIN] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIGIN);
    m_motion_track[MotionTrack::MOTION_TYPE_ORIENT] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIENT);
    m_motion_track[MotionTrack::MOTION_TYPE_SCALE]  = new MotionTrack(MotionTrack::MOTION_TYPE_SCALE);
}

ObjectMotion::~ObjectMotion()
{
    for(int i = 0; i < MotionTrack::MOTION_TYPE_COUNT; i++) {
        delete m_motion_track[i];
    }
}

bool ObjectMotion::add_keyframe(MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    if(!m_motion_track[motion_type]) {
        return false;
    }
    return m_motion_track[motion_type]->add_keyframe(frame_number, keyframe);
}

bool ObjectMotion::lerp_frame_data(int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const
{
    if(!m_motion_track[motion_type]) {
        return false;
    }
    return m_motion_track[motion_type]->lerp_frame_data(frame_number, value);
}

KeyframeMgr::~KeyframeMgr()
{
    for(script_t::iterator p = m_script.begin(); p != m_script.end(); p++) {
        delete (*p).second;
    }
}

bool KeyframeMgr::add_keyframe(vt::Mesh* mesh, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    script_t::iterator p = m_script.find(mesh);
    if(p == m_script.end()) {
        m_script.insert(script_t::value_type(mesh, new ObjectMotion()));
        p = m_script.find(mesh);
    }
    return (*p).second->add_keyframe(motion_type, frame_number, keyframe);
}

bool KeyframeMgr::lerp_frame_data(vt::Mesh* mesh, int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const
{
    script_t::const_iterator p = m_script.find(mesh);
    if(p == m_script.end()) {
        return false;
    }
    return (*p).second->lerp_frame_data(frame_number, motion_type, value);
}

}
