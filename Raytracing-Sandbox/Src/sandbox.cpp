// sandbox.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include <GLCore.h>
#include "ExampleLayer.h"
#include "Compute-Shader/00_Basic_Compute_Shader/basic_compute_shader.h"
#include "Compute-Shader/01_Blur_Using_Compute_Shader/blur_via_compute_shader.h"
#include "Compute-Shader/02_Evolving_Pics/test.h"
#include "In-One-Weekend/00_Image/image.h"

class MySandbox
	: public GLCore::Application
{
public:
	MySandbox ()
		: GLCore::Application("Sandbox")
	{
		PushLayer<ExampleLayer> ();
		PushLayer<BasicComputeShader_Test> ();
		PushLayer<BlurWithComputeShader_Test> ();
		PushLayer<just_a_test> ();
		PushLayer<In_One_Weekend::Image> ();
	}
};

int main()
{
	std::unique_ptr<MySandbox> app = std::make_unique<MySandbox>();
	app->Run();
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
