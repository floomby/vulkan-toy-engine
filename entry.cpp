#include "engine.hpp"

#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;

void run(EngineSettings& settings);

int main(int argc, char **argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print help message")
        ("debug", "Enable extra debug options")
        ("use-one-queue", "Disable use of the transfer queue")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    EngineSettings settings = {};
    if (vm.count("use-one-queue"))
        settings.useConcurrentTransferQueue = false;
    else
        settings.useConcurrentTransferQueue = true;
    settings.height = 900;
    settings.width = 1600;
    settings.validationLayers = { "VK_LAYER_KHRONOS_validation" };
    // settings.maxFramesInFlight = 3;
    settings.applicationName = "Vulkan Test";
    settings.extremelyVerbose = false;
    settings.verbose = true;
    if (vm.count("debug")) {
        settings.validationExtentions = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
        settings.extremelyVerbose = true;
    } else {
        settings.validationExtentions = { };
    }
    settings.maxTextures = 1024;
    settings.maxHudTextures = 128;

    try {
        run(settings);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// TODO I do horrible things by inspecting stack unwinding in destructors so I need raii pattern here before the catch
void run(EngineSettings& settings) {
    Engine engine(settings);
    engine.init();
    engine.runCurrentScene();
}

/*
 I am linking to libpng now for freetype2, maybe I should stop using stbi
 Make concurrent frames correspond to the command buffers, descriptor sets and so forth while swapchain.size() is used for the framebuffer and other stuff like that
   So I mostly did this, but now I have the problem of the input attachments being in the framebuffer and that size being depending on the swapchain,
   I do not know the correct way around this
 In the gui code for the ndc coords are in x = <> and y = ^v (nvm, I am just going insane cause I thought I was mixing my x and y in the input code)
 cpu (host) allocation stuff for vulkan (vma)?? (This is probably not worth doing until I know it is bad as it is)
 I think I read the glfw docs wrong and dont need spsc lockfree queues for the input handling threads (afaict the linux tids are the same as the engine thread, this
 could be not guarenteed though)
 The background rendinging pass is almost entirely uneeded at this point, I moved the icon rendering from this pass into the world pass because without it the code
 complexity of order line drawing was going to be really high
 one wierd thing is how the lighting works, the blin phong lighting model I was using was using a point light, but I am using orthographic lighting for the shadows
  I should probably change it to not be like that

Next steps
 I am getting worried about the amount of linear algebra (and just stuff in general, but almost everything we do takes linear algebra) we are doing on the cpu in the main render thread
 shader multisampling
 mess with the shadows, I think some messing with the filtering parameters as well as the depth biasing and slope factor could improve it dramatically
 world space raycasting and maybe spherecasting (this is the half of projectiles that still needs to be done, most of the work is already there in the form
   of glm functionallity, but it still needs some work)
 projectiles (this is half done)
 organized input handling (a new object and associated files to do this)
 gui - I still need some more components, but this seems really low priority right now
 lt - networking
 my text rendering with freetype cant render space characters for some reason
 the text rendering to texture code is so slow, (Like unbelievably slow, of course I did note that it looked slow when I was writting it in the first place)
 finish the path following/steering code

 not crittical - collider class so I can support aabb and sphere and importantly obb coliders
 I would like bloom, idk if this means I need hdr color attachements or what
 lt - better model library (assimp?)
 cube mapping for lights and calculating shadows with them would be cool
 dynamic backface culling (Idk if this is a thing, the hardware supports it, but my prefunctory perusing through the vulkan docs did not show this was availible)
   It might be one of those things that on some hardware would require rebuilding the pipeline and trigger shader recompilation and therefore might not be in the standard
   or fix normals for skybox geometry
 nlips is cool, we can make BIG units
 explosions - particles?

Bugfixxy stuff
 Mipmap max lod is not working, although I haven't looked at it in a while (I made a so question for this)
 InternalTexture constructor fails if channels are 3 (and presumably 2 and possibly 1 as well) Vulkan doesnt like the format when creating the image via vmaCreateImage
 resizing rarly, but sometimes does this:
    Validation layer: Validation Error: [ VUID-VkSwapchainCreateInfoKHR-imageExtent-01274 ] Object 0: handle = 0x560cc52946a8, type = VK_OBJECT_TYPE_DEVICE;
    | MessageID = 0x7cd0911d | vkCreateSwapchainKHR() called with imageExtent = (1326,610), which is outside the bounds returned by
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): currentExtent = (1293,581), minImageExtent = (1293,581), maxImageExtent = (1293,581). The Vulkan spec
    states: imageExtent must be between minImageExtent and maxImageExtent, inclusive, where minImageExtent and maxImageExtent are members of the
    VkSurfaceCapabilitiesKHR structure returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface
    (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-VkSwapchainCreateInfoKHR-imageExtent-01274)
 also it can mess up the gui more frequently (I try and suspend drawing to the gui if I have resized, but not rebuilt the gui)
 I actually think there are three seperate bugs... *sad walrus noises* (one with the framebuffer image extent, one where the gui buffer is used while it is still copying or 
  something leading to the warning about the vertex shader not having the buffer completely filled, and one where the buffer gets corrupted leading to
  inncorrect textures being used)

 get the api inspector figured out (I am crashing on vkEnumeratePhysicalDevices)
*/