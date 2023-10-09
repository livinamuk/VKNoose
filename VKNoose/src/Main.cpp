#include "vk_engine.h"
#include <iostream>
#define NOMINMAX
#include "Windows.h"
#include "Game/AssetManager.h"
#include "Engine.h"

#include "OpenImageDenoise/oidn.hpp"

int width = 512 * 2;
int height = 288 * 2;
int num_threads = 1;	// number of threads to use (defualt is all)
int affinity = 0;		// int Enable affinity. This pins virtual threads to physical cores and can improve performance (default 0 i.e. disabled)
int hdr = 0;			// int: Image is a HDR image. Disabling with will assume the image is in sRGB (default 1 i.e. enabled)
int srgb = 0;			// int: Whether the main input image is encoded with the sRGB (or 2.2 gamma) curve (LDR only) or is linear (default 0 i.e. disabled)
int maxmem = 128;		// int: Maximum memory size used by the denoiser in MB
int clean_aux = 0;		// Whether the auxiliary feature (albedo, normal) images are noise-free; recommended for highest quality but should *not* be enabled for noisy auxiliary images to avoid residual noise (default 0 i.e. disabled)

void errorCallback(void* userPtr, oidn::Error error, const char* message) {
	std::cout << "Open Denoiser ERROR: " << message << "\n";
	//throw std::runtime_error(message);
}

bool progressCallback(void* userPtr, double n)
{
	std::cout << (int)(n * 100.0) << " complete\n";
	return true;
}

void Go() {

	// Create our device
	oidn::DeviceRef device = oidn::newDevice();
	const char* errorMessage;
	if (device.getError(errorMessage) != oidn::Error::None)
		throw std::runtime_error(errorMessage);
	device.setErrorFunction(errorCallback);
	// Set our device parameters
	if (num_threads)
		device.set("numThreads", num_threads);
	if (affinity)
		device.set("setAffinity", affinity);

	// Commit the changes to the device
	device.commit();
	const int versionMajor = device.get<int>("versionMajor");
	const int versionMinor = device.get<int>("versionMinor");
	const int versionPatch = device.get<int>("versionPatch");

	std::cout << "Using OIDN version " << versionMajor << "." << versionMinor << "." << versionPatch << "\n";

	// Create the AI filter
	oidn::FilterRef filter = device.newFilter("RT");

	// Set our progress callback
	//filter.setProgressMonitorFunction((oidn::ProgressMonitorFunction)progressCallback);


	ImageData imageData("input2.png");


	struct ColorUC {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;

		std::string toString() {
			std::stringstream stream;

			stream << "[" << (float)this->r / 255.0f << ", " << (float)this->g / 255.0f << ", " << (float)this->b / 255.0f << ", " << (float)this->a / 255.0f << "]";

			return stream.str();
		}
	};


	// print content of first pixel of the image
	std::cout << ((ColorUC*)imageData.data)->toString() << std::endl;

	for (int i = 0; i < imageData.getSize(); i += 4) {
	//	imageData.data[i + 1] = 0;
	//	imageData.data[i + 2] = 0;
	}

	//imageData.save("output.png");

	// Get our pixel data
	int size = imageData.width * imageData.height * imageData.nrChannels;



	int bufferSize = imageData.width * imageData.height * 4 * sizeof(float);


	std::vector<float> beauty_pixels(bufferSize);




	oidn::BufferRef colorBuf = device.newBuffer(bufferSize);
	oidn::BufferRef outputBuf = device.newBuffer(bufferSize);

	// Fill the input image buffers
	//float* colorPtr = (float*)oidnGetBufferData(colorBuf);

	// Fill the input image buffers
	float* colorPtr = (float*)colorBuf.getData();

	int index = 0;

	for (int i = 0; i < imageData.getSize(); i += imageData.nrChannels) {
		//std::cout << '[' << i << "]\n";
		beauty_pixels[i + 0] = (float)(imageData.data[i + 0] / 255.0f);
		beauty_pixels[i + 1] = (float)(imageData.data[i + 1] / 255.0f);
		beauty_pixels[i + 2] = (float)(imageData.data[i + 2] / 255.0f);
		beauty_pixels[i + 3] = (float)(imageData.data[i + 3] / 255.0f);

		colorPtr[index++] = (float)(imageData.data[i] / 255.0f);
		colorPtr[index++] = (float)(imageData.data[i + 1] / 255.0f);
		colorPtr[index++] = (float)(imageData.data[i + 2] / 255.0f);

		//std::cout << beauty_pixels[i] << ", " << beauty_pixels[i + 1] << ", " << beauty_pixels[i + 2] << "\n";
	}

	for (int i = 0; i < 12; i++) {
	//	colorPtr[i] = 255;
	}



	
	AssetManager::SaveImageData("test.png", imageData.width, imageData.height, imageData.nrChannels, imageData.data);


	AssetManager::SaveImageDataF("test2.png", imageData.width, imageData.height, imageData.nrChannels, colorBuf.getData());

	// Set our the filter images
	filter.setImage("color", colorBuf.getData(), oidn::Format::Float3, imageData.width, imageData.height);
	filter.setImage("output", outputBuf.getData(), oidn::Format::Float3, imageData.width, imageData.height);

	ImageData outputData;

	/*if (a_loaded)
		filter.setImage("albedo", (void*)&albedo_pixels[0], oidn::Format::Float3, a_width, a_height);
	if (n_loaded)
		filter.setImage("normal", (void*)&normal_pixels[0], oidn::Format::Float3, n_width, n_height);
	filter.setImage("output", (void*)&output_pixels[0], oidn::Format::Float3, b_width, b_height); */

	

	filter.set("hdr", hdr);
	filter.set("srgb", srgb);
	if (maxmem >= 0)
		filter.set("maxMemoryMB", maxmem);
	filter.set("cleanAux", clean_aux);

	// Commit changes to the filter
	filter.commit();


	void* ptr = colorBuf.getData();
	float* floatArray = (float*)colorBuf.getData();

	void* outPtr = outputBuf.getData();
	float* floatArray2 = (float*)outputBuf.getData();



	unsigned int num_runs = 1;


	// Execute denoise
	int sum = 0;
	for (unsigned int i = 0; i < num_runs; i++)
	{
		std::cout << "Denoising...\n";
		clock_t start = clock(), diff;
		filter.execute();
		diff = clock() - start;
		int msec = diff * 1000 / CLOCKS_PER_SEC;
		//if (num_runs > 1)
	//		std::cout << "Denoising run %d complete in %d.%03d seconds", i + 1, msec / 1000, msec % 1000);
		//else
		//	PrintInfo("Denoising complete in %d.%03d seconds", msec / 1000, msec % 1000);
		sum += msec;
	}
	if (num_runs > 1)
	{
		sum /= num_runs;
		//PrintInfo("Denoising avg of %d complete in %d.%03d seconds", num_runs, sum / 1000, sum % 1000);
	}


	for (int i = 0; i < 12; i++) {
		//std::cout << i << ": " << (float)(floatArray[i]) << " " << (float)(floatArray2[i]) << "\n";
	}

	AssetManager::SaveImageDataF("cunt.png", imageData.width, imageData.height, 3, floatArray2);
}

int main()
{
    //Go();
    //return 0;

	std::cout << "\nWELCOME TO HELL FUCK WHIT\n";
	
	Engine::Init();
	Engine::Run();
	Engine::Cleanup();

	return 0;
}