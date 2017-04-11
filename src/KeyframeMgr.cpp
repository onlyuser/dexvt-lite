#include <KeyframeMgr.h>
#include <TransformObject.h>

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

void MotionTrack::insert_keyframe(int frame_number, Keyframe* keyframe)
{
    keyframes_t::iterator p = m_keyframes.find(frame_number);
    if(p == m_keyframes.end()) {
        m_keyframes.insert(keyframes_t::value_type(frame_number, keyframe));
        return;
    }
    if((*p).second) { delete [](*p).second; }
    (*p).second = keyframe;
}

void MotionTrack::erase_keyframe(int frame_number)
{
    keyframes_t::iterator p = m_keyframes.find(frame_number);
    if(p == m_keyframes.end()) {
        return;
    }
    if((*p).second) { delete [](*p).second; }
    (*p).second = NULL;
}

void MotionTrack::lerp_frame(int frame_number, glm::vec3* value) const
{
    keyframes_t::const_iterator p = m_keyframes.lower_bound(frame_number);
    if(p == m_keyframes.end()) {
        *value = (*(--p)).second->get_value();
        return;
    }
    if((*p).first == frame_number) {
        *value = (*p).second->get_value();
        return;
    }
    keyframes_t::const_iterator q = p--;
    float start_frame = (*p).first;
    float end_frame   = (*q).first;
    float alpha = (static_cast<float>(frame_number) - start_frame) / (end_frame - start_frame);
    glm::vec3 p_value = (*p).second->get_value();
    glm::vec3 q_value = (*q).second->get_value();
    *value = LERP(p_value, q_value, alpha);
}

Motion::Motion()
{
    m_motion_track[MotionTrack::MOTION_TYPE_ORIGIN] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIGIN);
    m_motion_track[MotionTrack::MOTION_TYPE_ORIENT] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIENT);
    m_motion_track[MotionTrack::MOTION_TYPE_SCALE]  = new MotionTrack(MotionTrack::MOTION_TYPE_SCALE);
}

Motion::~Motion()
{
    for(int i = 0; i < MotionTrack::MOTION_TYPE_COUNT; i++) {
        delete m_motion_track[i];
    }
}

void Motion::insert_keyframe(MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    m_motion_track[motion_type]->insert_keyframe(frame_number, keyframe);
}

void Motion::erase_keyframe(MotionTrack::motion_type_t motion_type, int frame_number)
{
    m_motion_track[motion_type]->erase_keyframe(frame_number);
}

void Motion::lerp_frame(int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const
{
    m_motion_track[motion_type]->lerp_frame(frame_number, value);
}

KeyframeMgr::~KeyframeMgr()
{
    for(script_t::iterator p = m_script.begin(); p != m_script.end(); p++) {
        delete (*p).second;
    }
}

void KeyframeMgr::insert_keyframe(vt::TransformObject* transform_object, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    script_t::iterator p = m_script.find(transform_object);
    if(p == m_script.end()) {
        m_script.insert(script_t::value_type(transform_object, new Motion()));
        p = m_script.find(transform_object);
    }
    (*p).second->insert_keyframe(motion_type, frame_number, keyframe);
}

void KeyframeMgr::erase_keyframe(vt::TransformObject* transform_object, MotionTrack::motion_type_t motion_type, int frame_number)
{
    script_t::iterator p = m_script.find(transform_object);
    if(p == m_script.end()) {
        return;
    }
    if(frame_number == -1) {
        delete (*p).second;
        m_script.erase(p);
        return;
    }
    (*p).second->erase_keyframe(motion_type, frame_number);
}

bool KeyframeMgr::lerp_frame(vt::TransformObject* transform_object, int frame_number, MotionTrack::motion_type_t motion_type, glm::vec3* value) const
{
    script_t::const_iterator p = m_script.find(transform_object);
    if(p == m_script.end()) {
        return false;
    }
    (*p).second->lerp_frame(frame_number, motion_type, value);
    return true;
}

void KeyframeMgr::apply_lerp_frame(vt::TransformObject* transform_object, int frame_number) const
{
    glm::vec3 origin;
    lerp_frame(transform_object, frame_number, MotionTrack::MOTION_TYPE_ORIGIN, &origin);
    glm::vec3 orient;
    lerp_frame(transform_object, frame_number, MotionTrack::MOTION_TYPE_ORIENT, &orient);
    glm::vec3 scale;
    lerp_frame(transform_object, frame_number, MotionTrack::MOTION_TYPE_SCALE, &scale);
    transform_object->set_origin(origin);
    transform_object->set_origin(orient);
    transform_object->set_origin(scale);
}

}
