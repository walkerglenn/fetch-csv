// Dear ImGui: standalone example application for Glfw + Vulkan

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
//   You will use those if you want to use this rendering backend in your engine/app.
// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
//   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
// Read comments in imgui_impl_vulkan.h.

#include "FetchCsvLib.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_stdlib.h"
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <nfd_glfw3.h>
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <string>
#include <optional>
#include <utility>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

//#define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
#endif

// Data
static VkAllocationCallbacks*   g_Allocator = nullptr;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static void SetupVulkan(ImVector<const char*> instance_extensions)
{
    VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(err);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_Instance);
#endif

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif
    }

    // Select Physical Device (GPU)
    g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
    IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);

    // Select graphics queue family
    g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
    IM_ASSERT(g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    // Create Descriptor Pool
    // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->Surface = surface;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_COUNTOF(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_COUNTOF(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount, 0);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow(ImGui_ImplVulkanH_Window* wd)
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, wd, g_Allocator);
    vkDestroySurfaceKHR(g_Instance, wd->Surface, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}

// Native file dialog (nativefiledialog) functions
void error_callback(int error_code, const char* description) {
	std::cerr << "GLFW call failed (code " << error_code << "): " << description << '\n';
}

void set_native_window(GLFWwindow* glfwWindow, nfdwindowhandle_t* nativeWindow) {
    if (!NFD_GetNativeWindowFromGLFWWindow(glfwWindow, nativeWindow)) {
	    std::cerr << "NFD_GetNativeWindowFromGLFWWindow failed" << '\n';
    }
}

const std::string open_file(GLFWwindow* window, int mods = 0)
{
	(void)mods;
	char* path;
	std::string pathCppStr {""};
            nfdopendialogu8args_t args = {0};
            set_native_window(window, &args.parentWindow);
            const nfdresult_t res = NFD_OpenDialogU8_With(&path, &args);
            switch (res)
	    {
                case NFD_OKAY:
		    std::cout << "NFD_OpenDialogU8_With success: " << path << '\n';
		    pathCppStr = path;
                    NFD_FreePathU8(path);
		    return pathCppStr;
                    break;
                case NFD_CANCEL:
		    std::cout << "NFD_OpenDialogU8_With cancelled" << '\n';
		    return pathCppStr;
                    break;
                case NFD_ERROR:
		    std::cerr << "NFD_OpenDialogU8_With error: " << NFD_GetError() << '\n';
		    return pathCppStr;
		    break;
                default:
		    return pathCppStr;
                    break;
	    }
}

const std::string save_file(GLFWwindow* window, int mods = 0)
{
	(void)mods;
	char* path;
	std::string pathCppStr {""};
            nfdsavedialogu8args_t args = {0};
            set_native_window(window, &args.parentWindow);
            const nfdresult_t res = NFD_SaveDialogU8_With(&path, &args);
            switch (res)
	    {
                case NFD_OKAY:
		    std::cout << "NFD_SaveDialogU8_With success: " << path << '\n';
		    pathCppStr = path;
                    NFD_FreePathU8(path);
		    return pathCppStr;
                    break;
                case NFD_CANCEL:
		    std::cout << "NFD_SaveDialogU8_With cancelled" << '\n';
		    return pathCppStr;
                    break;
                case NFD_ERROR:
		    std::cerr << "NFD_OpenDialogU8_With error: " << NFD_GetError() << '\n';
		    return pathCppStr;
		    break;
                default:
		    return pathCppStr;
                    break;
	    }
}

// FetchCSV-specific functions ###

static void paginateToIndex(const size_t targetIndex, const FetchCSV::DataFrame& activeDf, FetchCSV::AppState& appState)
{
	size_t numCols { activeDf.getNumColumns() };
	size_t cellsPerPage { appState.numRowsToDisplay * numCols };
	size_t targetPageFirstIndex { ( targetIndex / (cellsPerPage) * cellsPerPage) };
	appState.pageStartIndex = targetPageFirstIndex;
}

