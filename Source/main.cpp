#include "../Externals/Include/Include.h"
#include "../Externals/Include/assimp/Importer.hpp"
#include "../Externals/Include/assimp/scene.h"
#include "../Externals/Include/assimp/postprocess.h"
#include "../Externals/Include/Camera.h"
#include "../Externals/Include/Program.h"

#define _CRT_SECURE_NO_WARNINGS
#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3
#define SKYBOX_SIZE 5000
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define SHADOW_WIDTH 1024
#define SHADOW_HEIGHT 1024
#define SHADOW_MAP_SIZE 1024

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

// pos(0, 0, 0) front(-1, -1, 0)
Camera camera(vec3(0.0f, 0.0f, 30.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0, 0.0, -1.0), WINDOW_WIDTH, WINDOW_HEIGHT, 80.0);

// define some stucte
struct Shape
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint ibo;
	int drawCount;
	int materialID;
};

struct Material
{
	GLuint diffuse_tex;
};

struct MVP_mat
{
	mat4 proj;
	mat4 view;
	mat4 model;
	string model_name;
	string view_name;
	string proj_name;
};

// ladybug
Program ladybug_prog;
vector<Shape> ladybug_s;
vector<Material> ladybug_m;
float bug_final_r = 0.0f;
float bug_final_h = 0.0f;
// bug_1
float bug_1_now_r = 0.0f;
float bug_1_now_h = 0.0f;
// bug_2
float bug_2_now_r = 0.0f;
float bug_2_now_h = 0.0f;

// cubemap
Program cubemap_program;
GLuint cubemap_vao;
GLuint cubemap_vbo;
mat4 cubemap_proj;
mat4 cubemap_view;
mat4 cubemap_model;
GLuint cubemap_tex;

vec3 light_pos = vec3(-31.75, 26.05, -97.72);
vec3 obj_pos = vec3(-10, -13, -8);

static const GLfloat cubemap_positions[] =
{
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f
};

// 512 * 512
vector<std::string> faces_final_1 =
{
	"final_1/morning_rt.png",
	"final_1/morning_lf.png",
	"final_1/morning_up.png",
	"final_1/morning_dn.png",
	"final_1/morning_bk.png",
	"final_1/morning_ft.png"
};

vector<std::string> faces_final_2 =
{
	"final_2/spires_rt.png",
	"final_2/spires_lf.png",
	"final_2/spires_up.png",
	"final_2/spires_dn.png",
	"final_2/spires_bk.png",
	"final_2/spires_ft.png"
};

static const GLfloat window_positions[] =
{
	1.0f,-1.0f,1.0f,0.0f,
	-1.0f,-1.0f,0.0f,0.0f,
	-1.0f,1.0f,0.0f,1.0f,
	1.0f,1.0f,1.0f,1.0f
};

// proto-type
void My_Reshape(int width, int height);


// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
    _TextureData(void) :
        width(0),
        height(0),
        data(0)
    {
    }

    int width;
	int height;
    unsigned char* data;
} TextureData;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath, int channels)
{
    TextureData texture;
    int components;

    // load the texture with stb image, force RGBA (4 components required)
    stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, channels);

    // is the image successfully loaded?
    if (data != NULL)
    {
        // copy the raw data
        size_t dataSize = texture.width * texture.height * channels * sizeof(unsigned char);
        texture.data = new unsigned char[dataSize];
        memcpy(texture.data, data, dataSize);

        // mirror the image vertically to comply with OpenGL convention
        for (size_t i = 0; i < texture.width; ++i)
        {
            for (size_t j = 0; j < texture.height / 2; ++j)
            {
                for (size_t k = 0; k < channels; ++k)
                {
                    size_t coord1 = (j * texture.width + i) * channels + k;
                    size_t coord2 = ((texture.height - j - 1) * texture.width + i) * channels + k;
                    std::swap(texture.data[coord1], texture.data[coord2]);
                }
            }
        }

        // release the loaded image
        stbi_image_free(data);
    }

    return texture;
}

GLuint My_LoadShader(const char *vsName, const char *fsName)
{
	// Create Shader Program
	GLuint program = glCreateProgram();

	// Create customize shader by tell openGL specify shader type 
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

	// Load shader file
	char** vsSource = loadShaderSource(vsName);
	char** fsSource = loadShaderSource(fsName);

	// Assign content of these shader files to those shaders we created before
	glShaderSource(vs, 1, vsSource, NULL);
	glShaderSource(fs, 1, fsSource, NULL);

	// Free the shader file string(won't be used any more)
	freeShaderSource(vsSource);
	freeShaderSource(fsSource);

	// Compile these shaders
	glCompileShader(vs);
	glCompileShader(fs);

	// Assign the program we created before with these shaders
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	// Tell OpenGL to use this shader program now
	// glUseProgram(program);

	return program;
}

