#include <KeyframeMgr.h>
#include <Util.h>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <limits.h>

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
        Keyframe* keyframe = (*p).second;
        if(!keyframe) {
            continue;
        }
        delete keyframe;
    }
}

void MotionTrack::insert_keyframe(int frame_number, Keyframe* keyframe)
{
    keyframes_t::iterator p = m_keyframes.find(frame_number);
    if(p == m_keyframes.end()) {
        m_keyframes.insert(keyframes_t::value_type(frame_number, keyframe));
        return;
    }
    Keyframe* &motion_track_keyframe = (*p).second;
    if(!motion_track_keyframe) {
        return;
    }
    delete []motion_track_keyframe;
    motion_track_keyframe = keyframe;
}

void MotionTrack::erase_keyframe(int frame_number)
{
    keyframes_t::iterator p = m_keyframes.find(frame_number);
    if(p == m_keyframes.end()) {
        return;
    }
    Keyframe* &motion_track_keyframe = (*p).second;
    if(!motion_track_keyframe) {
        return;
    }
    motion_track_keyframe = NULL;
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
    int start_frame_number = (*p).first;
    int end_frame_number   = (*q).first;
    float alpha = static_cast<float>(frame_number - start_frame_number) / static_cast<float>(end_frame_number - start_frame_number);
    glm::vec3 start_frame_value = (*p).second->get_value();
    glm::vec3 end_frame_value   = (*q).second->get_value();
    *value = LERP(start_frame_value, end_frame_value, alpha);
}

bool MotionTrack::get_frame_range(int* start_frame_number, int* end_frame_number) const
{
    if(!start_frame_number && !end_frame_number) {
        return false;
    }
    if(start_frame_number) {
        *start_frame_number = INT_MAX;
        for(keyframes_t::const_iterator p = m_keyframes.begin(); p != m_keyframes.end(); p++) {
            int keyframe_number = (*p).first;
            *start_frame_number = std::min(*start_frame_number, keyframe_number);
        }
    }
    if(end_frame_number) {
        *end_frame_number = INT_MIN;
        for(keyframes_t::const_iterator p = m_keyframes.begin(); p != m_keyframes.end(); p++) {
            int keyframe_number = (*p).first;
            *end_frame_number = std::max(*end_frame_number, keyframe_number);
        }
    }
    return true;
}

ObjectScript::ObjectScript()
{
    m_motion_tracks[MotionTrack::MOTION_TYPE_ORIGIN] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIGIN);
    m_motion_tracks[MotionTrack::MOTION_TYPE_ORIENT] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIENT);
    m_motion_tracks[MotionTrack::MOTION_TYPE_SCALE]  = new MotionTrack(MotionTrack::MOTION_TYPE_SCALE);
}

ObjectScript::~ObjectScript()
{
    delete m_motion_tracks[MotionTrack::MOTION_TYPE_ORIGIN];
    delete m_motion_tracks[MotionTrack::MOTION_TYPE_ORIENT];
    delete m_motion_tracks[MotionTrack::MOTION_TYPE_SCALE];
}

void ObjectScript::insert_keyframe(MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    m_motion_tracks[motion_type]->insert_keyframe(frame_number, keyframe);
}

void ObjectScript::erase_keyframe(unsigned char motion_types, int frame_number)
{
    if(motion_types & MotionTrack::MOTION_TYPE_ORIGIN) {
        m_motion_tracks[MotionTrack::MOTION_TYPE_ORIGIN]->erase_keyframe(frame_number);
    }
    if(motion_types & MotionTrack::MOTION_TYPE_ORIENT) {
        m_motion_tracks[MotionTrack::MOTION_TYPE_ORIENT]->erase_keyframe(frame_number);
    }
    if(motion_types & MotionTrack::MOTION_TYPE_SCALE) {
        m_motion_tracks[MotionTrack::MOTION_TYPE_SCALE]->erase_keyframe(frame_number);
    }
}

void ObjectScript::lerp_frame_for_motion_track(MotionTrack::motion_type_t motion_type, int frame_number, glm::vec3* value) const
{
    motion_tracks_t::const_iterator p = m_motion_tracks.find(motion_type);
    if(p == m_motion_tracks.end()) {
        return;
    }
    MotionTrack* motion_track = (*p).second;
    if(!motion_track) {
        return;
    }
    motion_track->lerp_frame(frame_number, value);
}

