/** Copyright (C) 2008-2014 Josh Ventura, Robert B. Colton, DatZach, Polygone, Harijs Grinbergs
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "../General/OpenGLHeaders.h"
#include "../General/GSd3d.h"
#include "../General/GStextures.h"
#include "../General/GLTextureStruct.h"
#include "../General/GSmatrix.h"
#include "GLSLshader.h"
#include "Universal_System/var4.h"
#include "Universal_System/roomsystem.h"
#include "Bridges/General/GL3Context.h"
#include <math.h>

using namespace std;

#define __GETR(x) ((x & 0x0000FF))/255.0
#define __GETG(x) ((x & 0x00FF00)>>8)/255.0
#define __GETB(x) ((x & 0xFF0000)>>16)/255.0

#include <floatcomp.h>

namespace enigma {
  bool d3dMode = false;
  bool d3dHidden = false;
  bool d3dZWriteEnable = true;
  int d3dCulling = 0;
  extern unsigned bound_shader;
  extern vector<enigma::ShaderProgram*> shaderprograms;
}

GLenum renderstates[6] = {
  GL_FRONT, GL_BACK, GL_FRONT_AND_BACK,
  GL_NICEST, GL_FASTEST, GL_DONT_CARE
};

GLenum fogmodes[3] = {
  GL_EXP, GL_EXP2, GL_LINEAR
};

GLenum depthoperators[8] = {
  GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
  GL_GEQUAL, GL_ALWAYS
};

GLenum fillmodes[3] = {
  GL_POINT, GL_LINE, GL_FILL
};

GLenum cullingstates[3] = {
  0, GL_CW, GL_CCW
};

namespace enigma_user
{

void d3d_depth_clear() {
  d3d_depth_clear_value(1.0f);
}

void d3d_depth_clear_value(float value) {
  glClearDepthf(value);
}

void d3d_start()
{
  // Set global ambient lighting to nothing.
  float global_ambient[] = { 0.0f, 0.0f, 0.0f, 0.0f };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

  // Enable depth buffering
  enigma::d3dMode = true;
  enigma::d3dHidden = true;
  enigma::d3dZWriteEnable = true;
  enigma::d3dCulling = rs_none;
  glDepthMask(true);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_NOTEQUAL, 0);

  // Set up projection matrix
  d3d_set_projection_perspective(0, 0, view_wview[view_current], view_hview[view_current], 0);
  //enigma::projection_matrix.InitPersProjTransform(45, -view_wview[view_current] / (double)view_hview[view_current], 1, 32000);

  // Set up modelview matrix
  d3d_transform_set_identity();
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void d3d_end()
{
  enigma::d3dMode = false;
  enigma::d3dHidden = false;
  enigma::d3dZWriteEnable = false;
  enigma::d3dCulling = rs_none;
  glDepthMask(false);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_ALPHA_TEST);
  d3d_set_projection_ortho(0, 0, view_wview[view_current], view_hview[view_current], 0); //This should probably be changed not to use views
  //enigma::projection_matrix.InitIdentity();
}

// disabling hidden surface removal in means there is no depth buffer
void d3d_set_hidden(bool enable)
{
    (enable?glEnable:glDisable)(GL_DEPTH_TEST);
    enigma::d3dHidden = enable;
	d3d_set_zwriteenable(enable);
}

// disabling zwriting can let you turn off testing for a single model, for instance
// to fix cutout billboards such as trees the alpha pixels on their edges may not depth sort
// properly particle effects are usually drawn with zwriting disabled because of this as well
void d3d_set_zwriteenable(bool enable)
{
	glDepthMask(enable);
	enigma::d3dZWriteEnable = enable;
}

void d3d_set_fog(bool enable, int color, double start, double end)
{
  d3d_set_fog_enabled(enable);
  d3d_set_fog_color(color);
  d3d_set_fog_start(start);
  d3d_set_fog_end(end);
  d3d_set_fog_hint(rs_nicest);
  d3d_set_fog_mode(rs_linear);
}//NOTE: fog can use vertex checks with less good graphic cards which screws up large textures (however this doesn't happen in directx)

void d3d_set_fog_enabled(bool enable)
{
  (enable?glEnable:glDisable)(GL_FOG);
}

void d3d_set_fog_mode(int mode)
{
  glFogi(GL_FOG_MODE, fogmodes[mode]);
}

void d3d_set_fog_hint(int mode) {
  glHint(GL_FOG_HINT, mode);
}

void d3d_set_fog_color(int color)
{
   GLfloat fog_color[3];
   fog_color[0] = __GETR(color);
   fog_color[1] = __GETG(color);
   fog_color[2] = __GETB(color);
   glFogfv(GL_FOG_COLOR, fog_color);
}

void d3d_set_fog_start(double start)
{
  glFogf(GL_FOG_START, start);
}

void d3d_set_fog_end(double end)
{
  glFogf(GL_FOG_END, end);
}

void d3d_set_fog_density(double density)
{
  glFogf(GL_FOG_DENSITY, density);
}

void d3d_set_culling(int mode)
{
  enigma::d3dCulling = mode;
  oglmgr->SetEnabled(GL_CULL_FACE, mode > 0);
  glFrontFace(cullingstates[mode]);
}

bool d3d_get_mode()
{
    return enigma::d3dMode;
}

bool d3d_get_hidden() {
	return enigma::d3dHidden;
}

int d3d_get_culling() {
	return enigma::d3dCulling;
}

void d3d_set_fill_mode(int fill)
{
  glPolygonMode(GL_FRONT_AND_BACK, fillmodes[fill]);
}

void d3d_set_line_width(float value) {
  glLineWidth(value);
}

void d3d_set_point_size(float value) {
  glPointSize(value);
}

void d3d_set_depth_operator(int mode) {
  glDepthFunc(depthoperators[mode]);
}

void d3d_set_depth(double dep)
{

}//TODO: Write function

void d3d_set_shading(bool smooth)
{
    glShadeModel(smooth?GL_SMOOTH:GL_FLAT);
}

void d3d_set_clip_plane(bool enable)
{
    (enable?glEnable:glDisable)(GL_CLIP_DISTANCE0);
}

}

#include <map>
#include <list>
#include "Universal_System/fileio.h"

struct light3D {
    int type; //0 - directional, 1 - positional
    bool enabled;
    gs_scalar position[4];
    float diffuse[4];
    float specular[4];
    float ambient[4];
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;

    light3D()
    {
        type = 0;
        enabled = false;
        position[0]=0, position[1]=0, position[2]=0, position[3]=0;
        diffuse[0]=0, diffuse[1]=0, diffuse[2]=0, diffuse[3]=0;
        specular[0]=0, specular[1]=0, specular[2]=0, specular[3]=0;
        ambient[0]=0, ambient[1]=0, ambient[2]=0, ambient[3]=0;
        constant_attenuation = 0;
        linear_attenuation = 0;
        quadratic_attenuation = 0;
    };
};

struct material3D {
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float shininess;

    material3D()
    {
        ambient[0] = 0.2, ambient[1] = 0.2, ambient[2] = 0.2, ambient[3] = 1.0;
        diffuse[0] = 0.8, diffuse[1] = 0.8, diffuse[2] = 0.8, diffuse[3] = 1.0;
        specular[0] = 0.0, specular[1] = 0.0, specular[2] = 0.0, specular[3] = 0.0;
        shininess = 0.0;
    }
};

class d3d_lights
{
    vector<light3D> lights;
    material3D material;
    bool lights_enabled;

    public:
    float global_ambient_color[4];
    d3d_lights() {
        lights_enabled = false;
        global_ambient_color[0] = global_ambient_color[1] = global_ambient_color[2] = 0.2f;
        global_ambient_color[3] = 1.0f;
        for (unsigned int i=0; i<8; ++i){
            lights.push_back(light3D());
        }
    }
    //~d3d_lights() {}

    void lights_enable(bool enable){
        lights_enabled = enable;
    }

    void light_update()
    {
        glUniform1i(enigma::shaderprograms[enigma::bound_shader]->uni_lightEnable, lights_enabled);
        if (lights_enabled == true){
            glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_ambient_color, 1, global_ambient_color);
            glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_material_ambient, 1, material.ambient);
            glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_material_diffuse, 1, material.diffuse);
            glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_material_specular, 1, material.specular);
            glUniform1f(enigma::shaderprograms[enigma::bound_shader]->uni_material_shininess, material.shininess);
            unsigned int al = 0; //Active lights
            for (unsigned int i=0; i<lights.size(); ++i){
                if (lights[i].enabled == true){
                    enigma::mv_matrix.Print();
                    enigma::Vector4 lpos_eyespace = enigma::mv_matrix * enigma::Vector4(lights[i].position[0],lights[i].position[1],lights[i].position[2],1.0);
                    gs_scalar tmp_pos[4] = {lpos_eyespace.x,lpos_eyespace.y,lpos_eyespace.z,lights[i].position[3]};
                    //printf("Light position after:\nx = %f; y = %f; z = %f;\n", lpos_eyespace.x,lpos_eyespace.y,lpos_eyespace.z);

                    glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_light_position[i], 1, tmp_pos);
                    glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_light_ambient[i], 1, lights[i].ambient);
                    glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_light_diffuse[i], 1, lights[i].diffuse);
                    glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_light_specular[i], 1, lights[i].specular);
                    ++al;
                }
            }
            glUniform1i(enigma::shaderprograms[enigma::bound_shader]->uni_light_active, al);
        }
    }

    void light_update_positions()
    {
        for (unsigned int i=0; i<lights.size(); ++i){
            if (lights[i].enabled == true){
                //printf("model_view matrix:\n");
                //enigma::mv_matrix.Print();
                //printf("Light position before:\n");
                //printf("x = %f; y = %f; z = %f;\n", lights[i].position[0],lights[i].position[1],lights[i].position[2]);
                enigma::Vector4 lpos_eyespace = enigma::mv_matrix * enigma::Vector4(lights[i].position[0],lights[i].position[1],lights[i].position[2],1.0);
                //printf("Light position after:\nx = %f; y = %f; z = %f;\n", lpos_eyespace.x,lpos_eyespace.y,lpos_eyespace.z);
                gs_scalar tmp_pos[4] = {lpos_eyespace.x,lpos_eyespace.y,lpos_eyespace.z,lights[i].position[3]};
                glUniform4fv(enigma::shaderprograms[enigma::bound_shader]->uni_light_position[i], 1, tmp_pos);
            }
        }
    }

    bool light_define_direction(unsigned int id, gs_scalar dx, gs_scalar dy, gs_scalar dz, int col)
    {
        if (id<lights.size()){
            lights[id].type = 0;
            lights[id].position[0] = -dx;
            lights[id].position[1] = -dy;
            lights[id].position[2] = -dz;
            lights[id].position[3] = 0.0;
            lights[id].diffuse[0] = __GETR(col);
            lights[id].diffuse[1] = __GETG(col);
            lights[id].diffuse[2] = __GETB(col);
            lights[id].diffuse[3] = 1.0f;
            light_update();
            return true;
        }
        return false;
    }

    bool light_define_point(unsigned int id, gs_scalar x, gs_scalar y, gs_scalar z, gs_scalar range, int col)
    {
        if (range <= 0.0) {
            return false;
        }
        if (id<lights.size()){
            lights[id].type = 1;
            lights[id].position[0] = x;
            lights[id].position[1] = y;
            lights[id].position[2] = z;
            lights[id].position[3] = range;
            lights[id].diffuse[0] = __GETR(col);
            lights[id].diffuse[1] = __GETG(col);
            lights[id].diffuse[2] = __GETB(col);
            lights[id].diffuse[3] = 1.0f;
            lights[id].specular[0] = 0.0f;
            lights[id].specular[1] = 0.0f;
            lights[id].specular[2] = 0.0f;
            lights[id].specular[3] = 0.0f;
            lights[id].ambient[0] = 0.0f;
            lights[id].ambient[1] = 0.0f;
            lights[id].ambient[2] = 0.0f;
            lights[id].ambient[3] = 0.0f;
            lights[id].constant_attenuation = 1.0f;
            lights[id].linear_attenuation = 0.0f;
            lights[id].quadratic_attenuation = 8.0f/(range*range);
            light_update();
            return true;
        }
        return false;
    }

    bool light_set_specularity(unsigned int id, gs_scalar r, gs_scalar g, gs_scalar b, gs_scalar a)
    {
        if (id<lights.size()){
            lights[id].specular[0] = r;
            lights[id].specular[1] = g;
            lights[id].specular[2] = b;
            lights[id].specular[3] = a;
            light_update();
            return true;
        }
        return false;
    }

    bool light_set_ambient(unsigned int id, gs_scalar r, gs_scalar g, gs_scalar b, gs_scalar a)
    {
        if (id<lights.size()){
            lights[id].ambient[0] = r;
            lights[id].ambient[1] = g;
            lights[id].ambient[2] = b;
            lights[id].ambient[3] = a;
            light_update();
            return true;
        }
        return false;
    }


    bool light_enable(unsigned int id)
    {
        if (id<lights.size()){
            lights[id].enabled = true;
            light_update();
            return true;
        }
        return false;
    }

    bool light_disable(unsigned int id)
    {
        if (id<lights.size()){
            lights[id].enabled = false;
            light_update();
            return true;
        }
        return false;
    }
} d3d_lighting;

namespace enigma_user
{

bool d3d_light_define_direction(int id, gs_scalar dx, gs_scalar dy, gs_scalar dz, int col)
{
    return d3d_lighting.light_define_direction(id, dx, dy, dz, col);
}

bool d3d_light_define_point(int id, gs_scalar x, gs_scalar y, gs_scalar z, double range, int col)
{
    return d3d_lighting.light_define_point(id, x, y, z, range, col);
}

bool d3d_light_set_specularity(int id, int r, int g, int b, double a)
{
    return d3d_lighting.light_set_specularity(id, (gs_scalar)r/255.0, (gs_scalar)g/255.0, (gs_scalar)b/255.0, a);
}

bool d3d_light_set_ambient(int id, int r, int g, int b, double a)
{
    return d3d_lighting.light_set_ambient(id, (gs_scalar)r/255.0, (gs_scalar)g/255.0, (gs_scalar)b/255.0, a);
}

void d3d_light_define_ambient(int col)
{
    d3d_lighting.global_ambient_color[0] = __GETR(col);
    d3d_lighting.global_ambient_color[1] = __GETG(col);
    d3d_lighting.global_ambient_color[2] = __GETB(col);
    d3d_lighting.light_update();
}

bool d3d_light_enable(int id, bool enable)
{
    return enable?d3d_lighting.light_enable(id):d3d_lighting.light_disable(id);
}

void d3d_set_lighting(bool enable)
{
    d3d_lighting.lights_enable(enable);
    d3d_lighting.light_update();
}
}

namespace enigma {
    void d3d_light_update_positions()
    {
        d3d_lighting.light_update_positions();
    }
}