void My_LoadModels(const char *objName, vector<Shape> &obj_shapes, vector<Material> &obj_materials)
{
	const aiScene *scene = aiImportFile(objName, aiProcessPreset_TargetRealtime_MaxQuality);
	if (scene == NULL) {
		std::cout << "error scene load\n";
		return ;
	}
	else
		std::cout << "load " << objName << " sucess\n"
				  << "number of meshs = " << scene->mNumMeshes << "\n"
				  << "number of materials = " << scene->mNumMaterials << "\n";

	//Material material;
	aiString texturePath;
	//total = scene->mNumMeshes;
	for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial *material = scene->mMaterials[i];
		Material materials;
		aiString texturePath;
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
		{
			TextureData tdata = loadPNG(texturePath.C_Str(), 4);
			if (tdata.data != NULL)
				cout << "load material[ " << i << " ] : " << texturePath.C_Str() << " sucess\n";
			else
			{
				cout << "load material[ " << i << " ] fail\n";
				continue;
			}
			glGenTextures(1, &materials.diffuse_tex);
			glBindTexture(GL_TEXTURE_2D, materials.diffuse_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			cout << "material[ " << i << " ].texturePath is not found\n";
		}
		obj_materials.push_back(materials);

	}
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh *mesh = scene->mMeshes[i];
		Shape shape;
		glGenVertexArrays(1, &shape.vao);
		glBindVertexArray(shape.vao);

		vector<float>vertices;
		vector<float>texCoords;
		vector<float>normals;

		glGenBuffers(1, &shape.vbo_position);
		glGenBuffers(1, &shape.vbo_texcoord);
		glGenBuffers(1, &shape.vbo_normal);
		for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		{
			vertices.push_back(mesh->mVertices[v].x);
			vertices.push_back(mesh->mVertices[v].y);
			vertices.push_back(mesh->mVertices[v].z);
			texCoords.push_back(mesh->mTextureCoords[0][v].x);
			texCoords.push_back(mesh->mTextureCoords[0][v].y);

			normals.push_back(mesh->mNormals[v].x);
			normals.push_back(mesh->mNormals[v].y);
			normals.push_back(mesh->mNormals[v].z);
		}

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), &texCoords[0], GL_STATIC_DRAW);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(2);

		// ibo
		vector<unsigned int>indices;
		glGenBuffers(1, &shape.ibo);
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
		{

			indices.push_back(mesh->mFaces[f].mIndices[0]);
			indices.push_back(mesh->mFaces[f].mIndices[1]);
			indices.push_back(mesh->mFaces[f].mIndices[2]);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


		// glVertexAttribPointer / glEnableVertexArray calls¡K
		shape.materialID = mesh->mMaterialIndex;
		shape.drawCount = mesh->mNumFaces * 3;
		// save shape¡K

		obj_shapes.push_back(shape);
		cout << "shape[ " << i << " ].materialID = " << shape.materialID << "\n";
	}

	aiReleaseImport(scene);
}

GLuint load_cubemap(vector<std::string> faces)
{
	GLuint texture_ID;
	glGenTextures(1, &texture_ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture_ID);

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		TextureData texture;
		texture = loadPNG(faces[i].c_str(), 3);
		
		if (texture.data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
					   	 texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture.data);
			std::cout << "load " << faces[i] << " in Positive_x + " << i << "\n";
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	
	return texture_ID;
}

void My_Init()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// ----- begin ini ladybug -----
	ladybug_prog.set_program("ladybug_vertex.vs.glsl", "ladybug_fragment.fs.glsl");
	My_LoadModels("ladybug_v2/ladybug.obj", ladybug_s, ladybug_m);
	// ----- end ini ladybug ------

	// ----- begin ini cubemap -----
	/*
	cubemap_program.set_program("cubemap_vertex.vs.glsl", "cubemap_fragment.fs.glsl");

	// cubemap vao and vbo
	glGenVertexArrays(1, &cubemap_vao);
	glBindVertexArray(cubemap_vao);

	glGenBuffers(1, &cubemap_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cubemap_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubemap_positions), cubemap_positions, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(0);

	cubemap_tex = load_cubemap(faces_final_1);

	// seamless
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	*/
	// ----- end ini cubemap -----
	// glutWarpPointer(camera.aspect_w / 2, camera.aspect_h / 2);
}

