
// ADDING as a node has to wait until later
namespace Noise
{
	//Fbm2 is Fractal Brownim Motion type noise
	float Fbm2 (float x, float y, float freq, float lac, float gain, int octaves);

	//Turbulance is turbulent fractal type noise
	float Turbulance (float x, float y, float freq, float lac, float gain, int octaves);

	// 2D simplex noise
	float Snoise2 (float x, float y);
};