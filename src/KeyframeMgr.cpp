#include <KeyframeMgr.h>
#include <Util.h>
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <limits.h>

namespace vt {

Keyframe::Keyframe(glm::vec3 value, bool is_smooth, float prev_control_point_scale, float next_control_point_scale)
    : m_value(value),
      m_is_smooth(is_smooth),
      m_prev_control_point_scale(prev_control_point_scale),
      m_next_control_point_scale(next_control_point_scale)
{
}

void Keyframe::update_control_points(glm::vec3 prev_point, glm::vec3 next_point, float control_point_scale)
{
    glm::vec3 p1 = prev_point;
    glm::vec3 p2 = m_value;
    glm::vec3 p3 = next_point;
    glm::vec3 control_point_offset = ((p3 - p1) * 0.5f * control_point_scale) * static_cast<float>(m_is_smooth);
    m_control_point1 = p2 - control_point_offset * m_prev_control_point_scale;
    m_control_point2 = p2 + control_point_offset * m_next_control_point_scale;
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

bool MotionTrack::insert_keyframe(int frame_number, Keyframe* keyframe)
{
    if(!keyframe) {
        return false;
    }
    keyframes_t::iterator p = m_keyframes.find(frame_number);
    if(p == m_keyframes.end()) {
        m_keyframes.insert(keyframes_t::value_type(frame_number, keyframe));
        return true;
    }
    Keyframe* &motion_track_keyframe = (*p).second;
    if(!motion_track_keyframe) {
        return false;
    }
    delete motion_track_keyframe;
    motion_track_keyframe = keyframe;
    return true;
}

bool MotionTrack::erase_keyframe(int frame_number)
{
    keyframes_t::iterator p = m_keyframes.find(frame_number);
    if(p == m_keyframes.end()) {
        return false;
    }
    Keyframe* &motion_track_keyframe = (*p).second;
    if(!motion_track_keyframe) {
        return false;
    }
    motion_track_keyframe = NULL;
    return true;
}

bool MotionTrack::export_keyframe_values(std::vector<glm::vec3>* keyframe_values, bool include_control_points)
{
    if(!keyframe_values) {
        return false;
    }
    for(keyframes_t::iterator p = m_keyframes.begin(); p != m_keyframes.end(); p++) {
        Keyframe* keyframe = (*p).second;
        if(!keyframe) {
            continue;
        }
        if(include_control_points) {
            keyframe_values->push_back(keyframe->get_control_point1());
        }
        keyframe_values->push_back(keyframe->get_value());
        if(include_control_points) {
            keyframe_values->push_back(keyframe->get_control_point2());
        }
    }
    return true;
}

bool MotionTrack::interpolate_frame_value(int frame_number, glm::vec3* value, bool is_smooth) const
{
    if(!value) {
        return false;
    }
    if(m_keyframes.empty()) {
        return false;
    }
    keyframes_t::const_iterator p = m_keyframes.lower_bound(frame_number);
    if(p == m_keyframes.end()) {
        *value = (*(--p)).second->get_value();
        return true;
    }
    if((*p).first == frame_number) {
        *value = (*p).second->get_value();
        return true;
    }
    keyframes_t::const_iterator q = p--;
    int start_frame_number = (*p).first;
    int end_frame_number   = (*q).first;
    float alpha = static_cast<float>(frame_number - start_frame_number) / static_cast<float>(end_frame_number - start_frame_number);
    if(is_smooth) {
        // bezier interpolation
        glm::vec3 p1 = (*p).second->get_value();
        glm::vec3 p2 = (*p).second->get_control_point2();
        glm::vec3 p3 = (*q).second->get_control_point1();
        glm::vec3 p4 = (*q).second->get_value();
        *value = bezier_interpolate_point(p1, p2, p3, p4, alpha);
    } else {
        // linear interpolation
        glm::vec3 start_frame_value = (*p).second->get_value();
        glm::vec3 end_frame_value   = (*q).second->get_value();
        *value = LERP_POINT(start_frame_value, end_frame_value, alpha);
    }
    return true;
}

bool MotionTrack::get_frame_number_range(int* start_frame_number, int* end_frame_number) const
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

bool MotionTrack::update_control_points(float control_point_scale)
{
    if(m_keyframes.empty()) {
        return false;
    }
    bool is_loop = glm::distance((*m_keyframes.begin()).second->get_value(), (*(--m_keyframes.end())).second->get_value()) < EPSILON;
    for(keyframes_t::iterator p = m_keyframes.begin(); p != m_keyframes.end(); p++) {
        Keyframe* keyframe = (*p).second;
        if(!keyframe) {
            continue;
        }
        keyframes_t::iterator prev_iter = p;
        keyframes_t::iterator next_iter = p;
        if(is_loop) {
            if(p == m_keyframes.begin()) {
                prev_iter = --(--m_keyframes.end()); // decrement twice to skip the last frame, which is identical to the first
            } else {
                prev_iter--;
            }
            if(p == --m_keyframes.end()) {
                next_iter = ++m_keyframes.begin();
            } else {
                next_iter++;
            }
        } else {
            if(p != m_keyframes.begin()) {
                prev_iter--;
            }
            if(p != --m_keyframes.end()) {
                next_iter++;
            }
        }
        glm::vec3 prev_point = (*prev_iter).second->get_value();
        glm::vec3 next_point = (*next_iter).second->get_value();
        keyframe->update_control_points(prev_point, next_point, control_point_scale);
    }
    return true;
}

ObjectScript::ObjectScript()
{
    m_motion_tracks[MotionTrack::MOTION_TYPE_ORIGIN] = new MotionTrack(MotionTrack::MOTION_TYPE_ORIGIN);
    m_motion_tracks[MotionTrack::MOTION_TYPE_EULER]  = new MotionTrack(MotionTrack::MOTION_TYPE_EULER);
    m_motion_tracks[MotionTrack::MOTION_TYPE_SCALE]  = new MotionTrack(MotionTrack::MOTION_TYPE_SCALE);
}

ObjectScript::~ObjectScript()
{
    delete m_motion_tracks[MotionTrack::MOTION_TYPE_ORIGIN];
    delete m_motion_tracks[MotionTrack::MOTION_TYPE_EULER];
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
    if(motion_types & MotionTrack::MOTION_TYPE_EULER) {
        m_motion_tracks[MotionTrack::MOTION_TYPE_EULER]->erase_keyframe(frame_number);
    }
    if(motion_types & MotionTrack::MOTION_TYPE_SCALE) {
        m_motion_tracks[MotionTrack::MOTION_TYPE_SCALE]->erase_keyframe(frame_number);
    }
}

bool ObjectScript::export_keyframe_values_for_motion_track(MotionTrack::motion_type_t motion_type,
                                                           std::vector<glm::vec3>*    keyframe_values,
                                                           bool                       include_control_points)
{
    if(!keyframe_values) {
        return false;
    }
    motion_tracks_t::const_iterator p = m_motion_tracks.find(motion_type);
    if(p == m_motion_tracks.end()) {
        return false;
    }
    MotionTrack* motion_track = (*p).second;
    if(!motion_track) {
        return false;
    }
    return motion_track->export_keyframe_values(keyframe_values, include_control_points);
}

bool ObjectScript::interpolate_frame_value_for_motion_track(MotionTrack::motion_type_t motion_type,
                                                            int                        frame_number,
                                                            glm::vec3*                 value,
                                                            bool                       is_smooth) const
{
    if(!value) {
        return false;
    }
    motion_tracks_t::const_iterator p = m_motion_tracks.find(motion_type);
    if(p == m_motion_tracks.end()) {
        return false;
    }
    MotionTrack* motion_track = (*p).second;
    if(!motion_track) {
        return false;
    }
    return motion_track->interpolate_frame_value(frame_number, value, is_smooth);
}

bool ObjectScript::get_frame_number_range(int* start_frame_number, int* end_frame_number) const
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
        if(!motion_track) {
            continue;
        }
        int motion_track_start_frame_number = -1;
        int motion_track_end_frame_number   = -1;
        if(!motion_track->get_frame_number_range(&motion_track_start_frame_number, &motion_track_end_frame_number)) {
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

void ObjectScript::update_control_points(float control_point_scale)
{
    for(motion_tracks_t::iterator p = m_motion_tracks.begin(); p != m_motion_tracks.end(); p++) {
        MotionTrack* motion_track = (*p).second;
        if(!motion_track) {
            continue;
        }
        motion_track->update_control_points(control_point_scale);
    }
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

bool KeyframeMgr::insert_keyframe(long object_id, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe)
{
    script_t::iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        m_script.insert(script_t::value_type(object_id, new ObjectScript()));
        p = m_script.find(object_id);
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return false;
    }
    object_script->insert_keyframe(motion_type, frame_number, keyframe);
    return true;
}

bool KeyframeMgr::erase_keyframe(long object_id, unsigned char motion_types, int frame_number)
{
    script_t::iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        return false;
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return false;
    }
    if(frame_number == -1) {
        delete object_script;
        m_script.erase(p);
        return true;
    }
    object_script->erase_keyframe(motion_types, frame_number);
    return true;
}

bool KeyframeMgr::export_keyframe_values_for_object(long                    object_id,
                                                    std::vector<glm::vec3>* origin_keyframe_values,
                                                    std::vector<glm::vec3>* euler_keyframe_values,
                                                    std::vector<glm::vec3>* scale_keyframe_values,
                                                    bool                    include_control_points)
{
    if(!origin_keyframe_values && !euler_keyframe_values && !scale_keyframe_values) {
        return false;
    }
    script_t::const_iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        return false;
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return false;
    }
    if(origin_keyframe_values) {
        object_script->export_keyframe_values_for_motion_track(MotionTrack::MOTION_TYPE_ORIGIN, origin_keyframe_values, include_control_points);
    }
    if(euler_keyframe_values) {
        object_script->export_keyframe_values_for_motion_track(MotionTrack::MOTION_TYPE_EULER, euler_keyframe_values, include_control_points);
    }
    if(scale_keyframe_values) {
        object_script->export_keyframe_values_for_motion_track(MotionTrack::MOTION_TYPE_SCALE, scale_keyframe_values, include_control_points);
    }
    return true;
}

bool KeyframeMgr::interpolate_frame_value_for_object(long       object_id,
                                                     int        frame_number,
                                                     glm::vec3* origin,
                                                     glm::vec3* euler,
                                                     glm::vec3* scale,
                                                     bool       is_smooth) const
{
    if(!origin && !euler && !scale) {
        return false;
    }
    script_t::const_iterator p = m_script.find(object_id);
    if(p == m_script.end()) {
        return false;
    }
    ObjectScript* object_script = (*p).second;
    if(!object_script) {
        return false;
    }
    if(origin) {
        object_script->interpolate_frame_value_for_motion_track(MotionTrack::MOTION_TYPE_ORIGIN, frame_number, origin, is_smooth);
    }
    if(euler) {
        object_script->interpolate_frame_value_for_motion_track(MotionTrack::MOTION_TYPE_EULER, frame_number, euler, is_smooth);
    }
    if(scale) {
        object_script->interpolate_frame_value_for_motion_track(MotionTrack::MOTION_TYPE_SCALE, frame_number, scale, is_smooth);
    }
    return true;
}

bool KeyframeMgr::get_frame_number_range(long object_id, int* start_frame_number, int* end_frame_number) const
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
    if(!object_script->get_frame_number_range(&object_script_start_frame_number, &object_script_end_frame_number)) {
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

void KeyframeMgr::update_control_points(float control_point_scale)
{
    for(script_t::iterator p = m_script.begin(); p != m_script.end(); p++) {
        ObjectScript* object_script = (*p).second;
        if(!object_script) {
            continue;
        }
        object_script->update_control_points(control_point_scale);
    }
}

bool KeyframeMgr::export_frame_values_for_object(long                    object_id,
                                                 std::vector<glm::vec3>* origin_frame_values,
                                                 std::vector<glm::vec3>* euler_frame_values,
                                                 std::vector<glm::vec3>* scale_frame_values,
                                                 bool                    is_smooth) const
{
    if(!origin_frame_values && !euler_frame_values && !scale_frame_values) {
        return false;
    }
    int start_frame_number = -1;
    int end_frame_number   = -1;
    if(!get_frame_number_range(object_id, &start_frame_number, &end_frame_number)) {
        return false;
    }
    for(int frame_number = start_frame_number; frame_number != end_frame_number; frame_number++) {
        glm::vec3 origin;
        glm::vec3 euler;
        glm::vec3 scale;
        if(!interpolate_frame_value_for_object(object_id, frame_number, &origin, &euler, &scale, is_smooth)) {
            continue;
        }
        if(origin_frame_values) {
            origin_frame_values->push_back(origin);
        }
        if(euler_frame_values) {
            euler_frame_values->push_back(euler);
        }
        if(scale_frame_values) {
            scale_frame_values->push_back(scale);
        }
    }
    return true;
}

}
