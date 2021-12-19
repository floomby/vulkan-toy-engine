## Next steps
 * I should adopt a style guide or something, the code base is big enough to warrent one
 * Finish the sound
 * Configurable keyboard input
 * Resource system
 * Shader multisampling (This is going to create a problem with alpha discard for the icons)
 * Unit highlighting shader improvement
 * gui - I still need some more components, but this seems really low priority right now
 * Fix/finish the path following/steering code
 * Maybe units should try and avoid friendly firing
 * Finish unit ais
 * I guess I will go back to drawing in z sorted order for getting cloaking working
 * Decide where the cammera settings go (runtime in setting, in lua, or compile time?) becuase the health bar drawing depends near far for normed z offset (This needs to be fixed)
 * The switching in the dynamic shadow level of detail is sort of appalling (this could be as easy as reducing the pcf kernel size, but I doubt it)

## Considerations
 * I think many things will be broken on big endian systems
 * There are problems with building stuff. I had to patch the castxml rubygem I was using to make it work.
 * I am linking to libpng now for freetype2, maybe I should stop using stbi
 * The string of glyphs to cache is pretty silly imo
 * cpu (host) allocation stuff for vulkan (vma)?? (This is not worth doing until I profile stuff)
 * I think I read the glfw docs wrong and dont need to be using interthread queues for the input handling threads (afaict the linux tids are the same as the engine thread, this could be not guarenteed though)
 * The background rendinging pass is almost entirely uneeded at this point, I moved the icon rendering from this pass back into the world pass because without it the code complexity of order line drawing was going to be really high doing it this way
 * I need to take a look and lighting and materials and stuff
 * I should move to using cmake or something for building
 * I need to look around at vram memory usage in general, it is very inefficient in many places

## Longer term
 * Collider abstraction so I can support aabb and importantly obb coliders
 * I would like bloom, idk if this means I need hdr color attachements or what
 * Better model library?
 * I want to implement dynamic lighting at some point, I will probablly just stay with forward rendering since I have transparency
 * Nlips is cool
 * Explosions - particles?

### Things I am ignoring
 * Mipmap max lod is not working, although I haven't looked at it in a while and I just recently redid some of the related code
 * EntityTexture constructor fails if channels are 3 (and presumably 2 and possibly 1 as well) Vulkan doesnt like the format when creating the image via vmaCreateImage (I don't exactly remember what the problem was here. I think it is a matter of querying device support and adapting what we are doing.)
 * Resizing rarly, but sometimes does this:
    `Validation layer: Validation Error: [ VUID-VkSwapchainCreateInfoKHR-imageExtent-01274 ] Object 0: handle = 0x560cc52946a8, type = VK_OBJECT_TYPE_DEVICE;
    | MessageID = 0x7cd0911d | vkCreateSwapchainKHR() called with imageExtent = (1326,610), which is outside the bounds returned by
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): currentExtent = (1293,581), minImageExtent = (1293,581), maxImageExtent = (1293,581). The Vulkan spec
    states: imageExtent must be between minImageExtent and maxImageExtent, inclusive, where minImageExtent and maxImageExtent are members of the
    VkSurfaceCapabilitiesKHR structure returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR for the surface
    (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-VkSwapchainCreateInfoKHR-imageExtent-01274)`
    - also it can mess up the gui more frequently (I try and suspend drawing to the gui if I have resized, but not rebuilt the gui)
    - I actually think there are three seperate bugs... *exasperated walrus noises* (one with the framebuffer image extent, one where the gui buffer is used while it is still copying or 
 * Get the nvidia api inspector figured out (I am crashing on vkEnumeratePhysicalDevices) (I migh just need to add the layer???)