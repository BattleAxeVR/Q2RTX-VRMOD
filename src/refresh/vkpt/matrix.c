/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2003-2006 Andrey Nazarov

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "vkpt.h"

static void internal_create_entity_matrix(mat4_t matrix, entity_t *e, bool mirror)
{
	vec3_t axis[3];
	vec3_t origin;
	origin[0] = (1.f-e->backlerp) * e->origin[0] + e->backlerp * e->oldorigin[0];
	origin[1] = (1.f-e->backlerp) * e->origin[1] + e->backlerp * e->oldorigin[1];
	origin[2] = (1.f-e->backlerp) * e->origin[2] + e->backlerp * e->oldorigin[2];

	AnglesToAxis(e->angles, axis);

	float scale = (e->scale > 0.f) ? e->scale : 1.f;

	vec3_t scales = { scale, scale, scale };
	if (mirror)
	{
		scales[1] *= -1.f;
	}

	matrix[0]  = axis[0][0] * scales[0];
	matrix[4]  = axis[1][0] * scales[1];
	matrix[8]  = axis[2][0] * scales[2];
	matrix[12] = origin[0];

	matrix[1]  = axis[0][1] * scales[0];
	matrix[5]  = axis[1][1] * scales[1];
	matrix[9]  = axis[2][1] * scales[2];
	matrix[13] = origin[1];

	matrix[2]  = axis[0][2] * scales[0];
	matrix[6]  = axis[1][2] * scales[1];
	matrix[10] = axis[2][2] * scales[2];
	matrix[14] = origin[2];

	matrix[3]  = 0.0f;
	matrix[7]  = 0.0f;
	matrix[11] = 0.0f;
	matrix[15] = 1.0f;
}

void create_entity_matrix(int view_id, mat4_t matrix, entity_t *e)
{
	internal_create_entity_matrix(matrix, e, false);
}

void create_viewweapon_matrix(int stereo, int view_id, mat4_t matrix, entity_t *e)
{
	extern cvar_t   *info_hand;
	extern cvar_t   *cl_adjustfov;
	extern cvar_t   *cl_gunfov;

	internal_create_entity_matrix(matrix, e, info_hand->integer == 1);

	if (!stereo && (cl_gunfov->value > 0)) 
	{
		float gunfov_x, gunfov_y;
		gunfov_x = Cvar_ClampValue(cl_gunfov, 30, 160);

		if (cl_adjustfov->integer) 
		{
			gunfov_y = V_CalcFov(gunfov_x, 4, 3);
			gunfov_x = V_CalcFov(gunfov_y, vkpt_refdef.fd->height, vkpt_refdef.fd->width);
		} 
		else 
		{
			gunfov_y = V_CalcFov(gunfov_x, vkpt_refdef.fd->width, vkpt_refdef.fd->height);
		}

		/* Construct a matrix for the viewweapon entity so, after projection and all,
		 * it appears to be rendered with the gun FOV (which may differ from the view FOV). */
		mat4_t tmp;
		mult_matrix_matrix(tmp, vkpt_refdef.view_matrix[LEFT], matrix);

		mat4_t adjust;
		adjust[0] = tan(vkpt_refdef.fd->fov_x * M_PI / 360.0) / tan(gunfov_x * M_PI / 360.0);
		adjust[4] = 0;
		adjust[8] = 0;
		adjust[12] = 0;

		adjust[1] = 0;
		adjust[5] = tan(vkpt_refdef.fd->fov_y * M_PI / 360.0) / tan(gunfov_y * M_PI / 360.0);
		adjust[9] = 0;
		adjust[13] = 0;

		adjust[2] = 0;
		adjust[6] = 0;
		adjust[10] = 1;
		adjust[14] = 0;

		adjust[3] = 0;
		adjust[7] = 0;
		adjust[11] = 0;
		adjust[15] = 1;

		mat4_t tmp2;
		mult_matrix_matrix(tmp2, adjust, tmp);

		mult_matrix_matrix(matrix, vkpt_refdef.view_matrix_inv[view_id], tmp2);
	}
}

