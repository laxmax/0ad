/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include <math.h>

#include "MiniMap.h"

#include "graphics/GameView.h"
#include "graphics/LOSTexture.h"
#include "graphics/MiniPatch.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerritoryTexture.h"
#include "lib/ogl.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpMinimap.h"
#include "simulation2/system/ParamNode.h"

bool g_GameRestarted = false;

static unsigned int ScaleColor(unsigned int color, float x)
{
	unsigned int r = unsigned(float(color & 0xff) * x);
	unsigned int g = unsigned(float((color>>8) & 0xff) * x);
	unsigned int b = unsigned(float((color>>16) & 0xff) * x);
	return (0xff000000 | r | g<<8 | b<<16);
}

CMiniMap::CMiniMap() :
	m_TerrainTexture(0), m_TerrainData(0), m_MapSize(0), m_Terrain(0), m_TerrainDirty(true), m_MapScale(1.f)
{
	AddSetting(GUIST_CColor,	"fov_wedge_color");
	AddSetting(GUIST_CStrW,		"tooltip");
	AddSetting(GUIST_CStr,		"tooltip_style");
	m_Clicking = false;
	m_MouseHovering = false;
	
	// Get the maximum height for unit passage in water.
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml");
	const CParamNode pathingSettings = externalParamNode.GetChild("Pathfinder").GetChild("PassabilityClasses");
	if (pathingSettings.GetChild("default").IsOk() && pathingSettings.GetChild("default").GetChild("MaxWaterDepth").IsOk())
		m_ShallowPassageHeight = pathingSettings.GetChild("default").GetChild("MaxWaterDepth").ToFloat();
	else
		m_ShallowPassageHeight = 0.0f;
}

CMiniMap::~CMiniMap()
{
	Destroy();
}

void CMiniMap::HandleMessage(SGUIMessage &Message)
{
	switch(Message.type)
	{
	case GUIM_MOUSE_PRESS_LEFT:
		{
			if (m_MouseHovering)
			{
				SetCameraPos();
				m_Clicking = true;
			}
			break;
		}
	case GUIM_MOUSE_RELEASE_LEFT:
		{
			if(m_MouseHovering && m_Clicking)
			{
				SetCameraPos();
			}
			m_Clicking = false;
			break;
		}
	case GUIM_MOUSE_DBLCLICK_LEFT:
		{
			if(m_MouseHovering && m_Clicking)
			{
				SetCameraPos();
			}
			m_Clicking = false;
			break;
		}
	case GUIM_MOUSE_ENTER:
		{
			m_MouseHovering = true;
			break;
		}
	case GUIM_MOUSE_LEAVE:
		{
			m_Clicking = false;
			m_MouseHovering = false;
			break;
		}
	case GUIM_MOUSE_RELEASE_RIGHT:
		{
			CMiniMap::FireWorldClickEvent(SDL_BUTTON_RIGHT, 1);
			break;
		}
	case GUIM_MOUSE_DBLCLICK_RIGHT:
		{
			CMiniMap::FireWorldClickEvent(SDL_BUTTON_RIGHT, 2);
			break;
		}
	case GUIM_MOUSE_MOTION:
		{
			if (m_MouseHovering && m_Clicking)
			{
				SetCameraPos();
			}
			break;
		}
	case GUIM_MOUSE_WHEEL_DOWN:
	case GUIM_MOUSE_WHEEL_UP:
		Message.Skip();
		break;

	default:
		break;
	}	// switch
}

void CMiniMap::GetMouseWorldCoordinates(float& x, float& z)
{
	// Determine X and Z according to proportion of mouse position and minimap

	CPos mousePos = GetMousePos();

	float px = (mousePos.x - m_CachedActualSize.left) / m_CachedActualSize.GetWidth();
	float py = (m_CachedActualSize.bottom - mousePos.y) / m_CachedActualSize.GetHeight();

	float angle = GetAngle();

	// Scale world coordinates for shrunken square map
	x = TERRAIN_TILE_SIZE * m_MapSize * (m_MapScale * (cos(angle)*(px-0.5) - sin(angle)*(py-0.5)) + 0.5);
	z = TERRAIN_TILE_SIZE * m_MapSize * (m_MapScale * (cos(angle)*(py-0.5) + sin(angle)*(px-0.5)) + 0.5);
}

