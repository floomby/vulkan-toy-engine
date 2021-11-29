## Next steps
 * Shader multisampling (This is going to create a problem with alpha discard for the icons)
 * Unit highlighting shader improvement
 * Finish projectile stuff
 * gui - I still need some more components, but this seems really low priority right now
 * Finish the path following/steering code
 * Finish unit ais
 * Fix the order line drawing bug
 * Figure out engine to lua messaging - Options are piggyback off the gui to lua messaging or do something else entirly new. (Something new seems preferable.)
 * There are problems with building stuff. I had to patch the castxml rubygem I was using to make it work.

## Considerations
 * I am linking to libpng now for freetype2, maybe I should stop using stbi
 * The string of glyphs to cache is pretty silly imo
 * cpu (host) allocation stuff for vulkan (vma)?? (This is probably not worth doing until I profile stuff)
 * I think I read the glfw docs wrong and dont need to be using interthread queues for the input handling threads (afaict the linux tids are the same as the engine thread, this could be not guarenteed though)
 * The background rendinging pass is almost entirely uneeded at this point, I moved the icon rendering from this pass into the world pass because without it the code complexity of order line drawing was going to be really high doing it this way
 * One wierd thing is how the lighting works, the blin phong lighting model I was using was using a point light, but I am using orthographic lighting for the shadows. I should fix this
 * I should move to using cmake or something for building
 * I need to look around at vram memory usage in general, it is very inefficient in many places

## Longer term
 * Collider abstraction so I can support aabb and importantly obb coliders
 * I would like bloom, idk if this means I need hdr color attachements or what
 * Better model library?
 * Cube mapping for lights and calculating shadows with them would be cool
 * Dynamic backface culling (Idk if this is a thing, the hardware supports it, but my prefunctory perusing through the vulkan docs did not show this was availible)
   - It might be one of those things that on some hardware would require rebuilding the pipeline and trigger shader recompilation and therefore might not be in the standard.
   - I should fix the normals on the skybox anyways
   - For transparency I may not want backface culling at all though
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
 * Get the nvidia api inspector figured out (I am crashing on vkEnumeratePhysicalDevices)