// if need to send other uniform variables, send it before use draw_obj().
void draw_obj(Program program, GLuint vao, MVP_mat mvp, GLenum target, GLuint texture, bool mode, int count)
{
	program.use();
	
	glBindVertexArray(vao);

	program.set_mat4(mvp.model_name, mvp.model);
	program.set_mat4(mvp.view_name, mvp.view);
	program.set_mat4(mvp.proj_name, mvp.proj);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(target, texture);

	if (mode == 0)
	{
		glDrawArrays(GL_TRIANGLES, 0, count);
	}
	else if (mode == 1)
	{
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);
	}
	glBindVertexArray(0);
}

void render_cubemap()
{
	// glUseProgram(cubemap_program);
	cubemap_program.use();

	glBindVertexArray(cubemap_vao);

	// model
	cubemap_model = scale(mat4(), vec3(SKYBOX_SIZE, SKYBOX_SIZE, SKYBOX_SIZE));
	cubemap_view = camera.get_view_matrix();
	cubemap_proj = camera.get_proj_matrix();

	cubemap_program.set_mat4("cubemap_model", cubemap_model);
	cubemap_program.set_mat4("cubemap_view", cubemap_view);
	cubemap_program.set_mat4("cubemap_proj", cubemap_proj);

	glActiveTexture(GL_TEXTURE0);
	cubemap_program.set_int("cubemap", 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);

	glDrawArrays(GL_TRIANGLES, 0, 36);
}

//float temp = 0.0f;

