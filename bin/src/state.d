bin/src/state.o: src/state.c src/state.h src/util/aabb.h \
  src/util/macros.h src/util/types.h src/util/math.h src/config.h \
  src/util/log.h src/util/util.h lib/cglm/include/cglm/struct.h \
  lib/cglm/include/cglm/cglm.h lib/cglm/include/cglm/common.h \
  lib/cglm/include/cglm/types.h lib/cglm/include/cglm/simd/intrin.h \
  lib/cglm/include/cglm/simd/arm.h lib/cglm/include/cglm/vec2.h \
  lib/cglm/include/cglm/util.h lib/cglm/include/cglm/vec2-ext.h \
  lib/cglm/include/cglm/vec3.h lib/cglm/include/cglm/vec4.h \
  lib/cglm/include/cglm/vec4-ext.h lib/cglm/include/cglm/vec3-ext.h \
  lib/cglm/include/cglm/ivec2.h lib/cglm/include/cglm/ivec3.h \
  lib/cglm/include/cglm/ivec4.h lib/cglm/include/cglm/mat4.h \
  lib/cglm/include/cglm/simd/neon/mat4.h \
  lib/cglm/include/cglm/simd/neon/../../common.h \
  lib/cglm/include/cglm/simd/neon/../intrin.h \
  lib/cglm/include/cglm/mat3.h lib/cglm/include/cglm/mat2.h \
  lib/cglm/include/cglm/simd/neon/mat2.h lib/cglm/include/cglm/affine.h \
  lib/cglm/include/cglm/affine-mat.h \
  lib/cglm/include/cglm/simd/neon/affine.h \
  lib/cglm/include/cglm/affine-pre.h lib/cglm/include/cglm/affine-post.h \
  lib/cglm/include/cglm/cam.h lib/cglm/include/cglm/plane.h \
  lib/cglm/include/cglm/clipspace/persp.h \
  lib/cglm/include/cglm/clipspace/../common.h \
  lib/cglm/include/cglm/clipspace/../plane.h \
  lib/cglm/include/cglm/clipspace/../mat4.h \
  lib/cglm/include/cglm/clipspace/ortho_rh_no.h \
  lib/cglm/include/cglm/clipspace/persp_rh_no.h \
  lib/cglm/include/cglm/clipspace/view_rh_no.h \
  lib/cglm/include/cglm/clipspace/view_rh.h \
  lib/cglm/include/cglm/frustum.h lib/cglm/include/cglm/quat.h \
  lib/cglm/include/cglm/simd/neon/quat.h lib/cglm/include/cglm/euler.h \
  lib/cglm/include/cglm/box.h lib/cglm/include/cglm/color.h \
  lib/cglm/include/cglm/io.h lib/cglm/include/cglm/project.h \
  lib/cglm/include/cglm/clipspace/project_no.h \
  lib/cglm/include/cglm/clipspace/../vec3.h \
  lib/cglm/include/cglm/clipspace/../vec4.h \
  lib/cglm/include/cglm/sphere.h lib/cglm/include/cglm/ease.h \
  lib/cglm/include/cglm/curve.h lib/cglm/include/cglm/bezier.h \
  lib/cglm/include/cglm/ray.h lib/cglm/include/cglm/affine2d.h \
  lib/cglm/include/cglm/types-struct.h \
  lib/cglm/include/cglm/struct/vec2.h \
  lib/cglm/include/cglm/struct/../common.h \
  lib/cglm/include/cglm/struct/../types-struct.h \
  lib/cglm/include/cglm/struct/../util.h \
  lib/cglm/include/cglm/struct/../vec2.h \
  lib/cglm/include/cglm/struct/vec2-ext.h \
  lib/cglm/include/cglm/struct/../vec2-ext.h \
  lib/cglm/include/cglm/struct/vec3.h \
  lib/cglm/include/cglm/struct/../vec3.h \
  lib/cglm/include/cglm/struct/vec3-ext.h \
  lib/cglm/include/cglm/struct/../vec3-ext.h \
  lib/cglm/include/cglm/struct/vec4.h \
  lib/cglm/include/cglm/struct/../vec4.h \
  lib/cglm/include/cglm/struct/vec4-ext.h \
  lib/cglm/include/cglm/struct/../vec4-ext.h \
  lib/cglm/include/cglm/struct/mat2.h \
  lib/cglm/include/cglm/struct/../mat2.h \
  lib/cglm/include/cglm/struct/mat3.h \
  lib/cglm/include/cglm/struct/../mat3.h \
  lib/cglm/include/cglm/struct/mat4.h \
  lib/cglm/include/cglm/struct/../mat4.h \
  lib/cglm/include/cglm/struct/affine.h \
  lib/cglm/include/cglm/struct/../affine.h \
  lib/cglm/include/cglm/struct/affine-pre.h \
  lib/cglm/include/cglm/struct/affine-post.h \
  lib/cglm/include/cglm/struct/frustum.h \
  lib/cglm/include/cglm/struct/../frustum.h \
  lib/cglm/include/cglm/struct/plane.h \
  lib/cglm/include/cglm/struct/../plane.h \
  lib/cglm/include/cglm/struct/box.h \
  lib/cglm/include/cglm/struct/../box.h \
  lib/cglm/include/cglm/struct/color.h \
  lib/cglm/include/cglm/struct/../color.h \
  lib/cglm/include/cglm/struct/io.h lib/cglm/include/cglm/struct/../io.h \
  lib/cglm/include/cglm/struct/cam.h \
  lib/cglm/include/cglm/struct/../cam.h \
  lib/cglm/include/cglm/struct/clipspace/ortho_rh_no.h \
  lib/cglm/include/cglm/struct/clipspace/../../common.h \
  lib/cglm/include/cglm/struct/clipspace/../../types-struct.h \
  lib/cglm/include/cglm/struct/clipspace/../../plane.h \
  lib/cglm/include/cglm/struct/clipspace/../../cam.h \
  lib/cglm/include/cglm/struct/clipspace/persp_rh_no.h \
  lib/cglm/include/cglm/struct/clipspace/view_rh_no.h \
  lib/cglm/include/cglm/struct/quat.h \
  lib/cglm/include/cglm/struct/../quat.h \
  lib/cglm/include/cglm/struct/euler.h \
  lib/cglm/include/cglm/struct/../euler.h \
  lib/cglm/include/cglm/struct/project.h \
  lib/cglm/include/cglm/struct/../project.h \
  lib/cglm/include/cglm/struct/sphere.h \
  lib/cglm/include/cglm/struct/../sphere.h \
  lib/cglm/include/cglm/struct/curve.h \
  lib/cglm/include/cglm/struct/../curve.h \
  lib/cglm/include/cglm/struct/affine2d.h \
  lib/cglm/include/cglm/struct/../affine2d.h src/util/ivec2s.h \
  src/util/ivec2.h src/gfx/gfx.h src/gfx/sokol.h lib/sokol/sokol_gfx.h \
  lib/sokol_gp/sokol_gp.h lib/sokol/util/sokol_gl.h \
  lib/sokol/util/sokol_gfx_imgui.h src/gfx/sokol_gfx_ext.h \
  lib/sokol/util/sokol_imgui.h src/util/map.h src/util/assert.h \
  src/util/str.h src/util/dynlist.h src/reload.h \
  src/../util/reloadhost.h src/util/resource.h src/defs.h \
  src/util/enum.h src/shared_defs.h src/util/rand.h src/util/sound.h \
  src/gfx/atlas.h src/gfx/renderer.h src/gfx/dynbuf.h src/util/bitmap.h \
  src/util/dlist.h src/gfx/renderer_types.h src/level/level.h \
  src/level/level_defs.h src/util/llist.h src/level/io.h \
  src/editor/editor.h src/imgui.h lib/cimgui/cimgui.h src/level/lptr.h \
  src/util/color.h src/level/object.h src/util/file.h
