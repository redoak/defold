// Copyright 2020-2024 The Defold Foundation
// Copyright 2014-2020 King
// Copyright 2009-2014 Ragnar Svensson, Christian Murray
// Licensed under the Defold License version 1.0 (the "License"); you may not use
// this file except in compliance with the License.
//
// You may obtain a copy of the License, together with FAQs at
// https://www.defold.com/license
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "render_script.h"

#include <script/script.h>

namespace dmRender
{
    /*# Camera API documentation
     *
     * Camera functions, messages and constants.
     *
     * @document
     * @name Camera
     * @namespace camera
     */
    #define RENDER_SCRIPT_CAMERA_LIB_NAME "camera"

    struct RenderScriptCameraModule
    {
        dmRender::HRenderContext m_RenderContext;
    };

    static RenderScriptCameraModule g_RenderScriptCameraModule = { 0 };

    RenderCamera* CheckRenderCamera(lua_State* L, int index, HRenderContext render_context)
    {
        RenderCamera* camera = 0x0;

        if(lua_isnumber(L, index))
        {
            HRenderCamera h_camera = (HRenderCamera) lua_tonumber(L, index);
            camera = render_context->m_RenderCameras.Get(h_camera);

            if (!camera)
            {
                return (RenderCamera*) (uintptr_t) luaL_error(L, "Invalid handle.");
            }
        }
        else
        {
            dmMessage::URL url;
            if (dmScript::ResolveURL(L, index, &url, 0) != dmMessage::RESULT_OK)
            {
                return (RenderCamera*) (uintptr_t) luaL_error(L, "Could not resolve URL.");
            }

            camera = GetRenderCameraByUrl(g_RenderScriptCameraModule.m_RenderContext, url);
            if (camera == 0x0)
            {
                char buffer[256];
                dmScript::UrlToString(&url, buffer, sizeof(buffer));
                return (RenderCamera*) (uintptr_t) luaL_error(L, "Camera '%s' not found.", buffer);
            }
        }
        return camera;
    }

    /*# get all camera URLs
    * This function returns a table with all the camera URLs that have been
    * registered in the render context.
    *
    * @name camera.get_cameras
    * @param camera [type:url|handle|nil] camera id
    * @return cameras [type:table] a table with all camera URLs
    *
    * @examples
    * ```lua
    * for k,v in pairs(camera.get_cameras()) do
    *     render.set_camera(v)
    *     render.draw(...)
    *     render.set_camera()
    * end
    * ```
    */
    static int RenderScriptCamera_GetCameras(lua_State* L)
    {
        DM_LUA_STACK_CHECK(L, 1);

        int camera_index = 1;

        lua_newtable(L);

        for (int i = 0; i < g_RenderScriptCameraModule.m_RenderContext->m_RenderCameras.Capacity(); ++i)
        {
            RenderCamera* camera = g_RenderScriptCameraModule.m_RenderContext->m_RenderCameras.GetByIndex(i);

            if (camera)
            {
                lua_pushinteger(L, camera_index);
                dmScript::PushURL(L, camera->m_URL);
                lua_settable(L, -3);
                camera_index++;
            }
        }

        return 1;
    }