static void showMainMenuBar(FetchCSV::DataFrame& activeDf, GLFWwindow* window, FetchCSV::AppState& appState)
{

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New"))
		{
			std::cerr << "Not implemented!" << '\n';
		}

		if (ImGui::MenuItem("Open"))
		{
			std::string selectedPath {open_file(window)};
			if (selectedPath != "")
			{
				activeDf.loadData(selectedPath, appState.delimiter);
			}
		}
		
		if (ImGui::MenuItem("Save"))
		{
			std::string selectedPath {activeDf.getFilePath()};
			if (selectedPath != "")
			{
				if (activeDf.saveData(selectedPath, appState.delimiter))
				{
					std::cout << "DF saved to: " << selectedPath << '\n';
				}
			}
		}

		if (ImGui::MenuItem("Save As..."))
		{
			std::string selectedPath {save_file(window)};
			if (selectedPath != "")
			{
				if (activeDf.saveData(selectedPath, appState.delimiter))
				{
					std::cout << "DF saved to: " << selectedPath << '\n';
				}
			}
		}

		if (ImGui::MenuItem("Close")) //TODO: Ask the user if they really want to close
		{
			glfwSetWindowShouldClose(window, true);
		}

		ImGui::EndMenu();
	}
	
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::BeginMenu("Sort"))
		{
			if (ImGui::MenuItem("Placeholder"))
			{
				std::cerr << "Not implemented!" << '\n';
			}
			
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Display"))
		{
			if (ImGui::MenuItem("Column Width"))
			{
				std::cerr << "Not implemented!" << '\n';
			}

			static std::string showHideHeadersMenuText { "Hide Headers" };
			if (appState.shouldRenderHeaders)
			{
				showHideHeadersMenuText = "Hide Headers";
			}
			else
			{
				showHideHeadersMenuText = "Show Headers";
			}

			if (ImGui::MenuItem(showHideHeadersMenuText.c_str()))
			{
				appState.shouldRenderHeaders = !appState.shouldRenderHeaders;
			}
			
			
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Set Delimiter"))
		{
			if (ImGui::MenuItem(","))
			{
				appState.delimiter = ',';
			}

			if (ImGui::MenuItem("<TAB>"))
			{
				appState.delimiter = '\t';
			}

			if (ImGui::MenuItem("Custom..."))
			{
				appState.showCustomDelimiterWindow = true;
			}
			
			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Find"))
	{
		if (ImGui::MenuItem("Search for Value"))
		{
			appState.showValueSearchWindow = true;
		}

		if (ImGui::MenuItem("Jump to Row"))
		{
			std::cerr << "Not implemented!" << '\n';
		}

		ImGui::EndMenu();
	}
}

static void displayDelimiter(FetchCSV::AppState& appState)
{
	std::string delimLabelStr { appState.delimiter };

	if (delimLabelStr[0] == '\t')
	{
		delimLabelStr = "<TAB>";
	}
	
	delimLabelStr = "\tValue Delimiter: " + delimLabelStr;

	const char* delimLabelText { delimLabelStr.c_str() };

	ImGui::Text(delimLabelText);
}

// Main code
int main(int argc, char* argv[])
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    if (NFD_Init() != NFD_OKAY)
    {
	std::cerr << "NFD_Init failed: " << NFD_GetError();
	abort();    
    }
    
    if (!NFD_SetDisplayPropertiesFromGLFW()) {
	    std::cerr << "NFD_SetDisplayPropertiesFromGLFW failed" << '\n';
    }
    
    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "FetchCSV", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(err);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.Allocator = g_Allocator;
    init_info.PipelineInfoMain.RenderPass = wd->RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will call AddFontDefault() to select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    //bool show_demo_window = true;
    //bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Resize swap chain?
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount, 0);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

	// FetchCSV main functionality begins here ###
	
	// Create and show the main fullscreen window
	static ImGuiWindowFlags mainWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

	const ImGuiViewport* viewport { ImGui::GetMainViewport() };
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);	
	
	ImGui::Begin("FetchCSV-Window", nullptr, mainWindowFlags);

	// In case we need to see the demo window for reference	
	//static bool showDemo {false};
	//ImGui::ShowDemoWindow(&showDemo);

	// Instatiate our app state
	static FetchCSV::AppState appState {};

	// Initialize the active dataframe
	static FetchCSV::DataFrame activeDataFrame {};

	// Initialize the search state, this tells our spreadsheet rendering system if we need to focus on a particular cell that has a searched value in it
	static std::pair<bool, size_t> searchState;

	// If we have command-line arguments, try to load the dataframe from that path
	if ( (appState.shouldLoadCsv) && (argc > 1) )
	{
		activeDataFrame.loadData(argv[1]);
		appState.shouldLoadCsv = false;
	}
	
	// Menu bar
	if (ImGui::BeginMenuBar())
	{
		showMainMenuBar(activeDataFrame, window, appState);
	}

	ImGui::EndMenuBar();

	// Value search menu 
	if (appState.showValueSearchWindow)
	{
	    ImGui::SetNextWindowFocus(); // Always focus on this window when it's open
	    ImGui::Begin("ValueSearchWindow", &appState.showValueSearchWindow, ImGuiWindowFlags_AlwaysAutoResize);
	    static std::string searchValue {""};
	    ImGui::InputText("##SearchInput", &searchValue);

	    ImGui::Checkbox("Exact Match", &appState.isExactChecked);

	    if (ImGui::Button("Search"))
	    {
		std::optional<size_t> searchResult { activeDataFrame.getIndexOfValue(searchValue, appState.isExactChecked) };

		if (searchResult)
		{
			appState.searchState.first = true ;
			appState.searchState.second = *searchResult;
			appState.showValueSearchWindow = false;
			paginateToIndex(*searchResult, activeDataFrame, appState);
		}
		else
		{
			std::cout << "Value " << searchValue << " not found in spreadsheet.\n";	
		}
	    }

	    ImGui::SameLine();

	    if (ImGui::Button("Find Next"))
	    {
		std::optional<size_t> searchResult { activeDataFrame.getIndexOfValue(searchValue, appState.isExactChecked, appState.searchState.second + 1) };
		if (searchResult)
		{
			appState.searchState.first = true ;
			appState.searchState.second = *searchResult;
			appState.showValueSearchWindow = false;
			paginateToIndex(*searchResult, activeDataFrame, appState);
		}
		else
		{
			std::cout << "Value " << searchValue << " not found in spreadsheet.\n";	
		}
	    }

	    ImGui::End();
	}

	if (appState.showCustomDelimiterWindow)
	{
		ImGui::Begin("CustomDelimiterWindow", &appState.showCustomDelimiterWindow, ImGuiWindowFlags_AlwaysAutoResize);
		static std::string customDelimiterInput { "" };
		ImGui::InputText("##CustomDelimiterInput", &customDelimiterInput);
		
		if ( (ImGui::Button("Set")) && (customDelimiterInput.length() > 0) )
		{
			appState.delimiter = customDelimiterInput[0];
			appState.showCustomDelimiterWindow = false;	
		}

		ImGui::End();
	}

	// Pagination variables
	size_t numColumns { activeDataFrame.getNumColumns() };
	size_t numCells { activeDataFrame.getNumCells() };

	size_t numCellsToRender { appState.numRowsToDisplay * numColumns };	
	if (numCellsToRender > numCells)
	{
		numCellsToRender = numCells;
	}

	// Pagination controls
	if (ImGui::Button("<<"))
	{
		appState.pageStartIndex = 0;
	}

	ImGui::SameLine();

	if (ImGui::Button("<"))
	{
		if (!(appState.pageStartIndex == 0))
		{
			appState.pageStartIndex -= numCellsToRender;
		}
	}
	
	ImGui::SameLine();

	if (ImGui::Button(">"))
	{
		appState.pageStartIndex += numCellsToRender;
	}

	ImGui::SameLine();

	if (ImGui::Button(">>"))
	{
		appState.pageStartIndex = numCells - numCellsToRender;
	}

	// Recalculate rendering indexes based on pagination changes
	if (appState.pageStartIndex + numCellsToRender > numCells)
	{
		appState.pageEndIndex = numCells;
		appState.pageStartIndex = appState.pageEndIndex - numCellsToRender;
	}
	else
	{
		appState.pageEndIndex = appState.pageStartIndex + numCellsToRender;
	}

	ImGui::SameLine();

	// Show the current delimiter
	displayDelimiter(appState);	

	// Render the dataFrame (and headers) in a Child section
	static const float scrollBarSize { ImGui::GetStyle().ScrollbarSize };
	static const float framePaddingHeight { ImGui::GetStyle().FramePadding.y };
	static const float framePaddingWidth { ImGui::GetStyle().FramePadding.x };
	static const ImVec2 itemSpacing { ImGui::GetStyle().ItemSpacing };
	static float framedWidgetHeight { ImGui::GetFrameHeightWithSpacing() };
	static float xScrollPos { 0.0 };

	// Render headers (if we're supposed to)
	if (appState.shouldRenderHeaders)	
	{
		ImGui::BeginChild("HeaderPanel", ImVec2( (viewport->Size.x - scrollBarSize - framePaddingWidth), (framedWidgetHeight + (framePaddingHeight * 2) + (itemSpacing.y * 2) ) ), ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar);

		FetchCSV::renderSpreadSheet(activeDataFrame, 0, numColumns, appState);

		ImGui::SetScrollX(xScrollPos);

		ImGui::EndChild();
	}
	
	// Main spreadsheet rendering section
	static const float spreadsheetPanelStartPos { ImGui::GetCursorScreenPos().y };

	ImGui::BeginChild("SpreadsheetMainPanel", ImVec2( (viewport->Size.x - scrollBarSize - framePaddingWidth), viewport->Size.y - spreadsheetPanelStartPos - (framePaddingHeight * 3) ), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

	FetchCSV::renderSpreadSheet(activeDataFrame, appState.pageStartIndex, appState.pageEndIndex, appState);
	
	xScrollPos = ImGui::GetScrollX();

	ImGui::EndChild();

	ImGui::End();

	/* We'll keep this here as an example of how this works
        
        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

	*/

        // Rendering
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        if (!is_minimized)
        {
            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
            wd->ClearValue.color.float32[3] = clear_color.w;
            FrameRender(wd, draw_data);
            FramePresent(wd);
        }
    }

    // Cleanup
    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkanWindow(&g_MainWindowData);
    CleanupVulkan();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