src/state.h:
src/util/aabb.h:
src/util/macros.h:
src/util/types.h:
src/util/math.h:
src/config.h:
src/util/log.h:
src/util/util.h:
lib/cglm/include/cglm/struct.h:
lib/cglm/include/cglm/cglm.h:
lib/cglm/include/cglm/common.h:
lib/cglm/include/cglm/types.h:
lib/cglm/include/cglm/simd/intrin.h:
lib/cglm/include/cglm/simd/arm.h:
lib/cglm/include/cglm/vec2.h:
lib/cglm/include/cglm/util.h:
lib/cglm/include/cglm/vec2-ext.h:
lib/cglm/include/cglm/vec3.h:
lib/cglm/include/cglm/vec4.h:
lib/cglm/include/cglm/vec4-ext.h:
lib/cglm/include/cglm/vec3-ext.h:
lib/cglm/include/cglm/ivec2.h:
lib/cglm/include/cglm/ivec3.h:
lib/cglm/include/cglm/ivec4.h:
lib/cglm/include/cglm/mat4.h:
lib/cglm/include/cglm/simd/neon/mat4.h:
lib/cglm/include/cglm/simd/neon/../../common.h:
lib/cglm/include/cglm/simd/neon/../intrin.h:
lib/cglm/include/cglm/mat3.h:
lib/cglm/include/cglm/mat2.h:
lib/cglm/include/cglm/simd/neon/mat2.h:
lib/cglm/include/cglm/affine.h:
lib/cglm/include/cglm/affine-mat.h:
lib/cglm/include/cglm/simd/neon/affine.h:
lib/cglm/include/cglm/affine-pre.h:
lib/cglm/include/cglm/affine-post.h:
lib/cglm/include/cglm/cam.h:
lib/cglm/include/cglm/plane.h:
lib/cglm/include/cglm/clipspace/persp.h:
lib/cglm/include/cglm/clipspace/../common.h:
lib/cglm/include/cglm/clipspace/../plane.h:
lib/cglm/include/cglm/clipspace/../mat4.h:
lib/cglm/include/cglm/clipspace/ortho_rh_no.h:
lib/cglm/include/cglm/clipspace/persp_rh_no.h:
lib/cglm/include/cglm/clipspace/view_rh_no.h:
lib/cglm/include/cglm/clipspace/view_rh.h:
lib/cglm/include/cglm/frustum.h:
lib/cglm/include/cglm/quat.h:
lib/cglm/include/cglm/simd/neon/quat.h:
lib/cglm/include/cglm/euler.h:
lib/cglm/include/cglm/box.h:
lib/cglm/include/cglm/color.h:
lib/cglm/include/cglm/io.h:
lib/cglm/include/cglm/project.h:
lib/cglm/include/cglm/clipspace/project_no.h:
lib/cglm/include/cglm/clipspace/../vec3.h:
lib/cglm/include/cglm/clipspace/../vec4.h:
lib/cglm/include/cglm/sphere.h:
lib/cglm/include/cglm/ease.h:
lib/cglm/include/cglm/curve.h:
lib/cglm/include/cglm/bezier.h:
lib/cglm/include/cglm/ray.h:
lib/cglm/include/cglm/affine2d.h:
lib/cglm/include/cglm/types-struct.h:
lib/cglm/include/cglm/struct/vec2.h:
lib/cglm/include/cglm/struct/../common.h:
lib/cglm/include/cglm/struct/../types-struct.h:
lib/cglm/include/cglm/struct/../util.h:
lib/cglm/include/cglm/struct/../vec2.h:
lib/cglm/include/cglm/struct/vec2-ext.h:
lib/cglm/include/cglm/struct/../vec2-ext.h:
lib/cglm/include/cglm/struct/vec3.h:
lib/cglm/include/cglm/struct/../vec3.h:
lib/cglm/include/cglm/struct/vec3-ext.h:
lib/cglm/include/cglm/struct/../vec3-ext.h:
lib/cglm/include/cglm/struct/vec4.h:
lib/cglm/include/cglm/struct/../vec4.h:
lib/cglm/include/cglm/struct/vec4-ext.h:
lib/cglm/include/cglm/struct/../vec4-ext.h:
lib/cglm/include/cglm/struct/mat2.h:
lib/cglm/include/cglm/struct/../mat2.h:
lib/cglm/include/cglm/struct/mat3.h:
lib/cglm/include/cglm/struct/../mat3.h:
lib/cglm/include/cglm/struct/mat4.h:
lib/cglm/include/cglm/struct/../mat4.h:
lib/cglm/include/cglm/struct/affine.h:
lib/cglm/include/cglm/struct/../affine.h:
lib/cglm/include/cglm/struct/affine-pre.h:
lib/cglm/include/cglm/struct/affine-post.h:
lib/cglm/include/cglm/struct/frustum.h:
lib/cglm/include/cglm/struct/../frustum.h:
lib/cglm/include/cglm/struct/plane.h:
lib/cglm/include/cglm/struct/../plane.h:
lib/cglm/include/cglm/struct/box.h:
lib/cglm/include/cglm/struct/../box.h:
lib/cglm/include/cglm/struct/color.h:
lib/cglm/include/cglm/struct/../color.h:
lib/cglm/include/cglm/struct/io.h:
lib/cglm/include/cglm/struct/../io.h:
lib/cglm/include/cglm/struct/cam.h:
lib/cglm/include/cglm/struct/../cam.h:
lib/cglm/include/cglm/struct/clipspace/ortho_rh_no.h:
lib/cglm/include/cglm/struct/clipspace/../../common.h:
lib/cglm/include/cglm/struct/clipspace/../../types-struct.h:
lib/cglm/include/cglm/struct/clipspace/../../plane.h:
lib/cglm/include/cglm/struct/clipspace/../../cam.h:
lib/cglm/include/cglm/struct/clipspace/persp_rh_no.h:
lib/cglm/include/cglm/struct/clipspace/view_rh_no.h:
lib/cglm/include/cglm/struct/quat.h:
lib/cglm/include/cglm/struct/../quat.h:
lib/cglm/include/cglm/struct/euler.h:
lib/cglm/include/cglm/struct/../euler.h:
lib/cglm/include/cglm/struct/project.h:
lib/cglm/include/cglm/struct/../project.h:
lib/cglm/include/cglm/struct/sphere.h:
lib/cglm/include/cglm/struct/../sphere.h:
lib/cglm/include/cglm/struct/curve.h:
lib/cglm/include/cglm/struct/../curve.h:
lib/cglm/include/cglm/struct/affine2d.h:
lib/cglm/include/cglm/struct/../affine2d.h:
src/util/ivec2s.h:
src/util/ivec2.h:
src/gfx/gfx.h:
src/gfx/sokol.h:
lib/sokol/sokol_gfx.h:
lib/sokol_gp/sokol_gp.h:
lib/sokol/util/sokol_gl.h:
lib/sokol/util/sokol_gfx_imgui.h:
src/gfx/sokol_gfx_ext.h:
lib/sokol/util/sokol_imgui.h:
src/util/map.h:
src/util/assert.h:
src/util/str.h:
src/util/dynlist.h:
src/reload.h:
src/../util/reloadhost.h:
src/util/resource.h:
src/defs.h:
src/util/enum.h:
src/shared_defs.h:
src/util/rand.h:
src/util/sound.h:
src/gfx/atlas.h:
src/gfx/renderer.h:
src/gfx/dynbuf.h:
src/util/bitmap.h:
src/util/dlist.h:
src/gfx/renderer_types.h:
src/level/level.h:
src/level/level_defs.h:
src/util/llist.h:
src/level/io.h:
src/editor/editor.h:
src/imgui.h:
lib/cimgui/cimgui.h:
src/level/lptr.h:
src/util/color.h:
src/level/object.h:
src/util/file.h:
