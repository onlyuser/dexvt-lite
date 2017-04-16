#ifndef VT_KEYFRAMER_MGR_H_
#define VT_KEYFRAMER_MGR_H_

#include <glm/glm.hpp>
#include <map>
#include <vector>

namespace vt {

class Keyframe
{
public:
    Keyframe(glm::vec3 value);
    glm::vec3 get_value() const           { return m_value; }
    glm::vec3 get_control_point_1() const { return m_control_point_1; }
    glm::vec3 get_control_point_2() const { return m_control_point_2; }

private:
    glm::vec3 m_value;
    glm::vec3 m_control_point_1;
    glm::vec3 m_control_point_2;
};

class MotionTrack
{
public:
    enum motion_type_t {
        MOTION_TYPE_ORIGIN = 1,
        MOTION_TYPE_ORIENT = 2,
        MOTION_TYPE_SCALE  = 4
    };

    typedef std::map<int, Keyframe*> keyframes_t;

    MotionTrack(motion_type_t motion_type);
    ~MotionTrack();

    // get members
    motion_type_t get_motion_type() const          { return m_motion_type; }
    MotionTrack::keyframes_t get_keyframes() const { return m_keyframes; }

    // insert / erase / lerp
    void insert_keyframe(int frame_number, Keyframe* keyframe);
    void erase_keyframe(int frame_number);
    void lerp_frame(int frame_number, glm::vec3* value) const;

    // util
    bool get_frame_range(int* start_frame_number, int* end_frame_number) const;

private:
    motion_type_t m_motion_type;
    keyframes_t   m_keyframes;
};

class ObjectScript
{
public:
    typedef std::map<unsigned char, MotionTrack*> motion_tracks_t;

    ObjectScript();
    ~ObjectScript();

    // get members
    motion_tracks_t get_motion_track() const { return m_motion_tracks; }

    // insert / erase / lerp
    void insert_keyframe(MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    void erase_keyframe(unsigned char motion_types, int frame_number);
    void lerp_frame_for_motion_track(MotionTrack::motion_type_t motion_type, int frame_number, glm::vec3* value) const;

    // util
    bool get_frame_range(int* start_frame_number, int* end_frame_number) const;

private:
    motion_tracks_t m_motion_tracks;
};

class KeyframeMgr
{
public:
    typedef std::map<long, ObjectScript*> script_t;

    static KeyframeMgr* instance()
    {
        static KeyframeMgr keyframe_mgr;
        return &keyframe_mgr;
    }

    // insert / erase / lerp
    void insert_keyframe(long object_id, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    void erase_keyframe(long object_id, unsigned char motion_types, int frame_number = -1);
    bool lerp_frame_for_object_motion_track(long object_id, int frame_number, glm::vec3* origin, glm::vec3* orient, glm::vec3* scale) const;

    // util
    bool get_frame_range(long object_id, int* start_frame_number, int* end_frame_number) const;
    bool export_object_motion_track(long object_id, std::vector<glm::vec3>* origin_motion_track, std::vector<glm::vec3>* orient_motion_track) const;

private:
    KeyframeMgr();
    ~KeyframeMgr();

    script_t m_script;
};

}

#endif