void CMiniMap::SetCameraPos()
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	CVector3D target;
	GetMouseWorldCoordinates(target.X, target.Z);
	target.Y = terrain->GetExactGroundLevel(target.X, target.Z);
	g_Game->GetView()->MoveCameraTarget(target);
}

float CMiniMap::GetAngle()
{
	CVector3D cameraIn = m_Camera->m_Orientation.GetIn();
	return -atan2(cameraIn.X, cameraIn.Z);
}

void CMiniMap::FireWorldClickEvent(int button, int clicks)
{
	float x, z;
	GetMouseWorldCoordinates(x, z);

	CScriptValRooted coords;
	g_ScriptingHost.GetScriptInterface().Eval("({})", coords);
	g_ScriptingHost.GetScriptInterface().SetProperty(coords.get(), "x", x, false);
	g_ScriptingHost.GetScriptInterface().SetProperty(coords.get(), "z", z, false);
	ScriptEvent("worldclick", coords);

	UNUSED2(button);
	UNUSED2(clicks);
}

#if CONFIG2_GLES
#warning TODO: implement minimap for GLES
void CMiniMap::Draw()
{
}
#else

// render view rect : John M. Mena
// This sets up and draws the rectangle on the mini-map
// which represents the view of the camera in the world.
void CMiniMap::DrawViewRect()
{
	// Compute the camera frustum intersected with a fixed-height plane.
	// TODO: Currently we hard-code the height, so this'll be dodgy when maps aren't the
	// expected height - how can we make it better without the view rect wobbling in
	// size while the player scrolls?
	float h = 16384.f * HEIGHT_SCALE;

	CVector3D hitPt[4];
	hitPt[0]=m_Camera->GetWorldCoordinates(0, g_Renderer.GetHeight(), h);
	hitPt[1]=m_Camera->GetWorldCoordinates(g_Renderer.GetWidth(), g_Renderer.GetHeight(), h);
	hitPt[2]=m_Camera->GetWorldCoordinates(g_Renderer.GetWidth(), 0, h);
	hitPt[3]=m_Camera->GetWorldCoordinates(0, 0, h);

	float ViewRect[4][2];
	for (int i=0;i<4;i++) {
		// convert to minimap space
		float px=hitPt[i].X;
		float pz=hitPt[i].Z;
		ViewRect[i][0]=(m_CachedActualSize.GetWidth()*px/float(TERRAIN_TILE_SIZE*m_MapSize));
		ViewRect[i][1]=(m_CachedActualSize.GetHeight()*pz/float(TERRAIN_TILE_SIZE*m_MapSize));
	}

	// Enable Scissoring as to restrict the rectangle
	// to only the mini-map below by retrieving the mini-maps
	// screen coords.
	const float x = m_CachedActualSize.left, y = m_CachedActualSize.bottom;
	glScissor((int)x, g_Renderer.GetHeight()-(int)y, (int)m_CachedActualSize.GetWidth(), (int)m_CachedActualSize.GetHeight());
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(2.0f);
	glColor3f(1.0f, 0.3f, 0.3f);

	// Draw the viewing rectangle with the ScEd's conversion algorithm
	glBegin(GL_LINE_LOOP);
	glVertex2f(ViewRect[0][0], -ViewRect[0][1]);
	glVertex2f(ViewRect[1][0], -ViewRect[1][1]);
	glVertex2f(ViewRect[2][0], -ViewRect[2][1]);
	glVertex2f(ViewRect[3][0], -ViewRect[3][1]);
	glEnd();

	// restore state
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
}

struct MinimapUnitVertex
{
	u8 r, g, b, a;
	float x, y;
};

