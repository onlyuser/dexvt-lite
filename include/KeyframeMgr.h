// This file is part of dexvt-lite.
// -- 3D Inverse Kinematics (Cyclic Coordinate Descent) with Constraints
// Copyright (C) 2018 onlyuser <mailto:onlyuser@gmail.com>
//
// dexvt-lite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dexvt-lite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dexvt-lite.  If not, see <http://www.gnu.org/licenses/>.

#ifndef VT_KEYFRAMER_MGR_H_
#define VT_KEYFRAMER_MGR_H_

#include <glm/glm.hpp>
#include <map>
#include <vector>

namespace vt {

class Keyframe
{
public:
    Keyframe(glm::vec3 value, bool is_smooth = false, float prev_control_point_scale = 1, float next_control_point_scale = 1);

    // get members
    glm::vec3 get_value() const          { return m_value; }
    glm::vec3 get_control_point1() const { return m_control_point1; }
    glm::vec3 get_control_point2() const { return m_control_point2; }

    // util
    void update_control_points(glm::vec3 prev_point, glm::vec3 next_point, float control_point_scale);

private:
    glm::vec3 m_value;
    bool m_is_smooth;
    float m_prev_control_point_scale;
    float m_next_control_point_scale;
    glm::vec3 m_control_point1;
    glm::vec3 m_control_point2;
};

class MotionTrack
{
public:
    enum motion_type_t {
        MOTION_TYPE_ORIGIN = 1,
        MOTION_TYPE_EULER  = 2,
        MOTION_TYPE_SCALE  = 4
    };

    typedef std::map<int, Keyframe*> keyframes_t;

    MotionTrack(motion_type_t motion_type);
    ~MotionTrack();

    // get members
    motion_type_t get_motion_type() const          { return m_motion_type; }
    MotionTrack::keyframes_t get_keyframes() const { return m_keyframes; }

    // insert / erase / lerp
    bool insert_keyframe(int frame_number, Keyframe* keyframe);
    bool erase_keyframe(int frame_number);
    bool export_keyframe_values(std::vector<glm::vec3>* keyframe_values, bool include_control_points = false);
    bool interpolate_frame_value(int frame_number, glm::vec3* value, bool is_smooth = false) const;

    // util
    bool get_frame_number_range(int* start_frame_number, int* end_frame_number) const;
    bool update_control_points(float control_point_scale);

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
    bool export_keyframe_values_for_motion_track(MotionTrack::motion_type_t motion_type,
                                                 std::vector<glm::vec3>*    keyframe_values,
                                                 bool                       include_control_points = false);
    bool interpolate_frame_value_for_motion_track(MotionTrack::motion_type_t motion_type,
                                                  int                        frame_number,
                                                  glm::vec3*                 value,
                                                  bool                       is_smooth = false) const;

    // util
    bool get_frame_number_range(int* start_frame_number, int* end_frame_number) const;
    void update_control_points(float control_point_scale);

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
    bool insert_keyframe(long object_id, MotionTrack::motion_type_t motion_type, int frame_number, Keyframe* keyframe);
    bool erase_keyframe(long object_id, unsigned char motion_types, int frame_number = -1);
    bool export_keyframe_values_for_object(long                    object_id,
                                           std::vector<glm::vec3>* origin_keyframe_values,
                                           std::vector<glm::vec3>* euler_keyframe_values,
                                           std::vector<glm::vec3>* scale_keyframe_values,
                                           bool                    include_control_points = false);
    bool interpolate_frame_value_for_object(long       object_id,
                                            int        frame_number,
                                            glm::vec3* origin,
                                            glm::vec3* euler,
                                            glm::vec3* scale,
                                            bool       is_smooth = false) const;

    // util
    bool get_frame_number_range(long object_id, int* start_frame_number, int* end_frame_number) const;
    void update_control_points(float control_point_scale);
    bool export_frame_values_for_object(long                    object_id,
                                        std::vector<glm::vec3>* origin_frame_values,
                                        std::vector<glm::vec3>* euler_frame_values,
                                        std::vector<glm::vec3>* scale_frame_values,
                                        bool                    is_smooth = false) const;

    void clear();

private:
    KeyframeMgr() {}
    ~KeyframeMgr();

    script_t m_script;
};

}

#endif