bool ObjectScript::get_frame_range(int* start_frame_number, int* end_frame_number) const
{
    if(!start_frame_number && !end_frame_number) {
        return false;
    }
    if(start_frame_number) {
        *start_frame_number = INT_MAX;
    }
    if(end_frame_number) {
        *end_frame_number = INT_MIN;
    }
    for(motion_tracks_t::const_iterator p = m_motion_tracks.begin(); p != m_motion_tracks.end(); p++) {
        MotionTrack* motion_track = (*p).second;
        int motion_track_start_frame_number = -1;
        int motion_track_end_frame_number   = -1;
        if(!motion_track->get_frame_range(&motion_track_start_frame_number, &motion_track_end_frame_number)) {
            continue;
        }
        if(start_frame_number) {
            *start_frame_number = std::min(*start_frame_number, motion_track_start_frame_number);
        }
        if(end_frame_number) {
            *end_frame_number = std::max(*end_frame_number, motion_track_end_frame_number);
        }
    }
    return true;
}

KeyframeMgr::~KeyframeMgr()
{
    for(script_t::iterator p = m_script.begin(); p != m_script.end(); p++) {
        ObjectScript* object_script = (*p).second;
        if(!object_script) {
            continue;
        }
        delete object_script;
    }
}

void KeyframeMgr::insert_keyframe(long object_id, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    script_t::iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        m_script.insert(script_t::value_type(object_id, new ObjectScript()));
        p = m_script.find(object_id);
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return;
    }
    object_script->insert_keyframe(motion_type, frame_number, keyframe);
}

void KeyframeMgr::erase_keyframe(long object_id, unsigned char motion_types, int frame_number)
{
    script_t::iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        return;
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return;
    }
    if(frame_number == -1) {
        delete object_script;
        m_script.erase(p);
        return;
    }
    object_script->erase_keyframe(motion_types, frame_number);
}

bool KeyframeMgr::lerp_frame_for_object_motion_track(long object_id, int frame_number, glm::vec3* origin, glm::vec3* orient, glm::vec3* scale) const
{
    script_t::const_iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        return false;
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return false;
    }
    if(origin) {
        object_script->lerp_frame_for_motion_track(MotionTrack::MOTION_TYPE_ORIGIN, frame_number, origin);
    }
    if(orient) {
        object_script->lerp_frame_for_motion_track(MotionTrack::MOTION_TYPE_ORIENT, frame_number, orient);
    }
    if(scale) {
        object_script->lerp_frame_for_motion_track(MotionTrack::MOTION_TYPE_SCALE, frame_number, scale);
    }
    return true;
}

bool KeyframeMgr::get_frame_range(long object_id, int* start_frame_number, int* end_frame_number) const
{
    if(!start_frame_number && !end_frame_number) {
        return false;
    }
    if(start_frame_number) {
        *start_frame_number = INT_MAX;
    }
    if(end_frame_number) {
        *end_frame_number = INT_MIN;
    }
    script_t::const_iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        return false;
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return false;
    }
    int object_script_start_frame_number = -1;
    int object_script_end_frame_number   = -1;
    if(!object_script->get_frame_range(&object_script_start_frame_number, &object_script_end_frame_number)) {
        return false;
    }
    if(start_frame_number) {
        *start_frame_number = std::min(*start_frame_number, object_script_start_frame_number);
    }
    if(end_frame_number) {
        *end_frame_number = std::max(*end_frame_number, object_script_end_frame_number);
    }
    return true;
}

bool KeyframeMgr::export_object_motion_track(long object_id, std::vector<glm::vec3>* origin_motion_track, std::vector<glm::vec3>* orient_motion_track) const
{
    if(!origin_motion_track && !orient_motion_track) {
        return false;
    }
    int start_frame_number = -1;
    int end_frame_number   = -1;
    if(!get_frame_range(object_id, &start_frame_number, &end_frame_number)) {
        return false;
    }
    for(int frame_number = start_frame_number; frame_number != end_frame_number; frame_number++) {
        glm::vec3 origin;
        glm::vec3 orient;
        if(!lerp_frame_for_object_motion_track(object_id, frame_number, &origin, &orient, NULL)) {
            continue;
        }
        if(origin_motion_track) {
            origin_motion_track->push_back(origin);
        }
        if(orient_motion_track) {
            orient_motion_track->push_back(orient);
        }
    }
    return true;
}

}