void CMiniMap::DrawTexture(float coordMax, float angle, float x, float y, float x2, float y2, float z)
{
	// Rotate the texture coordinates (0,0)-(coordMax,coordMax) around their center point (m,m)
	// Scale square maps to fit in circular minimap area
	const float s = sin(angle) * m_MapScale;
	const float c = cos(angle) * m_MapScale;
	const float m = coordMax / 2.f;

	glBegin(GL_QUADS);
	glTexCoord2f(m*(-c + s + 1.f), m*(-c + -s + 1.f));
	glVertex3f(x, y, z);
	glTexCoord2f(m*(c + s + 1.f), m*(-c + s + 1.f));
	glVertex3f(x2, y, z);
	glTexCoord2f(m*(c + -s + 1.f), m*(c + s + 1.f));
	glVertex3f(x2, y2, z);
	glTexCoord2f(m*(-c + -s + 1.f), m*(c + -s + 1.f));
	glVertex3f(x, y2, z);
	glEnd();
}

void CMiniMap::Draw()
{
	PROFILE3("render minimap");

	// The terrain isn't actually initialized until the map is loaded, which
	// happens when the game is started, so abort until then.
	if(!(GetGUI() && g_Game && g_Game->IsGameStarted()))
		return;

	CSimulation2* sim = g_Game->GetSimulation2();
	CmpPtr<ICmpRangeManager> cmpRangeManager(*sim, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);

	// Set our globals in case they hadn't been set before
	m_Camera      = g_Game->GetView()->GetCamera();
	m_Terrain     = g_Game->GetWorld()->GetTerrain();
	m_Width  = (u32)(m_CachedActualSize.right - m_CachedActualSize.left);
	m_Height = (u32)(m_CachedActualSize.bottom - m_CachedActualSize.top);
	m_MapSize = m_Terrain->GetVerticesPerSide();
	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize);
	m_MapScale = (cmpRangeManager->GetLosCircular() ? 1.f : 1.414f);

	if(!m_TerrainTexture || g_GameRestarted)
		CreateTextures();


	// only update 2x / second
	// (note: since units only move a few pixels per second on the minimap,
	// we can get away with infrequent updates; this is slow)
	static double last_time;
	const double cur_time = timer_Time();
	if(cur_time - last_time > 0.5)
	{
		last_time = cur_time;

		if(m_TerrainDirty)
			RebuildTerrainTexture();
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	CMatrix3D matrix = GetDefaultGuiMatrix();
	glLoadMatrixf(&matrix._11);

	// Disable depth updates to prevent apparent z-fighting-related issues
	// with some drivers causing units to get drawn behind the texture
	glDepthMask(0);
	
	CShaderProgramPtr shader;
	CShaderTechniquePtr tech;
	
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		CShaderDefines defines;
		defines.Add("MINIMAP_BASE", "1");
		tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("minimap"), g_Renderer.GetSystemShaderDefines(), defines);
		tech->BeginPass();
		shader = tech->GetShader();
	}

	const float x = m_CachedActualSize.left, y = m_CachedActualSize.bottom;
	const float x2 = m_CachedActualSize.right, y2 = m_CachedActualSize.top;
	const float z = GetBufferedZ();
	const float texCoordMax = (float)(m_MapSize - 1) / (float)m_TextureSize;
	const float angle = GetAngle();

	// Draw the main textured quad
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		shader->BindTexture("baseTex", m_TerrainTexture);
	else
		g_Renderer.BindTexture(0, m_TerrainTexture);
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	DrawTexture(texCoordMax, angle, x, y, x2, y2, z);


	// Draw territory boundaries
	CTerritoryTexture& territoryTexture = g_Game->GetView()->GetTerritoryTexture();
	
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		shader->BindTexture("baseTex", territoryTexture.GetTexture());
	else
		territoryTexture.BindTexture(0);
	
	glEnable(GL_BLEND);
	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(territoryTexture.GetMinimapTextureMatrix());
	glMatrixMode(GL_MODELVIEW);

	DrawTexture(1.0f, angle, x, y, x2, y2, z);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_BLEND);


	// Draw the LOS quad in black, using alpha values from the LOS texture
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();
	
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		tech->EndPass();

		CShaderDefines defines;
		defines.Add("MINIMAP_LOS", "1");
		tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("minimap"), g_Renderer.GetSystemShaderDefines(), defines);
		tech->BeginPass();
		shader = tech->GetShader();
		shader->BindTexture("baseTex", losTexture.GetTexture());
	}
	else
	{
		losTexture.BindTexture(0);
	}
	
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor3f(0.0f, 0.0f, 0.0f);

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(losTexture.GetMinimapTextureMatrix());
	glMatrixMode(GL_MODELVIEW);

	DrawTexture(1.0f, angle, x, y, x2, y2, z);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_BLEND);
	
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		tech->EndPass();

		CShaderDefines defines;
		defines.Add("MINIMAP_POINT", "1");
		tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("minimap"), g_Renderer.GetSystemShaderDefines(), defines);
		tech->BeginPass();
		shader = tech->GetShader();
	}
	
	// Set up the matrix for drawing points and lines
	glPushMatrix();
	glTranslatef(x, y, z);
	// Rotate around the center of the map
	glTranslatef((x2-x)/2.f, (y2-y)/2.f, 0.f);
	// Scale square maps to fit in circular minimap area
	float unitScale = (cmpRangeManager->GetLosCircular() ? 1.f : m_MapScale/2.f);
	glScalef(unitScale, unitScale, 1.f);
	glRotatef(angle * 180.f/M_PI, 0.f, 0.f, 1.f);
	glTranslatef(-(x2-x)/2.f, -(y2-y)/2.f, 0.f);

	PROFILE_START("minimap units");

	// Don't enable GL_POINT_SMOOTH because it's far too slow
	// (~70msec/frame on a GF4 rendering a thousand points)
	glPointSize(3.f);

	float sx = (float)m_Width / ((m_MapSize - 1) * TERRAIN_TILE_SIZE);
	float sy = (float)m_Height / ((m_MapSize - 1) * TERRAIN_TILE_SIZE);

	CSimulation2::InterfaceList ents = sim->GetEntitiesWithInterface(IID_Minimap);

	std::vector<MinimapUnitVertex> vertexArray;
	vertexArray.reserve(ents.size());

	for (CSimulation2::InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
	{
		MinimapUnitVertex v;
		ICmpMinimap* cmpMinimap = static_cast<ICmpMinimap*>(it->second);
		entity_pos_t posX, posZ;
		if (cmpMinimap->GetRenderData(v.r, v.g, v.b, posX, posZ))
		{
			ICmpRangeManager::ELosVisibility vis = cmpRangeManager->GetLosVisibility(it->first, g_Game->GetPlayerID());
			if (vis != ICmpRangeManager::VIS_HIDDEN)
			{
				v.a = 255;
				v.x = posX.ToFloat()*sx;
				v.y = -posZ.ToFloat()*sy;

				// Check minimap pinging to indicate something
				if (cmpMinimap->IsPinging())
				{
					// Override the normal rendering of the entity with the ping color
					// Note: If the pinged entity's dot is rendered over by another entity's
					// dot then it will be invisible & the ping will be not be seen.
					// We can try to move the pinged dots towards the end in the vertexArray
					// Keep 2 pointers and insert pinged dots at end, unpinged at current position
					if (cmpMinimap->CheckPing())
					{
						v.r = 255; // ping color is white
						v.g = 255;
						v.b = 255;
					}
				}

				vertexArray.push_back(v);
			}
		}
	}

	if (!vertexArray.empty())
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		{
			shader->VertexPointer(2, GL_FLOAT, sizeof(MinimapUnitVertex), &vertexArray[0].x);
			shader->ColorPointer(4, GL_UNSIGNED_BYTE, sizeof(MinimapUnitVertex), &vertexArray[0].r);
		}
		else
		{
			glVertexPointer(2, GL_FLOAT, sizeof(MinimapUnitVertex), &vertexArray[0].x);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(MinimapUnitVertex), &vertexArray[0].r);
		}

		glDrawArrays(GL_POINTS, 0, (GLsizei)vertexArray.size());

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	PROFILE_END("minimap units");

	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		tech->EndPass();

		CShaderDefines defines;
		defines.Add("MINIMAP_LINE", "1");
		tech = g_Renderer.GetShaderManager().LoadEffect(CStrIntern("minimap"), g_Renderer.GetSystemShaderDefines(), defines);
		tech->BeginPass();
		shader = tech->GetShader();
	}

	DrawViewRect();

	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
	{
		tech->EndPass();
	}

	// Reset everything back to normal
	glPointSize(1.0f);
	glEnable(GL_TEXTURE_2D);
	glDepthMask(1);
}

