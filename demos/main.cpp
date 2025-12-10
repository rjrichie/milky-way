#include "Common.h"
#include "OpenGLCommon.h"
#include "OpenGLMarkerObjects.h"
#include "OpenGLBgEffect.h"
#include "OpenGLMesh.h"
#include "OpenGLViewer.h"
#include "OpenGLWindow.h"
#include "TinyObjLoader.h"
#include "OpenGLSkybox.h"
#include "OpenGLParticles.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>
#include <string>

#ifndef __Main_cpp__
#define __Main_cpp__

#ifdef __APPLE__
#define CLOCKS_PER_SEC 100000
#endif

#define PI 3.14159265359

struct PlanetInfo {
    std::string texture_name;
    float size;                 // Relative to sun
    float semi_major_axis;      // Distance from center
    float inclination;          // Tilt off of orbital plane (degrees)
    float a;                    // Longitude of Ascending Node - AKA Swivel (degrees)
    float start_anomaly;        // Starting position on orbit (degrees)
    float orbit_period_days;    // How long to orbit sun
    float axial_tilt;           // In degrees
    float axial_period_days;    // Length of a day
};

struct Planet {
    OpenGLTriangleMesh* mesh;
    PlanetInfo info;
};

class MyDriver : public OpenGLViewer
{
    std::vector<OpenGLTriangleMesh *> mesh_object_array;
    OpenGLBgEffect *bgEffect = nullptr;
    OpenGLSkybox *skybox = nullptr;
    OpenGLParticles<> *sunParticles = nullptr;
    clock_t startTime;

    std::vector<Planet> planets;
    float global_time_days = 0.0f;
    float days_per_frame = .05f / 24.0f; // speed of simulation in days per frame - 2 hours per frame
    OpenGLUbos::Light* sun_light = nullptr;

public:
    virtual void Initialize()
    {
        draw_axes = false;
        startTime = clock();
        OpenGLViewer::Initialize();
    }

