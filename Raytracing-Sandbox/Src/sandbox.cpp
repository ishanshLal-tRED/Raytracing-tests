// sandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include <GLCore.h>
#include "ExampleLayer.h"
#include "Compute-Shader/00_Basic_Compute_Shader/basic_compute_shader.h"
#include "Compute-Shader/01_Blur_Using_Compute_Shader/blur_via_compute_shader.h"
#include "Compute-Shader/02_Evolving_Pics/test.h"
#include "Compute-Shader/03_Buffer_supply_to_shader_OR_SSBO/test.h"
#include "In-One-Weekend/00_Image/image.h"
#include "In-One-Weekend/01_Adding_Sphere/Sphere.h"
#include "In-One-Weekend/02_Groups/groups.h"
#include "In-One-Weekend/03_Shadows_and_Materials/materials.h"
#include "In-Next-Week/00_MotionBlur/motion_blur.h"
#include "In-Next-Week/01_BoundingVolumeHierarchy/BVH.h"

//static size_t memory_in_use = 0;
//static size_t peak_memory_use = 0;
//// Damn there is hefty amount of memory leak
//void *operator new(size_t size)
//{
//	memory_in_use += size;
//	peak_memory_use += (peak_memory_use < memory_in_use ? size : 0);
//	return malloc (size);
//}
//void operator delete(void* ptr, size_t size)
//{
//	memory_in_use -= size;
//	free (ptr);
//}

class MySandbox
	: public GLCore::Application
{
public:
	MySandbox ()
		: GLCore::Application("Sandbox")
	{
		// PushLayer<ExampleLayer> ();
		// PushLayer<BasicComputeShader_Test> ();
		// PushLayer<BlurWithComputeShader_Test> ();
		// PushLayer<just_a_test> ();
		// PushLayer<In_One_Weekend::Image> ();
		// PushLayer<In_One_Weekend::Sphere> ();
		// PushLayer<In_One_Weekend::Groups> ();
		// PushLayer<In_One_Weekend::Adding_Materials> ();
		PushLayer<In_Next_Week::MotionBlur> ();
		// PushLayer<In_Next_Week::BoundingVolumeHierarchy> ();
		PushLayer<In_Next_Week::BVH> ();
		PushLayer<BuffSupply::Test> ();
	}
};

int main()
{
	MySandbox *app = new MySandbox();
	app->Run();
	delete app;
	//std::cout << "memory leak: " << memory_in_use << ", peak memory use: "<<peak_memory_use;
}
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