#endif // CONFIG2_GLES

void CMiniMap::CreateTextures()
{
	Destroy();

	// Create terrain texture
	glGenTextures(1, &m_TerrainTexture);
	g_Renderer.BindTexture(0, m_TerrainTexture);

	// Initialise texture with solid black, for the areas we don't
	// overwrite with glTexSubImage2D later
	u32* texData = new u32[m_TextureSize * m_TextureSize];
	for (ssize_t i = 0; i < m_TextureSize * m_TextureSize; ++i)
		texData[i] = 0xFF000000;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_TextureSize, m_TextureSize, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texData);
	delete[] texData;

	m_TerrainData = new u32[(m_MapSize - 1) * (m_MapSize - 1)];
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Rebuild and upload both of them
	RebuildTerrainTexture();
}


void CMiniMap::RebuildTerrainTexture()
{
	u32 x = 0;
	u32 y = 0;
	u32 w = m_MapSize - 1;
	u32 h = m_MapSize - 1;
	float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight;

	m_TerrainDirty = false;

	for(u32 j = 0; j < h; j++)
	{
		u32 *dataPtr = m_TerrainData + ((y + j) * (m_MapSize - 1)) + x;
		for(u32 i = 0; i < w; i++)
		{
			float avgHeight = ( m_Terrain->GetVertexGroundLevel((int)i, (int)j)
					+ m_Terrain->GetVertexGroundLevel((int)i+1, (int)j)
					+ m_Terrain->GetVertexGroundLevel((int)i, (int)j+1)
					+ m_Terrain->GetVertexGroundLevel((int)i+1, (int)j+1)
				) / 4.0f;

			if(avgHeight < waterHeight && avgHeight > waterHeight - m_ShallowPassageHeight)
			{
				// shallow water
				*dataPtr++ = 0xff7098c0;
			} else if (avgHeight < waterHeight)
			{
				// Set water as constant color for consistency on different maps
				*dataPtr++ = 0xff5078a0;
			}
			else
			{
				int hmap = ((int)m_Terrain->GetHeightMap()[(y + j) * m_MapSize + x + i]) >> 8;
				int val = (hmap / 3) + 170;

				u32 color = 0xFFFFFFFF;

				CMiniPatch *mp = m_Terrain->GetTile(x + i, y + j);
				if(mp)
				{
					CTerrainTextureEntry *tex = mp->GetTextureEntry();
					if(tex)
					{
						// If the texture can't be loaded yet, set the dirty flags
						// so we'll try regenerating the terrain texture again soon
						if(!tex->GetTexture()->TryLoad())
							m_TerrainDirty = true;

						color = tex->GetBaseColor();
					}
				}

				*dataPtr++ = ScaleColor(color, float(val) / 255.0f);
			}
		}
	}

	// Upload the texture
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize - 1, m_MapSize - 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_TerrainData);
}

void CMiniMap::Destroy()
{
	if(m_TerrainTexture)
	{
		glDeleteTextures(1, &m_TerrainTexture);
		m_TerrainTexture = 0;
	}

	delete[] m_TerrainData;
	m_TerrainData = 0;
}