    /*# get camera info
    * Get the info for a specific camera by URL. The result is a table with the following fields:
    *
    * `url`
    * : [type:url] the URL of the camera.
    *
    * `projection`
    * : [type:vmath.matrix4] the projection matrix.
    *
    * `view`
    * : [type:vmath.matrix4] the view matrix.
    *
    * `handle`
    * : [type:number] the handle of the camera.
    *
    * `fov`
    * : [type:number] the field of view.
    *
    * `aspect_ratio`
    * : [type:number] the aspect ratio.
    *
    * `near_z`
    * : [type:number] the near z.
    *
    * `far_z`
    * : [type:number] the far z.
    *
    * `orthographic_projection`
    * : [type:boolean] true if the camera is using an orthographic projection.
    *
    * `auto_aspect_ratio`
    * : [type:boolean] true if the camera is using an automatic aspect ratio.
    *
    * @name camera.get_cameras
    * @param camera [type:url|handle|nil] camera id
    * @return cameras [type:table] a table with all camera URLs
    *
    * @examples
    * ```lua
    * local info = camera.get_info("main:/my_go#camera")
    * render.set_camera(info.handle)
    * ```
    */
    static int RenderScriptCamera_GetInfo(lua_State* L)
    {
        DM_LUA_STACK_CHECK(L, 1);
        RenderCamera* camera = CheckRenderCamera(L, 1, g_RenderScriptCameraModule.m_RenderContext);

        lua_newtable(L);
        dmScript::PushURL(L, camera->m_URL);
        lua_setfield(L, -2, "url");

        dmScript::PushMatrix4(L, camera->m_Projection);
        lua_setfield(L, -2, "projection");

        dmScript::PushMatrix4(L, camera->m_View);
        lua_setfield(L, -2, "view");

        lua_pushnumber(L, camera->m_Handle);
        lua_setfield(L, -2, "handle");

        // TODO: Since we don't have any way of changing this (yet), we don't expose it.
        //       The idea is to use a normalized viewport vector that can be set though the API
        //       and via the editor, but it is the next part of this feature.
        /*
        dmScript::PushVector4(L, camera->m_Data.m_Viewport);
        lua_setfield(L, -2, "viewport");
        */

    #define PUSH_NUMBER(name, param) \
        lua_pushnumber(L, camera->m_Data.param); \
        lua_setfield(L, -2, name);

        PUSH_NUMBER("fov",          m_Fov);
        PUSH_NUMBER("aspect_ratio", m_AspectRatio);
        PUSH_NUMBER("near_z",       m_NearZ);
        PUSH_NUMBER("far_z",        m_FarZ);
    #undef PUSH_NUMBER

    #define PUSH_BOOL(name, param) \
        lua_pushboolean(L, camera->m_Data.param); \
        lua_setfield(L, -2, name);

        PUSH_BOOL("orthographic_projection", m_OrthographicProjection);
        PUSH_BOOL("auto_aspect_ratio",       m_AutoAspectRatio);
    #undef PUSH_BOOL

        return 1;
    }

    /*# get projection matrix
    *
    * @name camera.get_projection
    * @param camera [type:url|handle|nil] camera id
    * @return projection [type:vmath.matrix4] the projection matrix.
    */
    static int RenderScriptCamera_GetProjection(lua_State* L)
    {
        DM_LUA_STACK_CHECK(L, 0);
        RenderCamera* camera = CheckRenderCamera(L, 1, g_RenderScriptCameraModule.m_RenderContext);
        dmScript::PushMatrix4(L, camera->m_Projection);
        return 1;
    }

