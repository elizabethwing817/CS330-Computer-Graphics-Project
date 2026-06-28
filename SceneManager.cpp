///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL basicMaterial;

	basicMaterial.ambientColor = glm::vec3(0.5f, 0.5f, 0.5f);
	basicMaterial.ambientStrength = 0.35f;

	basicMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);

	basicMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);
	basicMaterial.shininess = 4.0f;

	basicMaterial.tag = "basic";

	m_objectMaterials.push_back(basicMaterial);
}

void SceneManager::SetupSceneLights()
{
	// Turn on lighting in the shader.
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main warm yellow light above the scene.
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 6.0f, 4.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.10f, 0.08f, 0.03f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(1.0f, 0.75f, 0.25f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(1.0f, 0.85f, 0.35f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 24.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.35f);

	// Secondary blue accent light near the floor lamp.
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(7.0f, 3.0f, -3.4f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.02f, 0.03f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.05f, 0.25f, 1.0f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.15f, 0.45f, 1.0f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 20.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.5f);
}
/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	DefineObjectMaterials();
	SetupSceneLights();

	CreateGLTexture("textures/wood.jpg", "wood");
	BindGLTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Floor
	scaleXYZ = glm::vec3(20.0f, 1.0f, 12.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.18f, 0.16f, 0.14f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawPlaneMesh();

	// Record player base
	scaleXYZ = glm::vec3(5.0f, 0.4f, 3.5f);
	positionXYZ = glm::vec3(0.0f, 0.4f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(2.0f, 2.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Vinyl record
	scaleXYZ = glm::vec3(1.8f, 0.08f, 1.8f);
	positionXYZ = glm::vec3(0.0f, 0.72f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawCylinderMesh();

	// Record label
	scaleXYZ = glm::vec3(0.45f, 0.09f, 0.45f);
	positionXYZ = glm::vec3(0.0f, 0.78f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.1f, 0.1f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawCylinderMesh();

	// Tonearm
	scaleXYZ = glm::vec3(0.08f, 0.08f, 2.2f);
	positionXYZ = glm::vec3(1.5f, 0.9f, 0.4f);
	SetTransformations(scaleXYZ, 0.0f, 35.0f, 0.0f, positionXYZ);
	SetShaderColor(0.65f, 0.48f, 0.30f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Left speaker body
	scaleXYZ = glm::vec3(1.0f, 3.0f, 0.8f);
	positionXYZ = glm::vec3(-4.0f, 1.1f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Left speaker circle
	scaleXYZ = glm::vec3(0.45f, 0.08f, 0.45f);
	positionXYZ = glm::vec3(-4.0f, 1.25f, 0.45f);
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.18f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawCylinderMesh();

	// Right speaker body
	scaleXYZ = glm::vec3(1.0f, 2.2f, 0.8f);
	positionXYZ = glm::vec3(4.0f, 1.1f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.08f, 0.08f, 0.08f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Right speaker circle
	scaleXYZ = glm::vec3(0.45f, 0.08f, 0.45f);
	positionXYZ = glm::vec3(4.0f, 1.25f, 0.45f);
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.18f, 0.18f, 0.18f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawCylinderMesh();
	// Vinyl shelf outer frame
	scaleXYZ = glm::vec3(6.0f, 0.25f, 1.0f);
	positionXYZ = glm::vec3(0.0f, 1.6f, -3.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.20f, 0.11f, 0.05f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Shelf bottom
	scaleXYZ = glm::vec3(6.0f, 0.25f, 1.0f);
	positionXYZ = glm::vec3(0.0f, 0.25f, -3.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.20f, 0.11f, 0.05f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Shelf left side
	scaleXYZ = glm::vec3(0.25f, 1.5f, 1.0f);
	positionXYZ = glm::vec3(-3.0f, 0.9f, -3.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.20f, 0.11f, 0.05f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Shelf right side
	scaleXYZ = glm::vec3(0.25f, 1.5f, 1.0f);
	positionXYZ = glm::vec3(3.0f, 0.9f, -3.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.20f, 0.11f, 0.05f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawBoxMesh();

	// Shelf vertical dividers
	for (float x = -1.5f; x <= 1.5f; x += 1.5f)
	{
		scaleXYZ = glm::vec3(0.18f, 1.35f, 1.0f);
		positionXYZ = glm::vec3(x, 0.9f, -3.2f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(0.23f, 0.13f, 0.06f, 1.0f);
		SetShaderMaterial("basic");
		m_basicMeshes->DrawBoxMesh();
	}

	// Simple colored vinyl records inside shelf
	for (float x = -2.3f; x <= 2.3f; x += 0.35f)
	{
		scaleXYZ = glm::vec3(0.08f, 0.9f, 0.75f);
		positionXYZ = glm::vec3(x, 0.75f, -2.65f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 8.0f, positionXYZ);
		SetShaderColor(0.12f, 0.12f, 0.12f, 1.0f);
		SetShaderMaterial("basic");
		m_basicMeshes->DrawBoxMesh();
	}
	// Lamp cone base - gray
	scaleXYZ = glm::vec3(0.55f, 0.95f, 0.55f);
	positionXYZ = glm::vec3(7.0f, 0.48f, -3.4f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.65f, 0.65f, 0.65f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawConeMesh();

	// Lamp blue cone shade
	scaleXYZ = glm::vec3(0.95f, 0.40f, 0.95f);
	positionXYZ = glm::vec3(7.0f, 1.30f, -3.4f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.10f, 0.25f, 0.95f, 1.0f);
	SetShaderMaterial("basic");
	m_basicMeshes->DrawConeMesh();

}