void create_projection_matrix(mat4_t matrix, float znear, float zfar, float fov_x, float fov_y)
{
	float xmin, xmax, ymin, ymax;
	float width, height, depth;

	ymax = znear * tan(fov_y * M_PI / 360.0);
	ymin = -ymax;

	xmax = znear * tan(fov_x * M_PI / 360.0);
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zfar - znear;

	matrix[0] = 2 * znear / width;
	matrix[4] = 0;
	matrix[8] = (xmax + xmin) / width;
	matrix[12] = 0;

	matrix[1] = 0;
	matrix[5] = -2 * znear / height;
	matrix[9] = (ymax + ymin) / height;
	matrix[13] = 0;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = (zfar + znear) / depth;
	matrix[14] = 2 * zfar * znear / depth;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 1;
	matrix[15] = 0;
}

void create_projection_matrixXR(float znear, float zfar, XrFovf* fov, mat4_t projection_matrix)
{
	float aspect_ratio = 1.0f;

	const float tan_left = tanf(fov->angleLeft) * aspect_ratio;
	const float tan_right = tanf(fov->angleRight) * aspect_ratio;

	const float tan_up = tanf(fov->angleUp);
	const float tan_down = tanf(fov->angleDown);

	const float tan_width = (tan_right - tan_left);
	const float tan_right_left_sum = (tan_right + tan_left);

	const float tan_height = (tan_down - tan_up);
	const float tan_up_down_sum = (tan_up + tan_down);

	if(zfar <= znear)
	{
		const float tan_width = (tan_right - tan_left);
		const float tan_right_left_sum = (tan_right + tan_left);

		const float tan_height = (tan_down - tan_up);
		const float tan_up_down_sum = (tan_up + tan_down);

		float ymax = znear * tan_up;
		float ymin = znear * tan_down;

		float xmax = znear * tan_right;
		float xmin = znear * tan_left;

		float width = xmax - xmin;
		float height = ymax - ymin;
		float depth = zfar - znear;

		projection_matrix[0] = 2.0f / tan_width;
		projection_matrix[4] = 0.0f;
		projection_matrix[8] = tan_right_left_sum / tan_width;
		projection_matrix[12] = 0.0f;

		projection_matrix[1] = 0.0f;
		projection_matrix[5] = 2.0f / tan_height;
		projection_matrix[9] = tan_up_down_sum / tan_height;
		projection_matrix[13] = 0.0f;

		projection_matrix[2] = 0.0f;
		projection_matrix[6] = 0.0f;
		projection_matrix[10] = 1.0f;
		projection_matrix[14] = -znear;

		projection_matrix[3] = 0.0f;
		projection_matrix[7] = 0.0f;
		projection_matrix[11] = 1.0f;
		projection_matrix[15] = 0.0f;
	}
	else
	{
		float ymax = znear * tan_up;
		float ymin = znear * tan_down;

		float xmax = znear * tan_right;
		float xmin = znear * tan_left;

		float width = xmax - xmin;
		float height = ymax - ymin;
		float depth = zfar - znear;

		projection_matrix[0] = 2.0f * znear / width;
		projection_matrix[4] = 0.0f;
		projection_matrix[8] = (xmax + xmin) / width;
		projection_matrix[12] = 0.0f;

		projection_matrix[1] = 0.0f;
		projection_matrix[5] = -2.0f * znear / height;
		projection_matrix[9] = (ymax + ymin) / height;
		projection_matrix[13] = 0.0f;

		projection_matrix[2] = 0.0f;
		projection_matrix[6] = 0.0f;
		projection_matrix[10] = (zfar + znear) / depth;
		projection_matrix[14] = 2.0f * zfar * znear / depth;

		projection_matrix[3] = 0.0f;
		projection_matrix[7] = 0.0f;
		projection_matrix[11] = 1.0f;
		projection_matrix[15] = 0.0f;
	}
}