    /*# get view matrix
    *
    * @name camera.get_view
    * @param camera [type:url|handle|nil] camera id
    * @return view [type:vmath.matrix4] the view matrix.
    */
    static int RenderScriptCamera_GetView(lua_State* L)
    {
        DM_LUA_STACK_CHECK(L, 0);
        RenderCamera* camera = CheckRenderCamera(L, 1, g_RenderScriptCameraModule.m_RenderContext);
        dmScript::PushMatrix4(L, camera->m_View);
        return 1;
    }

#define GET_CAMERA_DATA_PROPERTY_FN(param, lua_fn) \
    static int RenderScriptCamera_Get##param(lua_State* L) \
    { \
        DM_LUA_STACK_CHECK(L, 1); \
        RenderCamera* camera = CheckRenderCamera(L, 1, g_RenderScriptCameraModule.m_RenderContext); \
        lua_fn(L, camera->m_Data.m_##param); \
        return 1; \
    }

#define SET_CAMERA_DATA_PROPERTY_FN(param, lua_fn) \
    static int RenderScriptCamera_Set##param(lua_State* L) \
    { \
        DM_LUA_STACK_CHECK(L, 0); \
        RenderCamera* camera = CheckRenderCamera(L, 1, g_RenderScriptCameraModule.m_RenderContext); \
        camera->m_Data.m_##param = lua_fn(L, 2); \
        return 0; \
    }

    /*# get aspect ratio
    *
    * @name camera.get_aspect_ratio
    * @param camera [type:url|handle|nil] camera id
    * @return aspect_ratio [type:number] the aspect ratio.
    */
    GET_CAMERA_DATA_PROPERTY_FN(AspectRatio, lua_tonumber);

    /*# get far z
    *
    * @name camera.get_far_z
    * @param camera [type:url|handle|nil] camera id
    * @return far_z [type:number] the far z.
    */
    GET_CAMERA_DATA_PROPERTY_FN(FarZ, lua_tonumber);

    /*# get field of view
    *
    * @name camera.get_fov
    * @param camera [type:url|handle|nil] camera id
    * @return fov [type:number] the field of view.
    */
    GET_CAMERA_DATA_PROPERTY_FN(Fov, lua_tonumber);

    /*# get near z
    *
    * @name camera.get_near_z
    * @param camera [type:url|handle|nil] camera id
    * @return near_z [type:number] the near z.
    */
    GET_CAMERA_DATA_PROPERTY_FN(NearZ, lua_tonumber);

    /*# get orthographic zoom
    *
    * @name camera.get_orthographic_zoom
    * @param camera [type:url|handle|nil] camera id
    * @return orthographic_zoom [type:boolean] true if the camera is using an orthographic projection.
    */
    GET_CAMERA_DATA_PROPERTY_FN(OrthographicZoom, lua_toboolean);

    /*# set aspect ratio
    *
    * @name camera.set_aspect_ratio
    * @param camera [type:url|handle|nil] camera id
    * @param aspect_ratio [type:number] the aspect ratio.
    */
    SET_CAMERA_DATA_PROPERTY_FN(AspectRatio, lua_tonumber);

    /*# set far z
    *
    * @name camera.set_far_z
    * @param camera [type:url|handle|nil] camera id
    * @param far_z [type:number] the far z.
    */
    SET_CAMERA_DATA_PROPERTY_FN(FarZ, lua_tonumber);

    /*# set field of view
    *
    * @name camera.set_fov
    * @param camera [type:url|handle|nil] camera id
    * @param fov [type:number] the field of view.
    */
    SET_CAMERA_DATA_PROPERTY_FN(Fov, lua_tonumber);

    /*# set near z
    *
    * @name camera.set_near_z
    * @param camera [type:url|handle|nil] camera id
    * @param near_z [type:number] the near z.
    */
    SET_CAMERA_DATA_PROPERTY_FN(NearZ, lua_tonumber);

    /*# set orthographic zoom
    *
    * @name camera.set_orthographic_zoom
    * @param camera [type:url|handle|nil] camera id
    * @param orthographic_zoom [type:boolean] true if the camera is using an orthographic projection.
    */
    SET_CAMERA_DATA_PROPERTY_FN(OrthographicZoom, lua_toboolean);

#undef GET_CAMERA_DATA_PROPERTY_FN
#undef SET_CAMERA_DATA_PROPERTY_FN

    static const luaL_reg RenderScriptCamera_Methods[] =
    {
        {"get_cameras",             RenderScriptCamera_GetCameras},
        {"get_info",                RenderScriptCamera_GetInfo},

        // READ-ONLY
        {"get_projection",          RenderScriptCamera_GetProjection},
        {"get_view",                RenderScriptCamera_GetView},

        // READ-WRITE
        {"get_aspect_ratio",        RenderScriptCamera_GetAspectRatio},
        {"set_aspect_ratio",        RenderScriptCamera_SetAspectRatio},
        {"get_fov",                 RenderScriptCamera_GetFov},
        {"set_fov",                 RenderScriptCamera_SetFov},
        {"get_near_z",              RenderScriptCamera_GetNearZ},
        {"set_near_z",              RenderScriptCamera_SetNearZ},
        {"get_far_z",               RenderScriptCamera_GetFarZ},
        {"set_far_z",               RenderScriptCamera_SetFarZ},
        {"get_orthographic_zoom",   RenderScriptCamera_GetOrthographicZoom},
        {"set_orthographic_zoom",   RenderScriptCamera_SetOrthographicZoom},
        {0, 0}
    };

    void InitializeRenderScriptCameraContext(HRenderContext render_context, dmScript::HContext script_context)
    {
        lua_State* L = dmScript::GetLuaState(script_context);
        DM_LUA_STACK_CHECK(L, 0);

        luaL_register(L, RENDER_SCRIPT_CAMERA_LIB_NAME, RenderScriptCamera_Methods);

        lua_pop(L, 1);

        assert(g_RenderScriptCameraModule.m_RenderContext == 0x0);
        g_RenderScriptCameraModule.m_RenderContext = render_context;
    }

    void FinalizeRenderScriptCameraContext(HRenderContext render_context)
    {
        g_RenderScriptCameraModule.m_RenderContext = 0x0;
    }
}