    virtual void Initialize_Data()
    {
        //// Load all the shaders you need for the scene 
        //// In the function call of Add_Shader_From_File(), we specify three names: 
        //// (1) vertex shader file name
        //// (2) fragment shader file name
        //// (3) shader name used in the shader library
        //// When we bind a shader to an object, we implement it as follows:
        //// object->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("shader_name"));
        //// Here "shader_name" needs to be one of the shader names you created previously with Add_Shader_From_File()

        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/basic.vert", "shaders/basic.frag", "basic");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/basic.vert", "shaders/environment.frag", "environment");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/stars.vert", "shaders/stars.frag", "stars");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/basic.vert", "shaders/alphablend.frag", "blend");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/billboard.vert", "shaders/alphablend.frag", "billboard");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/terrain.vert", "shaders/terrain.frag", "terrain");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/skybox.vert", "shaders/skybox.frag", "skybox");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/psize_ucolor.vert", "shaders/psize_ucolor.frag", "psize_ucolor");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/skybox.vert", "shaders/stars_skybox.frag", "stars_skybox");

        //// Load all the textures you need for the scene
        //// In the function call of Add_Shader_From_File(), we specify two names:
        //// (1) the texture's file name
        //// (2) the texture used in the texture library
        //// When we bind a texture to an object, we implement it as follows:
        //// object->Add_Texture("tex_sampler", OpenGLTextureLibrary::Get_Texture("tex_name"));
        //// Here "tex_sampler" is the name of the texture sampler2D you used in your shader, and
        //// "tex_name" needs to be one of the texture names you created previously with Add_Texture_From_File()
        
        // Load planet textures
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/sun_color.jpg", "sun_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/mercury_color.jpg", "mercury_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/venus_color.jpg", "venus_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/earth_color.png", "earth_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/mars_color.jpg", "mars_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/jupiter_color.jpg", "jupiter_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/saturn_color.jpg", "saturn_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/uranus_color.jpg", "uranus_color");
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/neptune_color.jpg", "neptune_color");
        
        // Load normal maps
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/earth_normal.png", "earth_normal");
        // OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/bunny_normal.png", "bunny_normal");

        //// Add the background / environment
        //// Here we provide you with four default options to create the background of your scene:
        //// (1) Gradient color (like A1 and A2; if you want a simple background, use this one)
        //// (2) Programmable Canvas (like A7 and A8; if you consider implementing noise or particles for the background, use this one)
        //// (3) Sky box (cubemap; if you want to load six background images for a skybox, use this one)
        //// (4) Sky sphere (if you want to implement a sky sphere, enlarge the size of the sphere to make it colver the entire scene and update its shaders for texture colors)
        //// By default, Option (2) (Buzz stars) is turned on, and all the other three are commented out.

        //// Skybox Stars: procedural stars rendered on a rotating skybox
        {
            skybox = Add_Interactive_Object<OpenGLSkybox>();
            skybox->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("stars_skybox"));
            skybox->Initialize();
        }
        

        Add_Planet({ "sun_color", 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 25.67f });
        Add_Planet({ "mercury_color", 0.15f, 3.87f, 7.0f, 48.33f, 172.75f, 88.0f, 0.01f, 58.65f });
        Add_Planet({ "venus_color", 0.28f, 7.23f, 3.39f, 76.68f, 49.31f, 224.7f, 177.4f, -243.0f });
        Add_Planet({ "earth_color", 0.3f, 10.0f, 0.0f, 163.97f, 358.19f, 365.2f, 23.4f, 1.0f });
        Add_Planet({ "mars_color", 0.20f, 15.2f, 1.85f, 49.56f, 19.1f, 687.0f, 25.2f, 1.03f });
        Add_Planet({ "jupiter_color", 0.8f, 52.0f, 1.3f, 100.49f, 18.72f, 4331.0f, 3.1f, 0.41f });
        Add_Planet({ "saturn_color", 0.7f, 95.8f, 2.48f, 113.69f, 320.38f, 10747.0f, 26.7f, 0.45f });
        Add_Planet({ "uranus_color", 0.5f, 192.0f, 0.77f, 73.96f, 142.9f, 30589.0f, 97.8f, -0.72f });
        Add_Planet({ "neptune_color", 0.5f, 300.5f, 1.77f, 131.77f, 266.6f, 59800.0f, 28.3f, 0.67f });

        //// This for-loop updates the rendering model for each object on the list
        for (auto &mesh_obj : mesh_object_array){
            Set_Polygon_Mode(mesh_obj, PolygonMode::Fill);
            Set_Shading_Mode(mesh_obj, ShadingMode::TexAlpha);
            mesh_obj->Set_Data_Refreshed();
            mesh_obj->Initialize();
        }
        Toggle_Play();

        // Setup lighting: clear previous lights and add a sun point light + a fill directional light
        OpenGLUbos::Clear_Lights();
        OpenGLUbos::Set_Ambient(glm::vec4(0.04f, 0.04f, 0.05f, 1.0f));

        // add a point light at origin (sun)
        sun_light = OpenGLUbos::Add_Point_Light(glm::vec3(0.0f, 0.0f, 0.0f));
        if (sun_light) {
            sun_light->dif  = glm::vec4(1.0f, 0.9f, 0.7f, 1.0f); 
            sun_light->spec = glm::vec4(0.5f);
            sun_light->amb  = glm::vec4(0.05f, 0.04f, 0.03f, 1.0f);
            sun_light->atten = glm::vec4(1.0f, 0.02f, 0.0f, 0.0f);
        }

        // add a soft directional fill light
        {
            auto dir = OpenGLUbos::Add_Directional_Light(glm::vec3(-0.5f, -0.2f, -1.0f));
            if (dir) {
                dir->dif = glm::vec4(0.25f, 0.28f, 0.32f, 1.0f);
                dir->spec = glm::vec4(0.2f);
            }
        }

        // Initialize sun particle system
        Initialize_SunParticles();
    }

    //// add mesh object by reading an .obj file
    OpenGLTriangleMesh *Add_Obj_Mesh_Object(std::string obj_file_name)
    {
        auto mesh_obj = Add_Interactive_Object<OpenGLTriangleMesh>();
        Array<std::shared_ptr<TriangleMesh<3>>> meshes;
        // Obj::Read_From_Obj_File(obj_file_name, meshes);
        Obj::Read_From_Obj_File_Discrete_Triangles(obj_file_name, meshes);

        mesh_obj->mesh = *meshes[0];
        std::cout << "load tri_mesh from obj file, #vtx: " << mesh_obj->mesh.Vertices().size() << ", #ele: " << mesh_obj->mesh.Elements().size() << std::endl;

        mesh_object_array.push_back(mesh_obj);
        return mesh_obj;
    }

    //// add mesh object by reading an array of vertices and an array of elements
    OpenGLTriangleMesh* Add_Tri_Mesh_Object(const std::vector<Vector3>& vertices, const std::vector<Vector3i>& elements)
    {
        auto obj = Add_Interactive_Object<OpenGLTriangleMesh>();
        mesh_object_array.push_back(obj);
        // set up vertices and elements
        obj->mesh.Vertices() = vertices;
        obj->mesh.Elements() = elements;

        return obj;
    }

    void Add_Planet(PlanetInfo info) 
    {
        auto obj = Add_Obj_Mesh_Object("obj/sphere.obj");
        
        // Material - customize per planet type
        // Special case for sun: emissive appearance (high ambient, no specular)
        if (info.texture_name == "sun_color") {
            obj->Set_Ka(Vector3f(0.9, 0.9, 0.8));  // High ambient = self-illuminated
            obj->Set_Kd(Vector3f(1.0, 1.0, 0.95)); // Full diffuse
            obj->Set_Ks(Vector3f(0.0, 0.0, 0.0));  // No specular highlights
            obj->Set_Shininess(1.0);
        } else {
            // Regular planets: realistic material values
            obj->Set_Ka(Vector3f(0.05, 0.05, 0.05)); // Low ambient (lit by sun)
            obj->Set_Kd(Vector3f(0.8, 0.8, 0.8));    // Good diffuse reflectance
            obj->Set_Ks(Vector3f(0.3, 0.3, 0.3));    // Moderate specular
            obj->Set_Shininess(32.0);                 // Nice smooth highlights
        }

        // Bind Texture and Shader
        obj->Add_Texture("tex_color", OpenGLTextureLibrary::Get_Texture(info.texture_name));
        
        // Try to load matching normal map if it exists (e.g., earth_color -> earth_normal)
        std::string normal_name = info.texture_name;
        size_t color_pos = normal_name.find("_color");
        if (color_pos != std::string::npos) {
            normal_name.replace(color_pos, 6, "_normal");
            auto normal_tex = OpenGLTextureLibrary::Get_Texture(normal_name);
            if (normal_tex) {
                obj->Add_Texture("tex_normal", normal_tex);
            }
        }
        
        obj->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("basic"));

        // Store for physics updates
        planets.push_back({obj, info});
    }

    //// Initialize sun particle system
    void Initialize_SunParticles()
    {
        //// Create sun particles
        sunParticles = Add_Interactive_Object<OpenGLParticles<>>();
        int N = 800;
        sunParticles->particles.Resize(N);

        float sunRadius = 1.0f;

        // fill particle arrays
        for (int i = 0; i < N; ++i) {
            // Random direction on sphere
            float z = 2.0f * ((rand() / (float)RAND_MAX) - 0.5f);
            float phi = 2.0f * M_PI * (rand() / (float)RAND_MAX);
            float rxy = sqrt(max(0.0f, 1.0f - z*z));
            Vector3 posDir((real)(rxy * cos(phi)), (real)(rxy * sin(phi)), (real)z);
            // start slightly above surface
            float dist = sunRadius * (0.95f + 0.05f * (rand() / (float)RAND_MAX));
            sunParticles->particles.XRef()[i] = posDir * (real)dist; // position around the sun

            // Velocity outward from surface plus tangent swirl
            float speed = 0.5f + 1.5f * (rand() / (float)RAND_MAX); // tune
            Vector3 vel = posDir * (real)speed;
            Vector3 tangent = Vector3(-posDir.y(), posDir.x(), (real)0.0).normalized();
            vel += (real)0.2 * ((rand() / (float)RAND_MAX) - 0.5f) * tangent;
            sunParticles->particles.VRef()[i] = vel;

            sunParticles->particles.CRef()[i] = (real)1.0;
            sunParticles->particles.RRef()[i] = (real)(0.02f + 0.03f * (rand() / (float)RAND_MAX));
        }

        // mark particle data refreshed so OpenGLParticles will upload it to the GPU
        sunParticles->Set_Data_Refreshed();

        // Use a moderate point size for visibility (drivers may clamp very large sizes)
        sunParticles->Set_Point_Size(64.f);

        // Use a simple point-sprite shader (psize_ucolor) for GPU point sprites as an option
        sunParticles->shader_programs.clear();
        sunParticles->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("psize_ucolor"));

        // Try to load a small flare texture; if it exists we'll use textured sprites,
        // otherwise fall back to plain colored points so particles are visible for debugging.
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/sun_flare.png","sun_flare");
        auto flare_tex = OpenGLTextureLibrary::Get_Texture("sun_flare");
        if (flare_tex) {
            sunParticles->Set_Shading_Mode(ShadingMode::TexAlpha);
            sunParticles->Add_Texture("tex_color", flare_tex);
            sunParticles->Set_Color(OpenGLColor(1.0f, 0.7f, 0.2f, 1.0f));
            sunParticles->Enable_Alpha_Blend();
        } else {
            // fallback: untextured colored points (guaranteed to render with existing pipeline)
            sunParticles->Set_Shading_Mode(ShadingMode::None);
            sunParticles->Set_Color(OpenGLColor(1.0f, 0.65f, 0.1f, 1.0f));
        }

        // initialize
        sunParticles->Initialize();
    }

    float ToRadian(float deg) { 
        return deg * (float)M_PI / 180.0f; 
    }

    Matrix4f GetScaleMatrix(float s) {
        Matrix4f m; 
        m << s, 0, 0, 0,  
             0, s, 0, 0,  
             0, 0, s, 0,  
             0, 0, 0, 1; 
        return m;
    }
    Matrix4f GetTranslationMatrix(float x) {
        Matrix4f m; 
        m << 1, 0, 0, x,  
             0, 1, 0, 0,  
             0, 0, 1, 0,  
             0, 0, 0, 1; 
        return m;
    }
    Matrix4f GetRotationX(float deg) {
        float r = ToRadian(deg); 
        float c = cos(r); 
        float s = sin(r);
        Matrix4f m; 
        m << 1, 0, 0, 0,  
             0, c, -s, 0,  
             0, s, c, 0,  
             0, 0, 0, 1; 
        return m;
    }
    Matrix4f GetRotationY(float deg) {
        float r = ToRadian(deg); 
        float c = cos(r); 
        float s = sin(r);
        Matrix4f m; 
        m << c, 0, s, 0,  
             0, 1, 0, 0,  
             -s, 0, c, 0,  
             0, 0, 0, 1; return m;
    }
    Matrix4f GetRotationZ(float deg) {
        float r = ToRadian(deg); 
        float c = cos(r); 
        float s = sin(r);
        Matrix4f m; 
        m << c, -s, 0, 0,  
             s, c, 0, 0,  
             0, 0, 1, 0,  
             0, 0, 0, 1; 
        return m;
    }

    //// Go to next frame
    virtual void Toggle_Next_Frame()
    {
        global_time_days += days_per_frame;

        for (auto &p : planets) {
            
            // Calculate angles based on time
            float orbit_angle = (global_time_days / p.info.orbit_period_days) * 360.0f;
            float spin_angle = (global_time_days / p.info.axial_period_days) * 360.0f;
            float current_orbit_pos = p.info.start_anomaly + orbit_angle;

            // Create Transformations
            Matrix4f m_scale = GetScaleMatrix(p.info.size / 2.5);
            Matrix4f m_spin = GetRotationY(spin_angle); // Roate around own axis
            Matrix4f m_tilt = GetRotationZ(p.info.axial_tilt); // Axial tilt
            Matrix4f m_trans = GetTranslationMatrix(p.info.semi_major_axis / 4); // Orbit radius
            Matrix4f m_orbit = GetRotationY(current_orbit_pos); // Position along the orbit ring
            Matrix4f m_inc   = GetRotationZ(p.info.inclination); // Inclination of the orbit plane
            Matrix4f m_lan   = GetRotationY(p.info.a); // Longitude of Ascending Node (rotate plane around sun)

            // Combine Matrices 
            Matrix4f final_transform = m_lan * m_inc * m_orbit * m_trans * m_tilt * m_spin * m_scale;

            p.mesh->Set_Model_Matrix(final_transform);
        }

        // update sun point-light to follow the first planet (sun)
        if (sun_light && planets.size() > 0) {
            auto& sun_mesh = planets[0].mesh->model_matrix;
            glm::vec3 sun_pos = glm::vec3(sun_mesh[3][0], sun_mesh[3][1], sun_mesh[3][2]);
            sun_light->pos = glm::vec4(sun_pos, 1.0f);
            OpenGLUbos::Update_Lights_Ubo();
        }

        for (auto &mesh_obj : mesh_object_array)
            mesh_obj->setTime(GLfloat(clock() - startTime) / CLOCKS_PER_SEC);

        if (bgEffect){
            bgEffect->setResolution((float)Win_Width(), (float)Win_Height());
            bgEffect->setTime(GLfloat(clock() - startTime) / CLOCKS_PER_SEC);
            bgEffect->setFrame(frame++);
        }

        if (skybox){
            skybox->setTime(GLfloat(clock() - startTime) / CLOCKS_PER_SEC);
        }   

        if (sunParticles) {
            float dt = 1.0f / 60.0f; 
            int N = sunParticles->particles.Size();
            for (int i = 0; i < N; ++i) {
                sunParticles->particles.XRef()[i] += sunParticles->particles.VRef()[i] * dt;
                float dist = sunParticles->particles.XRef()[i].norm();
            }
            sunParticles->Set_Data_Refreshed();
        }

        OpenGLViewer::Toggle_Next_Frame();
    }

    virtual void Run()
    {
        OpenGLViewer::Run();
    }
};

int main(int argc, char *argv[])
{
    MyDriver driver;
    driver.Initialize();
    driver.Run();
}

#endif