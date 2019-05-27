[![Build Status](https://secure.travis-ci.org/onlyuser/dexvt-lite.png)](http://travis-ci.org/onlyuser/dexvt-lite)

3D Inverse Kinematics with Constraints and... GPU Ray Tracing!
==============================================================

Copyright (C) 2011-2017 <mailto:onlyuser@gmail.com>

About
-----

dexvt-lite is a 3D inverse kinematics and GPU ray tracing tech demonstrator.
It features rovolute joint constraints, prismatic joint constraints, end-effector orientation constraints, and... GPU ray tracing!

See sister project demonstrating other engine features: [dexvt-test](https://github.com/onlyuser/dexvt-test)

Algorithm
---------

**3D Inverse Kinematics**

Supports rovolute joint constraints, prismatic joint constraints, and end-effector orientation constraints.
The way each constraint type is handled is described below:

* Revolute Joint Constraints
    * "Non-hinge" joints
        1. In each joint's rotation step, rotate on a pivot that aligns the end-effector with the target.
        2. Enforce joint constraints in euler space by capping the post-rotation orientation of the adjacent segment to be within the joint's maximal deviation from neutral orientation. Do this for each axis.
    * "Hinge" joints
        1. In each joint's rotation step, rotate on a pivot that minimizes the distance between the end-effector and the target, all the while staying within the joint's plane of free rotation. Setting non-constraint-violating goals early on avoids most problems described [here](https://stackoverflow.com/questions/21373012/best-inverse-kinematics-algorithm-with-constraints-on-joint-angles).
        2. Enforce joint constraints perpendicular to the joint's pivot by squeezing the post-rotation orientation of the adjacent segment to be within the joint's plane of free rotation. This adds numerical stability to the pivot so it doesn't "drift".
        3. Enforce joint constraints within the joint's plane of free rotation by capping the post-rotation orientation of the adjacent segment to be within the joint's maximal deviation from neutral orientation.

* Prismatic Joint Constraints
    * In each joint's translation step, slide the adjacent segment by the maximal extent it can slide within the joint's range of free translation to align the end-effector with the target.

* End-effector Orientation Constraints
    * In each joint's rotation/translation step, extend the end-effector position by the end-effector orientation offset.

**GPU Ray Tracing**

* Supports 4 material types:
    * Reflective
    * Transparent
    * Diffuse
    * Glowing

* Supports 3 primitive types:
    * Spheres
    * Planes
    * Boxes

Screenshots
-----------

[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ik_const_thumb.gif?attredirects=0)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ik_const.gif?attredirects=0)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ray_tracer_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ray_tracer.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ray_tracer2_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ray_tracer2.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ray_tracer3_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ray_tracer3.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ray_tracer4_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ray_tracer4.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ray_tracer_old_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ray_tracer_old.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_ray_tracer_old2_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_ray_tracer_old2.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_boids_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_boids.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_gimbal_lock_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_gimbal_lock.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_hexapod_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_hexapod.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_path_hexapod_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_path_hexapod.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_rail_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_rail.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_path_stewart_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_path_stewart.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_spider_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_spider.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_spider_wireframe_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_spider_wireframe.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_path_deltabot_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_path_deltabot.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_path_3dprinter_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_path_3dprinter.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_path_fanta_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_path_fanta.png)
[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-lite_path_fanta_wireframe_thumb.png)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-lite_path_fanta_wireframe.png)

Requirements
------------

Unix tools and 3rd party components (accessible from $PATH):

    gcc mesa-common-dev freeglut3-dev libglew-dev libglm-dev libpng-dev curl imagemagick

Resource files not included:

<table>
    <tr>
        <th>resource files</th>
        <th>purpose</th>
    </tr>
    <tr>
        <td>
            data/SaintPetersSquare2/posx.png<br>
            data/SaintPetersSquare2/negx.png<br>
            data/SaintPetersSquare2/posy.png<br>
            data/SaintPetersSquare2/negy.png<br>
            data/SaintPetersSquare2/posz.png<br>
            data/SaintPetersSquare2/negz.png
        </td>
        <td>Cube map texture (6 faces)</td>
    </tr>
    <tr>
        <td>data/chesterfield_color.png</td>
        <td>Color map texture</td>
    </tr>
    <tr>
        <td>data/chesterfield_normal.png</td>
        <td>Normal map texture</td>
    </tr>
</table>

Make Targets
------------

<table>
    <tr><th> target          </th><th> action                        </th></tr>
    <tr><td> all             </td><td> make binaries                 </td></tr>
    <tr><td> test            </td><td> all + run tests               </td></tr>
    <tr><td> clean           </td><td> remove all intermediate files </td></tr>
    <tr><td> lint            </td><td> perform cppcheck              </td></tr>
    <tr><td> docs            </td><td> make doxygen documentation    </td></tr>
    <tr><td> resources       </td><td> download resource files       </td></tr>
    <tr><td> clean_lint      </td><td> remove cppcheck results       </td></tr>
    <tr><td> clean_docs      </td><td> remove doxygen documentation  </td></tr>
    <tr><td> clean_resources </td><td> remove resource files         </td></tr>
</table>

Controls
--------

Keyboard:

<table>
    <tr><th> key         </th><th> purpose                 </th></tr>
    <tr><td> b           </td><td> toggle bounding-box     </td></tr>
    <tr><td> f           </td><td> toggle frame rate       </td></tr>
    <tr><td> g           </td><td> toggle guide wires      </td></tr>
    <tr><td> h           </td><td> toggle HUD              </td></tr>
    <tr><td> l           </td><td> toggle lights           </td></tr>
    <tr><td> n           </td><td> toggle normals          </td></tr>
    <tr><td> p           </td><td> toggle ortho-projection </td></tr>
    <tr><td> t           </td><td> toggle texture          </td></tr>
    <tr><td> w           </td><td> toggle wireframe        </td></tr>
    <tr><td> x           </td><td> toggle axis             </td></tr>
    <tr><td> z           </td><td> toggle labels           </td></tr>
    <tr><td> up/down     </td><td> pitch                   </td></tr>
    <tr><td> left/right  </td><td> yaw                     </td></tr>
    <tr><td> pg-up/pg-dn </td><td> roll                    </td></tr>
    <tr><td> home        </td><td> toggle target           </td></tr>
    <tr><td> space       </td><td> toggle animation        </td></tr>
    <tr><td> esc         </td><td> exit                    </td></tr>
</table>

Mouse:

<table>
    <tr><th> mouse-button + drag </th><th> purpose </th></tr>
    <tr><td> left                </td><td> orbit   </td></tr>
    <tr><td> right               </td><td> zoom    </td></tr>
</table>

References
----------

<dl>
    <dt>"Setting up an OpenGL development environment in Ubuntu Linux"</dt>
    <dd>http://www.codeproject.com/Articles/182109/Setting-up-an-OpenGL-development-environment-in-Ub</dd>
    <dt>"OpenGL Programming"</dt>
    <dd>http://en.wikibooks.org/wiki/OpenGL_Programming</dd>
    <dt>"Tutorial - Getting Started with the OpenGL Shading Language (GLSL)"</dt>
    <dd>http://joshbeam.com/articles/getting_started_with_glsl/</dd>
    <dt>"Cube Maps: Sky Boxes and Environment Mapping"</dt>
    <dd>http://antongerdelan.net/opengl/cubemaps.html</dd>
    <dt>"3D C/C++ tutorials - OpenGL - GLSL cube mapping"</dt>
    <dd>http://www.belanecbn.sk/3dtutorials/index.php?id=24</dd>
    <dt>"Skyboxes using glsl Version 330"</dt>
    <dd>http://gamedev.stackexchange.com/questions/60313/skyboxes-using-glsl-version-330</dd>
    <dt>"Rioki's Corner - GLSL Skybox"</dt>
    <dd>http://www.rioki.org/2013/03/07/glsl-skybox.html</dd>
    <dt>"How would you implement chromatic aberration?"</dt>
    <dd>http://gamedev.stackexchange.com/questions/58408/how-would-you-implement-chromatic-aberration</dd>
    <dt>"How do you calculate the angle between two normals in glsl?"</dt>
    <dd>http://stackoverflow.com/questions/338762/how-do-you-calculate-the-angle-between-two-normals-in-glsl</dd>
    <dt>"Chapter 7. Environment Mapping Techniques"</dt>
    <dd>http://http.developer.nvidia.com/CgTutorial/cg_tutorial_chapter07.html</dd>
    <dt>"Humus Cube Map Textures - Colosseum"</dt>
    <dd>http://www.humus.name/index.php?page=Textures</dd>
    <dt>"Kay's Blog Texture and game development freebies! - Well Preserved Chesterfield"</dt>
    <dd>http://kay-vriend.blogspot.tw/2012/11/well-preserved-chesterfield.html</dd>
    <dt>"Philip Rideout's OpenGL Bloom Tutorial"</dt>
    <dd>http://prideout.net/archive/bloom/</dd>
    <dt>"An investigation of fast real-time GPU-based image blur algorithms"</dt>
    <dd>https://software.intel.com/en-us/blogs/2014/07/15/an-investigation-of-fast-real-time-gpu-based-image-blur-algorithms</dd>
    <dt>"mattdesl/lwjgl-basics - ShaderLesson5"</dt>
    <dd>https://github.com/mattdesl/lwjgl-basics/wiki/ShaderLesson5</dd>
    <dt>"Jam3/glsl-fast-gaussian-blur - glsl-fast-gaussian-blur"</dt>
    <dd>https://github.com/Jam3/glsl-fast-gaussian-blur</dd>
    <dt>"john-chapman-graphics SSAO Tutorial"</dt>
    <dd>http://john-chapman-graphics.blogspot.tw/2013/01/ssao-tutorial.html</dd>
    <dt>"Know your SSAO artifacts"</dt>
    <dd>http://mtnphil.wordpress.com/2013/06/26/know-your-ssao-artifacts/</dd>
    <dt>"songho.ca OpenGL Projection Matrix"</dt>
    <dd>http://www.songho.ca/opengl/gl_projectionmatrix.html</dd>
    <dt>"3D MeshWorks"</dt>
    <dd>http://www.jrbassett.com</dd>
    <dt>"Missing gtc/constants.hpp #12"</dt>
    <dd>https://github.com/g-truc/glm/issues/12</dd>
    <dt>"Kinematics (Advanced Methods in Computer Graphics) Part 4"</dt>
    <dd>http://what-when-how.com/advanced-methods-in-computer-graphics/kinematics-advanced-methods-in-computer-graphics-part-4/</dd>
    <dt>"alfanick's inverse-kinematics repo (ccd.cpp)"</dt>
    <dd>https://github.com/alfanick/inverse-kinematics/blob/master/ccd.cpp</dd>
    <dt>"Best Inverse kinematics algorithm with constraints on joint angles"</dt>
    <dd>https://stackoverflow.com/questions/21373012/best-inverse-kinematics-algorithm-with-constraints-on-joint-angles</dd>
    <dt>"Real-Time 3d Robotics UI (Python)"</dt>
    <dd>https://coreykruger.wordpress.com/programming-and-scripting/462-2/</dd>
    <dt>"Anand's blog - GPGPU In Android: Load a texture with floats and read it back"</dt>
    <dd>http://www.anandmuralidhar.com/blog/tag/gpgpu/</dd>
    <dt>"Ray-Plane and Ray-Disk Intersection"</dt>
    <dd>https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection</dd>
    <dt>"Ray-Sphere Intersection"</dt>
    <dd>https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection</dd>
</dl>

Keywords
--------

    3D inverse kinematics (cyclic coordinate descent), forward kinematics, rovolute joint constraints, prismatic joint constraints, end-effector orientation constraints, GPU ray tracing
