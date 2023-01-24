#include "vulkan_swapchain.hpp"
#include "vulkan/vulkan_structs.hpp"

namespace Vulkan
{

Swapchain::Swapchain(vk::Device device_, vk::PhysicalDevice physical_device_, vk::Queue queue_, vk::SurfaceKHR surface_, vk::CommandPool command_pool_)
    : surface(surface_),
      command_pool(command_pool_),
      physical_device(physical_device_),
      queue(queue_)
{
    device = device_;
    create_render_pass();
}

Swapchain::~Swapchain()
{
}

bool Swapchain::set_vsync(bool new_setting)
{
    if (new_setting == vsync)
        return false;

    vsync = new_setting;
    return true;
}

void Swapchain::create_render_pass()
{
    auto attachment_description = vk::AttachmentDescription{}
        .setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    auto attachment_reference = vk::AttachmentReference{}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    std::array<vk::SubpassDependency, 2> subpass_dependency{};
    subpass_dependency[0]
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits(0))
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    subpass_dependency[1]
        .setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);


    auto subpass_description = vk::SubpassDescription{}
        .setColorAttachments(attachment_reference)
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

    auto render_pass_create_info = vk::RenderPassCreateInfo{}
        .setSubpasses(subpass_description)
        .setDependencies(subpass_dependency)
        .setAttachments(attachment_description);

    render_pass = device.createRenderPassUnique(render_pass_create_info);
}

bool Swapchain::recreate(int new_width, int new_height)
{
    device.waitIdle();

    return create(num_frames, new_width, new_height);
}

vk::Image Swapchain::get_image()
{
    return imageviewfbs[current_swapchain_image].image;
}

bool Swapchain::create(unsigned int desired_num_frames, int new_width, int new_height)
{
    frames.clear();
    imageviewfbs.clear();

    auto surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);

    if (surface_capabilities.minImageCount > desired_num_frames)
        num_frames = surface_capabilities.minImageCount;
    else
        num_frames = desired_num_frames;

    extents = surface_capabilities.currentExtent;

    if (new_width > 0 && new_height > 0)
    {
        // No buffer is allocated for surface yet
        extents.width = new_width;
        extents.height = new_height;
    }
    else if (extents.width < 1 || extents.height < 1)
    {
        // Surface is likely hidden
        printf("Extents too small.\n");
        swapchain_object.reset();
        return false;
    }

    auto swapchain_create_info = vk::SwapchainCreateInfoKHR{}
        .setMinImageCount(num_frames)
        .setImageFormat(vk::Format::eB8G8R8A8Unorm)
        .setImageExtent(extents)
        .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(true)
        .setPresentMode(vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate)
        .setSurface(surface)
        .setImageArrayLayers(1);

    if (swapchain_object)
        swapchain_create_info.setOldSwapchain(swapchain_object.get());

    swapchain_object = device.createSwapchainKHRUnique(swapchain_create_info);
    if (!swapchain_object)
        return false;

    auto swapchain_images = device.getSwapchainImagesKHR(swapchain_object.get());
    vk::CommandBufferAllocateInfo command_buffer_allocate_info(command_pool, vk::CommandBufferLevel::ePrimary, num_frames);
    auto command_buffers = device.allocateCommandBuffersUnique(command_buffer_allocate_info);

    if (imageviewfbs.size() > num_frames)
        num_frames = imageviewfbs.size();

    frames.resize(num_frames);
    imageviewfbs.resize(num_frames);

    vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);

    for (unsigned int i = 0; i < num_frames; i++)
    {
        // Create frame queue resources
        auto &frame = frames[i];
        frame.command_buffer = std::move(command_buffers[i]);
        frame.fence = device.createFenceUnique(fence_create_info);
        frame.acquire = device.createSemaphoreUnique({});
        frame.complete = device.createSemaphoreUnique({});
    }
    current_frame = 0;

    for (unsigned int i = 0; i < num_frames; i++)
    {
        // Create resources associated with swapchain images
        auto &image = imageviewfbs[i];
        image.image = swapchain_images[i];
        auto image_view_create_info = vk::ImageViewCreateInfo{}
            .setImage(swapchain_images[i])
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(vk::Format::eB8G8R8A8Unorm)
            .setComponents(vk::ComponentMapping())
            .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        image.image_view = device.createImageViewUnique(image_view_create_info);

        auto framebuffer_create_info = vk::FramebufferCreateInfo{}
            .setAttachments(image.image_view.get())
            .setWidth(extents.width)
            .setHeight(extents.height)
            .setLayers(1)
            .setRenderPass(render_pass.get());
        image.framebuffer = device.createFramebufferUnique(framebuffer_create_info);
    }

    device.waitIdle();

    current_swapchain_image = 0;

    return true;
}