void My_Display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	camera.get_delta_time();

	// ----- begin draw cubemap -----
	// render_cubemap();
	// ----- end draw cubemap -----

	// ========== begin draw bugs ==========
	// ----- begin draw ladybug_1 -----
	ladybug_prog.use();

	GLfloat move = glutGet(GLUT_ELAPSED_TIME) / 30.0f;
	float bug_start_r = 15.0f;

	float diff_1 = 0.32f;
	if (bug_final_r > bug_1_now_r)
		bug_1_now_r += diff_1;
	if (bug_final_r < bug_1_now_r)
		bug_1_now_r -= diff_1;
	if (bug_final_h > bug_1_now_h)
		bug_1_now_h += diff_1;
	if (bug_final_h < bug_1_now_h)
		bug_1_now_h -= diff_1;
	mat4 ladybug_model = rotate(mat4(), radians(-move), vec3(0.0, 1.0, 0.0)) * translate(mat4(), vec3(bug_start_r + bug_1_now_r, 0.0 + bug_1_now_h, 0.0)) * scale(mat4(), vec3(1.0, 1.0, 1.0));
	mat4 ladybug_view = camera.get_view_matrix();
	mat4 ladybug_proj = camera.get_proj_matrix();

	ladybug_prog.set_mat4("um4mv", ladybug_view * ladybug_model);
	ladybug_prog.set_mat4("um4p", ladybug_proj);

	for (int i = 0; i < ladybug_s.size(); i++)
	{
		glBindVertexArray(ladybug_s[i].vao);

		int materialID = ladybug_s[i].materialID;
		glBindTexture(GL_TEXTURE_2D, ladybug_m[materialID].diffuse_tex);
		glDrawElements(GL_TRIANGLES, ladybug_s[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	// ----- end draw ladybug_1 -----

	// ----- begin draw ladybug_2 -----
	// prog and vp are same as bug_1
	float diff_2 = 0.16f;
	if (bug_final_r > bug_2_now_r)
		bug_2_now_r += diff_2;
	if (bug_final_r < bug_2_now_r)
		bug_2_now_r -= diff_2;
	if (bug_final_h > bug_2_now_h)
		bug_2_now_h += diff_2;
	if (bug_final_h < bug_2_now_h)
		bug_2_now_h -= diff_2;

	ladybug_model = rotate(mat4(), radians(-move + 30.0f), vec3(0.0, 1.0, 0.0)) * translate(mat4(), vec3(bug_start_r + bug_2_now_r, 0.0 + bug_2_now_h, 0.0)) * scale(mat4(), vec3(1.0, 1.0, 1.0));
	
	ladybug_prog.set_mat4("um4mv", ladybug_view * ladybug_model);

	for (int i = 0; i < ladybug_s.size(); i++)
	{
		glBindVertexArray(ladybug_s[i].vao);

		int materialID = ladybug_s[i].materialID;
		glBindTexture(GL_TEXTURE_2D, ladybug_m[materialID].diffuse_tex);
		glDrawElements(GL_TRIANGLES, ladybug_s[i].drawCount, GL_UNSIGNED_INT, 0);
	}
	// ----- end draw ladybug_2 -----
	// ========== end draw bugs ==========

	// ----- begin draw ladybug -----
	// bee_v1
	/*
	// body
	ladybug_prog.use();

	GLfloat move = glutGet(GLUT_ELAPSED_TIME) / 50.0;
	mat4 ladybug_model =  translate(mat4(), vec3(0.0, 0.0, 0.0)) * rotate(mat4(), radians(move), vec3(0.0, 1.0, 0.0)) * scale(mat4(), vec3(1.0, 1.0, 1.0));
	mat4 ladybug_view = camera.get_view_matrix();
	mat4 ladybug_proj = camera.get_proj_matrix();

	ladybug_prog.set_mat4("um4mv", ladybug_view * ladybug_model);
	ladybug_prog.set_mat4("um4p", ladybug_proj);

	glBindVertexArray(ladybug_s[0].vao);
	glBindTexture(GL_TEXTURE_2D, ladybug_m[0].diffuse_tex);
	glDrawElements(GL_TRIANGLES, ladybug_s[0].drawCount, GL_UNSIGNED_INT, 0);
	
	// wing
	ladybug_prog.use();

	//GLfloat move = glutGet(GLUT_ELAPSED_TIME) / 50.0;
	ladybug_model = translate(mat4(), vec3(0.0, 1.2, -2.0)) * rotate(mat4(), radians(move), vec3(0.0, 1.0, 0.0)) * rotate(mat4(), radians(-30.0f), vec3(1.0, 0.0, 0.0)) * rotate(mat4(), radians(90.0f), vec3(0.0, 0.0, 1.0)) * scale(mat4(), vec3(0.2, 0.2, 0.2));
	//mat4 ladybug_view = camera.get_view_matrix();
	//mat4 ladybug_proj = camera.get_proj_matrix();

	ladybug_prog.set_mat4("um4mv", ladybug_view * ladybug_model);
	ladybug_prog.set_mat4("um4p", ladybug_proj);

	glBindVertexArray(ladybug_s[1].vao);
	glBindTexture(GL_TEXTURE_2D, ladybug_m[1].diffuse_tex);
	glDrawElements(GL_TRIANGLES, ladybug_s[1].drawCount, GL_UNSIGNED_INT, 0);
	*/
	// ----- end draw ladybug -----

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
}

void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(timer_speed, My_Timer, val);
}

void My_Mouse(int button, int state, int x, int y)
{
	if(state == GLUT_DOWN)
	{
		printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if(state == GLUT_UP)
	{
		printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void My_Keyboard(unsigned char key, int x, int y)
{
	camera.camera_movement(key);

	// control the circle radius
	float r_diff = 3.2f;
	float r_min = 0.0f;
	float r_max = 32.0f;
	if (key == 'o')
		bug_final_r += r_diff;
	if (key == 'p')
		bug_final_r -= r_diff;
	if (bug_final_r > r_max)
		bug_final_r = r_max;
	if (bug_final_r < r_min)
		bug_final_r = r_min;

	// control the bug's flying height
	float h_diff = 5.0f;
	float h_min = -15.0f;
	float h_max = 15.0f;
	if (key == 'k')
		bug_final_h += h_diff;
	if (key == 'l')
		bug_final_h -= h_diff;
	if (bug_final_h > h_max)
		bug_final_h = h_max;
	if (bug_final_h < h_min)
		bug_final_h = h_min;

	printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	
	//cout << "camera position: " << camera.Position[0] << " " << camera.Position[1] << " " << camera.Position[2] << "\n"
	//	<< "camera front: " << camera.Front[0] << " " << camera.Front[1] << " " << camera.Front[2] << "\n"
	//	<< "delta time: " << camera.delta_time << "\n"
	//	<< "last frame: " << camera.last_frame << "\n";
	//cout << "temp :" << temp << endl;
}

void My_SpecialKeys(int key, int x, int y)
{
	switch(key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch(id)
	{
	case MENU_TIMER_START:
		if(!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	default:
		break;
	}
}

void detect_mouse(int x, int y)
{
	// camera.camera_rotation(x, y);
	// cout << "in detect_mouse:" << x << " " << y << "\n";
}

void detect_mouse_scroll(int wheel, int dir, int x, int y)
{
	camera.camera_zoom(dir);
	// cout << "in detect_scroll:" << dir << " " << x << " " << y << "\n";
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
    // Change working directory to source code path
    chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Final project"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
    glPrintContextInfo();
	My_Init();

	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0); 

	// camera
	glutPassiveMotionFunc(detect_mouse);
	glutMouseWheelFunc(detect_mouse_scroll);
	// glutSetCursor(GLUT_CURSOR_NONE);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}