void create_orthographic_matrix(mat4_t matrix, float xmin, float xmax, float ymin, float ymax, float znear, float zfar)
{
	float width, height, depth;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zfar - znear;

	matrix[0] = 2 / width;
	matrix[4] = 0;
	matrix[8] = 0;
	matrix[12] = -(xmax + xmin) / width;

	matrix[1] = 0;
	matrix[5] = 2 / height;
	matrix[9] = 0;
	matrix[13] = -(ymax + ymin) / height;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = 1 / depth;
	matrix[14] = -znear / depth;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

void create_view_matrix(bool zero_out_pitch, int stereo, int view_id, float ipd, vec4_t ipd_offset_WS, mat4_t view_matrix, refdef_t *fd)
{
	vec3_t viewaxis[3];
	AnglesToAxis(fd->viewangles, viewaxis);

	if(zero_out_pitch)
	{
		vec3_t viewangles = { 0 };
		viewangles[YAW] = fd->viewangles[YAW];
		viewangles[ROLL] = fd->viewangles[ROLL];

		AnglesToAxis(viewangles, viewaxis);
	}
	else
	{
		AnglesToAxis(fd->viewangles, viewaxis);
	}

	view_matrix[0]  = -viewaxis[1][0];
	view_matrix[4]  = -viewaxis[1][1];
	view_matrix[8]  = -viewaxis[1][2];
	view_matrix[12] = 0.0f;

	view_matrix[1]  = viewaxis[2][0];
	view_matrix[5]  = viewaxis[2][1];
	view_matrix[9]  = viewaxis[2][2];
	view_matrix[13] = 0.0f;

	view_matrix[2]  = viewaxis[0][0];
	view_matrix[6]  = viewaxis[0][1];
	view_matrix[10] = viewaxis[0][2];
	view_matrix[14] = 0.0f;

	view_matrix[3]  = 0;
	view_matrix[7]  = 0;
	view_matrix[11] = 0;
	view_matrix[15] = 1;

	float abs_ipd = fabs(ipd);

	if(stereo && (abs_ipd > 0.0f))
	{
		const float half_ipd = (abs_ipd * 0.5f);

		mat4_t view_matrix_inv = { 0 };
		inverse(view_matrix, view_matrix_inv);

		const float ipd_offset_mag = (view_id == LEFT) ? -half_ipd : half_ipd;
		const vec4_t ipd_offset_LS = { ipd_offset_mag, 0.0f, 0.0f, 0.0f };

		mult_matrix_vector(ipd_offset_WS, view_matrix_inv, ipd_offset_LS);

		float x_ipd_offset = ipd_offset_WS[0];
		float y_ipd_offset = ipd_offset_WS[1];
		float z_ipd_offset = ipd_offset_WS[2];

		float world_space_x = fd->vieworg[0];
		float world_space_y = fd->vieworg[1];
		float world_space_z = fd->vieworg[2];

		world_space_x += x_ipd_offset;
		world_space_y += y_ipd_offset;
		world_space_z += z_ipd_offset;

		vec3_t vieworg_with_ipd_offset = { 0 };

		vieworg_with_ipd_offset[0] = world_space_x;
		vieworg_with_ipd_offset[1] = world_space_y;
		vieworg_with_ipd_offset[2] = world_space_z;

		float x_val = DotProduct(viewaxis[1], vieworg_with_ipd_offset);
		float y_val = -DotProduct(viewaxis[2], vieworg_with_ipd_offset);
		float z_val = -DotProduct(viewaxis[0], vieworg_with_ipd_offset);

		view_matrix[12] = x_val;
		view_matrix[13] = y_val;
		view_matrix[14] = z_val;
	}
	else
	{
		view_matrix[12] = DotProduct(viewaxis[1], fd->vieworg);
		view_matrix[13] = -DotProduct(viewaxis[2], fd->vieworg);
		view_matrix[14] = -DotProduct(viewaxis[0], fd->vieworg);

		ipd_offset_WS[0] = 0.0f;
		ipd_offset_WS[1] = 0.0f;
		ipd_offset_WS[2] = 0.0f;
		ipd_offset_WS[3] = 0.0f;
	}
}

void inverse(const mat4_t m, mat4_t inv)
{
	inv[0] = m[5]  * m[10] * m[15] -
	         m[5]  * m[11] * m[14] -
	         m[9]  * m[6]  * m[15] +
	         m[9]  * m[7]  * m[14] +
	         m[13] * m[6]  * m[11] -
	         m[13] * m[7]  * m[10];
	
	inv[1] = -m[1]  * m[10] * m[15] +
	          m[1]  * m[11] * m[14] +
	          m[9]  * m[2] * m[15] -
	          m[9]  * m[3] * m[14] -
	          m[13] * m[2] * m[11] +
	          m[13] * m[3] * m[10];
	
	inv[2] = m[1]  * m[6] * m[15] -
	         m[1]  * m[7] * m[14] -
	         m[5]  * m[2] * m[15] +
	         m[5]  * m[3] * m[14] +
	         m[13] * m[2] * m[7] -
	         m[13] * m[3] * m[6];
	
	inv[3] = -m[1] * m[6] * m[11] +
	          m[1] * m[7] * m[10] +
	          m[5] * m[2] * m[11] -
	          m[5] * m[3] * m[10] -
	          m[9] * m[2] * m[7] +
	          m[9] * m[3] * m[6];
	
	inv[4] = -m[4]  * m[10] * m[15] +
	          m[4]  * m[11] * m[14] +
	          m[8]  * m[6]  * m[15] -
	          m[8]  * m[7]  * m[14] -
	          m[12] * m[6]  * m[11] +
	          m[12] * m[7]  * m[10];
	
	inv[5] = m[0]  * m[10] * m[15] -
	         m[0]  * m[11] * m[14] -
	         m[8]  * m[2] * m[15] +
	         m[8]  * m[3] * m[14] +
	         m[12] * m[2] * m[11] -
	         m[12] * m[3] * m[10];
	
	inv[6] = -m[0]  * m[6] * m[15] +
	          m[0]  * m[7] * m[14] +
	          m[4]  * m[2] * m[15] -
	          m[4]  * m[3] * m[14] -
	          m[12] * m[2] * m[7] +
	          m[12] * m[3] * m[6];
	
	inv[7] = m[0] * m[6] * m[11] -
	         m[0] * m[7] * m[10] -
	         m[4] * m[2] * m[11] +
	         m[4] * m[3] * m[10] +
	         m[8] * m[2] * m[7] -
	         m[8] * m[3] * m[6];
	
	inv[8] = m[4]  * m[9] * m[15] -
	         m[4]  * m[11] * m[13] -
	         m[8]  * m[5] * m[15] +
	         m[8]  * m[7] * m[13] +
	         m[12] * m[5] * m[11] -
	         m[12] * m[7] * m[9];
	
	inv[9] = -m[0]  * m[9] * m[15] +
	          m[0]  * m[11] * m[13] +
	          m[8]  * m[1] * m[15] -
	          m[8]  * m[3] * m[13] -
	          m[12] * m[1] * m[11] +
	          m[12] * m[3] * m[9];
	
	inv[10] = m[0]  * m[5] * m[15] -
	          m[0]  * m[7] * m[13] -
	          m[4]  * m[1] * m[15] +
	          m[4]  * m[3] * m[13] +
	          m[12] * m[1] * m[7] -
	          m[12] * m[3] * m[5];
	
	inv[11] = -m[0] * m[5] * m[11] +
	           m[0] * m[7] * m[9] +
	           m[4] * m[1] * m[11] -
	           m[4] * m[3] * m[9] -
	           m[8] * m[1] * m[7] +
	           m[8] * m[3] * m[5];
	
	inv[12] = -m[4]  * m[9] * m[14] +
	           m[4]  * m[10] * m[13] +
	           m[8]  * m[5] * m[14] -
	           m[8]  * m[6] * m[13] -
	           m[12] * m[5] * m[10] +
	           m[12] * m[6] * m[9];
	
	inv[13] = m[0]  * m[9] * m[14] -
	          m[0]  * m[10] * m[13] -
	          m[8]  * m[1] * m[14] +
	          m[8]  * m[2] * m[13] +
	          m[12] * m[1] * m[10] -
	          m[12] * m[2] * m[9];
	
	inv[14] = -m[0]  * m[5] * m[14] +
	           m[0]  * m[6] * m[13] +
	           m[4]  * m[1] * m[14] -
	           m[4]  * m[2] * m[13] -
	           m[12] * m[1] * m[6] +
	           m[12] * m[2] * m[5];
	
	inv[15] = m[0] * m[5] * m[10] -
	          m[0] * m[6] * m[9] -
	          m[4] * m[1] * m[10] +
	          m[4] * m[2] * m[9] +
	          m[8] * m[1] * m[6] -
	          m[8] * m[2] * m[5];

	float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	det = 1.0f / det;

	for(int i = 0; i < 16; i++)
		inv[i] = inv[i] * det;
}

void mult_matrix_matrix(mat4_t p, const mat4_t a, const mat4_t b)
{
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			p[i * 4 + j] =
				a[0 * 4 + j] * b[i * 4 + 0] +
				a[1 * 4 + j] * b[i * 4 + 1] +
				a[2 * 4 + j] * b[i * 4 + 2] +
				a[3 * 4 + j] * b[i * 4 + 3];
		}
	}
}

void mult_matrix_vector(vec4_t v, const mat4_t a, const vec4_t b)
{
	int j;
	for (j = 0; j < 4; j++) {
		v[j] =
			a[0 * 4 + j] * b[0] +
			a[1 * 4 + j] * b[1] +
			a[2 * 4 + j] * b[2] +
			a[3 * 4 + j] * b[3];
	}
}