bool Swapchain::begin_frame()
{
    if (!swapchain_object || extents.width < 1 || extents.height < 1)
    {
        printf ("Extents too small\n");
        return false;
    }

    auto &frame = frames[current_frame];

    auto result = device.waitForFences(frame.fence.get(), true, 33000000);
    if (result != vk::Result::eSuccess)
    {
        printf("Failed fence\n");
        return false;
    }

    auto result_value = device.acquireNextImageKHR(swapchain_object.get(), 33000000, frame.acquire.get());
    if (result_value.result == vk::Result::eErrorOutOfDateKHR ||
        result_value.result == vk::Result::eSuboptimalKHR)
    {
        printf("Out of date\n");
        recreate();
        return begin_frame();
    }

    if (result_value.result != vk::Result::eSuccess)
    {
        printf("Timeout waiting for frame. Running too slow.\n");
        return false;
    }

    device.resetFences(frame.fence.get());
    current_swapchain_image = result_value.value;

    vk::CommandBufferBeginInfo command_buffer_begin_info(vk::CommandBufferUsageFlags{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    frame.command_buffer->begin(command_buffer_begin_info);

    return true;
}

bool Swapchain::end_frame(vk::Fence extra_fence)
{
    auto &frame = frames[current_frame];
    frame.command_buffer->end();

    vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info(
        frame.acquire.get(),
        flags,
        frame.command_buffer.get(),
        frame.complete.get());

    if (extra_fence)
        queue.submit(submit_info, extra_fence);
    queue.submit(submit_info, frame.fence.get());

    auto present_info = vk::PresentInfoKHR{}
        .setWaitSemaphores(frames[current_frame].complete.get())
        .setSwapchains(swapchain_object.get())
        .setImageIndices(current_swapchain_image);

    auto result = queue.presentKHR(present_info);

    current_frame = (current_frame + 1) % num_frames;

    if (result != vk::Result::eSuccess)
        return false;
    return true;
}

vk::Framebuffer Swapchain::get_framebuffer()
{
    return imageviewfbs[current_swapchain_image].framebuffer.get();
}

vk::CommandBuffer &Swapchain::get_cmd()
{
    return frames[current_frame].command_buffer.get();
}

void Swapchain::begin_render_pass()
{
    vk::ClearColorValue colorval;
    colorval.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });
    vk::ClearValue value;
    value.setColor(colorval);

    auto render_pass_begin_info = vk::RenderPassBeginInfo{}
        .setRenderPass(render_pass.get())
        .setFramebuffer(imageviewfbs[current_swapchain_image].framebuffer.get())
        .setRenderArea(vk::Rect2D({}, extents))
        .setClearValues(value);
    get_cmd().beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
}

void Swapchain::end_render_pass()
{
    get_cmd().endRenderPass();
}

unsigned int Swapchain::get_current_frame()
{
    return current_frame;
}

bool Swapchain::wait_on_frame(int frame_num)
{
    auto result = device.waitForFences(frames[frame_num].fence.get(), true, 33000000);
    return (result == vk::Result::eSuccess);
}

vk::Extent2D Swapchain::get_extents()
{
    return extents;
}

vk::RenderPass &Swapchain::get_render_pass()
{
    return render_pass.get();
}

unsigned int Swapchain::get_num_frames()
{
    return num_frames;
}

} // namespace Vulkan